# Expected UART JSON payload (one line):
# {"soil1": 450, "soil2": 520, "temp": 22.80, "hum": 46.50, "light": 300}
# Feature order for the model: [soil1, soil2, temp, hum, light]

#!/usr/bin/env python3
import os, json, time
import numpy as np
import serial, requests, yaml
from dotenv import load_dotenv

# Load .env if present (non-fatal if missing)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ENV_PATH = os.path.join(BASE_DIR, "../../../_secrets/.env")
if os.path.exists(ENV_PATH):
    load_dotenv(ENV_PATH)

# YAML config
with open(os.path.join(BASE_DIR, "config", "settings.yaml"), "r") as f:
    config = yaml.safe_load(f)


def _env_expand(v):
    """Expand ${VAR} placeholders using current environment."""
    return os.path.expandvars(str(v)) if isinstance(v, str) else v


# Config values (allowing env expansion from .env)
SERIAL_PORT = _env_expand(config["serial"]["port"])
BAUD = int(_env_expand(config["serial"]["baud"]))
MODEL_PATH = _env_expand(config["model"]["path"])
THRESHOLD = float(_env_expand(config["model"]["threshold"]))
TS_URL = _env_expand(config["thingspeak"]["url"])
TS_KEY = _env_expand(config["thingspeak"]["api_key"])

# TFLite runtime (fallback to TensorFlow Lite if tflite-runtime is unavailable)
try:
    from tflite_runtime.interpreter import Interpreter  # type: ignore[reportMissingImports]
except ImportError:
    from tensorflow.lite.python.interpreter import Interpreter  # type: ignore[reportMissingImports]

interpreter = Interpreter(model_path=MODEL_PATH)
interpreter.allocate_tensors()
IN = interpreter.get_input_details()
OUT = interpreter.get_output_details()


def infer(features: np.ndarray) -> float:
    """Run inference on a single feature vector. Expected shape for this baseline: [1, n_features]."""
    x = features.astype(np.float32).reshape(1, -1)
    interpreter.set_tensor(IN[0]["index"], x)
    interpreter.invoke()
    y = interpreter.get_tensor(OUT[0]["index"]).flatten()[0]
    return float(y)


def parse_line(line: str) -> dict:
    """Parse a single JSON line from UART. Expected payload example:{"soil1": 450, "soil2": 520, "temp": 22.80, "hum": 46.50, "light": 300}"""
    try:
        return json.loads(line)
    except Exception:
        return {}


def to_vector(d: dict) -> np.ndarray:
    """Convert sensor dict to ordered feature vector: [soil1, soil2, temp, hum, light]."""
    soil1 = float(d.get("soil1", 0))
    soil2 = float(d.get("soil2", 0))
    temp = float(d.get("temp", 0))
    hum = float(d.get("hum", 0))
    light = float(d.get("light", 0))
    return np.array([soil1, soil2, temp, hum, light], dtype=np.float32)


def post_thingspeak(prob: float, decision: int, raw: dict):
    """Send inference result and raw features to ThingSpeak fields."""
    if not TS_KEY or not TS_URL:
        return
    payload = {
        "api_key": TS_KEY,
        "field1": raw.get("soil1"),
        "field2": raw.get("soil2"),
        "field3": raw.get("temp"),
        "field4": raw.get("hum"),
        "field5": raw.get("light"),
        "field6": decision,  # TinyML Prediction (0/1)
    }
    try:
        r = requests.post(TS_URL, data=payload, timeout=5)
        print(f"[TS] status={r.status_code}")
    except Exception as e:
        print(f"[TS][ERR] {e}")


def main():
    print(f"[EDGE] starting... UART={SERIAL_PORT}@{BAUD}, model={MODEL_PATH}")
    ser = serial.Serial(SERIAL_PORT, BAUD, timeout=1)
    time.sleep(1.5)  # give UART a moment to stabilize
    while True:
        try:
            line = ser.readline().decode("utf-8", errors="ignore").strip()
            if not line:
                continue
            data = parse_line(line)
            if not data:
                continue

            # Features -> inference -> decision
            x = to_vector(data)
            prob = infer(x)
            decision = 1 if prob >= THRESHOLD else 0

            # Return decision back to the gateway over UART
            ser.write(
                (json.dumps({"decision": decision, "prob": prob}) + "\n").encode(
                    "utf-8"
                )
            )

            # Push to ThingSpeak
            post_thingspeak(prob, decision, data)

            print(f"[EDGE] prob={prob:.3f} decision={decision} raw={data}")
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"[EDGE][ERR] {e}")
            time.sleep(0.5)


if __name__ == "__main__":
    main()
