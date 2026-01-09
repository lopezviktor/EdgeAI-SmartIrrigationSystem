import os
import sys
import serial
import time
import json
from collections import deque

import numpy as np
import joblib

BASE_DIR = os.path.dirname(os.path.dirname(__file__))  # .../edge/raspberry_pi
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)

MODEL_DIR = os.path.join(BASE_DIR, "model")
DOSE_DIR = os.path.join(MODEL_DIR, "dose")

# Bluetooth SPP reader + telemetry parser + decision + (optional) dose inference.
# It reads from /dev/rfcomm0 (ESP32 SPP), parses lines like:
#   S1:<value>,S2:<value>,T:<value>,H:<value>,L:<value>
#
# Production approach (robust + explainable):
# - ON/OFF decision: rule-based gating with hysteresis (HIGH = dry, LOW = wet)
# - Dose (seconds): RandomForest regressor (shadow mode until we enable actuation)
#   (This file loads the joblib model + feature contract JSON directly.)

PORT = "/dev/rfcomm0"
BAUDRATE = 9600  # symbolic for SPP, required by pyserial
TIMEOUT = 5.0  # seconds (avoid partial reads on slow/fragmented SPP delivery)


def parse_telemetry(line: str):
    """
    Parse a telemetry line like:
      S1:802.0,S2:822.0,T:21.7,H:69.6,L:202
    into a dictionary with numeric values.
    """
    parts = line.split(",")
    data = {}

    for part in parts:
        if ":" not in part:
            continue
        key, value = part.split(":", 1)
        key = key.strip()
        value = value.strip()

        try:
            data[key] = float(value)
        except ValueError:
            continue

    required_keys = ("S1", "S2", "T", "H", "L")
    if not all(k in data for k in required_keys):
        # Bluetooth SPP can deliver partial / corrupted lines. Treat as a soft error and skip.
        return None

    return data


