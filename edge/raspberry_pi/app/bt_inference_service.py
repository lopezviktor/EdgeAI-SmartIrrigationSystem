import serial
import time

from edge.raspberry_pi.core.edge_model import EdgeIrrigationModel

# Bluetooth SPP reader + telemetry parser + TinyML inference.
# It reads from /dev/rfcomm0 (ESP32 SPP), parses lines like:
#   S1:<value>,S2:<value>,T:<value>,H:<value>,L:<value>
# builds a 6-feature vector and runs the TFLite model to decide:
#   0 = WATER_OFF, 1 = WATER_ON

PORT = "/dev/rfcomm0"
BAUDRATE = 9600  # symbolic for SPP, required by pyserial
TIMEOUT = 1.0    # seconds


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
        raise ValueError(f"Incomplete telemetry: {data}")

    return data


def build_feature_vector(data):
    """
    Build the 6-feature vector expected by the MinMaxScaler + TFLite model.

    We use:
      soil1     = S1
      soil2     = S2
      soil_mean = (S1 + S2) / 2
      temp      = T
      hum       = H
      light     = L
    """
    s1 = data["S1"]
    s2 = data["S2"]
    temp = data["T"]
    hum = data["H"]
    light = data["L"]

    soil_mean = 0.5 * (s1 + s2)

    # Order of features must match the scaler training:
    # [soil1, soil2, soil_mean, temp, hum, light]
    return [s1, s2, soil_mean, temp, hum, light]


def main():
    print("[INFO] Opening Bluetooth serial port:", PORT)
    ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)

    # Load TinyML model once at startup
    model = EdgeIrrigationModel(
        model_path="model/model.tflite",
        scaler_path="model/scaler.joblib"
    )

    try:
        while True:
            raw = ser.readline()
            if not raw:
                time.sleep(0.1)
                continue

            try:
                text = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                text = str(raw)

            if not text:
                continue

            print(f"[BT] Raw line: {text}")

            try:
                data = parse_telemetry(text)
                print(
                    "[PARSED] "
                    f"S1={data['S1']:.1f}, "
                    f"S2={data['S2']:.1f}, "
                    f"T={data['T']:.1f}°C, "
                    f"H={data['H']:.1f}%, "
                    f"L={data['L']:.0f}"
                )

                # Build the 6-feature vector and run inference
                features = build_feature_vector(data)
                prob, label = model.predict(features)

                decision = "WATER_ON" if label == 1 else "WATER_OFF"
                print(
                    f"[MODEL] features={features} "
                    f"-> prob={prob:.3f}, decision={decision}"
                )

                # Send decision back to ESP32 over Bluetooth
                try:
                    msg = f"DECISION:{decision}\n"
                    ser.write(msg.encode("utf-8"))
                    ser.flush()
                    print(f"[BT-TX] Sent to ESP32: {msg.strip()}")
                except Exception as e:
                    print(f"[WARN] Could not send decision over BT: {e}")

            except ValueError as e:
                print(f"[WARN] Could not parse telemetry: {e}")
            except Exception as e:
                print(f"[ERROR] Inference failed: {e}")

    except KeyboardInterrupt:
        print("\n[INFO] Stopping bt_inference_service...")
    finally:
        ser.close()
        print("[INFO] Serial port closed.")


if __name__ == "__main__":
    main()