def main():
    print("[INFO] Opening Bluetooth serial port:", PORT)

    dose_model_path = os.path.join(DOSE_DIR, "rf_dose_regressor_prod.joblib")
    dose_features_path = os.path.join(DOSE_DIR, "rf_dose_features_prod.json")

    dose_model = joblib.load(dose_model_path)

    with open(dose_features_path, "r", encoding="utf-8") as f:
        dose_features_obj = json.load(f)

    # The prod features file is stored as: {"features": [..29 names..]}
    if isinstance(dose_features_obj, dict) and "features" in dose_features_obj:
        dose_feature_names = dose_features_obj["features"]
    else:
        # Backward compatibility: allow a plain list of feature names
        dose_feature_names = dose_features_obj

    allowed_seconds = [8.0, 14.0, 18.0, 24.0]

    print("[DOSE] Dose model loaded (shadow mode)")
    print(f"[DOSE] Features contract: {len(dose_feature_names)} features")

    # --- Rule-based ON/OFF gating with hysteresis (data-driven thresholds) ---
    # Confirmed by real watering test: higher raw readings mean drier soil.
    # We gate on soil_avg to avoid uneven wetting between sensors.
    # Thresholds derived from your real pump dataset around irrigation events:
    # - DRY ~ 75th percentile of soil_avg during the 60 minutes before irrigation
    # - WET ~ conservative stop threshold (hysteresis) so we don't flap
    SOIL_AVG_DRY = 521.0
    SOIL_AVG_WET = 480.0

    SIMULATE_DRY = False  # set back to False after the test
    SIM_DRY_VALUE = 530.0

    watering_state = False

    # --- Rolling window for dose features (PRE window) ---
    # The production dose model expects 29 features built from a PRE window.
    # We use the last PRE_N samples to compute mean/std/min/max.
    SAMPLE_MINUTES = 3
    PRE_MINUTES = 30
    PRE_N = PRE_MINUTES // SAMPLE_MINUTES  # 10

    window = deque(
        maxlen=PRE_N
    )  # each item: dict with s1,s2,soil_avg,soil_diff,temp,hum

    ser = None
    rx_buffer = ""

    def _stats(values):
        arr = np.asarray(values, dtype=float)
        return (
            float(arr.mean()),
            float(arr.std(ddof=0)),
            float(arr.min()),
            float(arr.max()),
        )

    def build_dose_features(win, at_event):
        """Build the 29-feature dict expected by rf_dose_features_prod.json."""
        # Extract series
        s1s = [r["soil1"] for r in win]
        s2s = [r["soil2"] for r in win]
        avgs = [r["soil_avg"] for r in win]
        diffs = [r["soil_diff"] for r in win]
        temps = [r["temperature"] for r in win]
        hums = [r["humidity"] for r in win]

        s1_mean, s1_std, s1_min, s1_max = _stats(s1s)
        s2_mean, s2_std, s2_min, s2_max = _stats(s2s)
        avg_mean, avg_std, avg_min, avg_max = _stats(avgs)
        diff_mean, diff_std, diff_min, diff_max = _stats(diffs)
        t_mean, t_std, t_min, t_max = _stats(temps)
        h_mean, h_std, h_min, h_max = _stats(hums)

        feats = {
            "soil1_pre_mean": s1_mean,
            "soil1_pre_std": s1_std,
            "soil1_pre_min": s1_min,
            "soil1_pre_max": s1_max,
            "soil2_pre_mean": s2_mean,
            "soil2_pre_std": s2_std,
            "soil2_pre_min": s2_min,
            "soil2_pre_max": s2_max,
            "soil_avg_pre_mean": avg_mean,
            "soil_avg_pre_std": avg_std,
            "soil_avg_pre_min": avg_min,
            "soil_avg_pre_max": avg_max,
            "soil_diff_pre_mean": diff_mean,
            "soil_diff_pre_std": diff_std,
            "soil_diff_pre_min": diff_min,
            "soil_diff_pre_max": diff_max,
            "temperature_pre_mean": t_mean,
            "temperature_pre_std": t_std,
            "temperature_pre_min": t_min,
            "temperature_pre_max": t_max,
            "humidity_pre_mean": h_mean,
            "humidity_pre_std": h_std,
            "humidity_pre_min": h_min,
            "humidity_pre_max": h_max,
            "delta_soil_avg_min_vs_pre_mean": float(avg_min - avg_mean),
            "soil_avg_at_event": float(at_event["soil_avg"]),
            "soil_diff_at_event": float(at_event["soil_diff"]),
            "temp_at_event": float(at_event["temperature"]),
            "humidity_at_event": float(at_event["humidity"]),
        }
        return feats

    def snap_seconds(x, allowed):
        return float(min(allowed, key=lambda a: abs(a - x)))

    while True:
        try:
            if ser is None or not ser.is_open:
                print("[INFO] Connecting to Bluetooth serial...")
                ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
                time.sleep(1)

            # Read chunks and reconstruct full lines. Bluetooth SPP can fragment messages.
            chunk = ser.read(256)
            if not chunk:
                time.sleep(0.1)
                continue

            rx_buffer += chunk.decode("utf-8", errors="replace")

            # Process all complete lines currently in the buffer
            while "\n" in rx_buffer:
                line, rx_buffer = rx_buffer.split("\n", 1)
                text = line.strip().strip("\r")
                if not text:
                    continue

                # Optional hard filter: ignore any non-telemetry lines
                # Telemetry must contain S1: and S2: at minimum.
                if "S1:" not in text or "S2:" not in text:
                    print(f"[WARN] Non-telemetry line, skipping: raw={text!r}")
                    continue

                print(f"[BT] Raw line: {text!r}")

                data = parse_telemetry(text)
                if data is None:
                    print(f"[WARN] Incomplete telemetry, skipping: raw={text!r}")
                    continue

                print(
                    "[PARSED] "
                    f"S1={data['S1']:.1f}, "
                    f"S2={data['S2']:.1f}, "
                    f"T={data['T']:.1f}°C, "
                    f"H={data['H']:.1f}%, "
                    f"L={data['L']:.0f}"
                )

                # --- Decision (ON/OFF) ---
                s1 = float(data["S1"])
                s2 = float(data["S2"])
                soil_avg = 0.5 * (s1 + s2)
                soil_diff = abs(s1 - s2)

                soil_avg_for_decision = SIM_DRY_VALUE if SIMULATE_DRY else soil_avg

                if not watering_state:
                    # Start watering if average soil reading is dry enough
                    if soil_avg_for_decision >= SOIL_AVG_DRY:
                        watering_state = True
                else:
                    # Stop watering only when the soil is wet enough
                    if soil_avg_for_decision <= SOIL_AVG_WET:
                        watering_state = False

                decision = "WATER_ON" if watering_state else "WATER_OFF"
                if SIMULATE_DRY:
                    print(
                        f"[DECISION] {decision} (rule-based, soil_avg={soil_avg:.1f}, used={soil_avg_for_decision:.1f})"
                    )
                else:
                    print(
                        f"[DECISION] {decision} (rule-based, soil_avg={soil_avg:.1f})"
                    )
                # Update rolling window (always)
                window.append(
                    {
                        "soil1": s1,
                        "soil2": s2,
                        "soil_avg": soil_avg,
                        "soil_diff": soil_diff,
                        "temperature": float(data["T"]),
                        "humidity": float(data["H"]),
                    }
                )

                # --- Dose (shadow mode) ---
                sec = 0.0
                if decision == "WATER_ON":
                    if len(window) < PRE_N:
                        print(
                            f"[DOSE] Not enough samples yet for PRE window: {len(window)}/{PRE_N}"
                        )
                    else:
                        at_event = {
                            "soil_avg": soil_avg,
                            "soil_diff": soil_diff,
                            "temperature": float(data["T"]),
                            "humidity": float(data["H"]),
                        }
                        feat_dict = build_dose_features(window, at_event)
                        X = np.array(
                            [[feat_dict[name] for name in dose_feature_names]],
                            dtype=float,
                        )
                        pred_cont = float(dose_model.predict(X)[0])
                        pred_snap = snap_seconds(pred_cont, allowed_seconds)
                        sec = pred_snap
                        print(
                            f"[DOSE] Predicted={pred_cont:.3f}s -> snapped={pred_snap:.1f}s"
                        )

                # --- Command back to ESP32/Arduino ---
                # Set to False if you want to disable actuation path while keeping inference running.
                SEND_COMMANDS = True

                cmd = f"CMD:{decision};SEC:{int(sec)}\n"

                if SEND_COMMANDS and ser is not None and ser.is_open:
                    try:
                        ser.write(cmd.encode("utf-8"))
                        ser.flush()
                        print(f"[TX] {cmd.strip()}")
                    except Exception as tx_err:
                        print(f"[WARN] TX failed, command not sent: {tx_err}")
                else:
                    print(f"[TX-DISABLED] {cmd.strip()}")

        except serial.SerialException as e:
            print(f"[WARN] Serial error: {e}")
            try:
                if ser:
                    ser.close()
            except Exception:
                pass
            ser = None
            time.sleep(2)

        except Exception as e:
            print(f"[ERROR] Unexpected error: {e}")
            time.sleep(1)


if __name__ == "__main__":
    main()
