# Raspberry Pi <-> ESP32/Arduino UART bridge for Edge Inference
# Protocol RX:
#   SOIL1=<f>,SOIL2=<f>,TEMP=<f>,HUM=<f>,LIGHT=<f>\n
# Reply TX (to MCU):
#   WATER_ON\n  |  WATER_OFF\n
#
# Notes:
# - Model expects 6 features: [soil1, soil2, soil_max, temp_c, humidity, light]
# - soil_max is computed here as max(soil1, soil2)
# - Use /dev/serial0 on Raspberry Pi (alias to primary UART)
# - Baud rate must match MCU side (default here: 9600)

import time
import serial
import numpy as np
import os, sys
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from inference_test import predict, decision_from_output

PORT = "/dev/serial0"
BAUD = 9600
TIMEOUT = 1  # seconds


def parse_line(line: str):
    """
    Parse a payload like:
      SOIL1=45.2, SOIL2=52.0,TEMP=23.5,HUM=48.0,LIGHT=300
    Return np.array shape (1,6) in the exact training order on None if invalid
    """
    try:
        parts = dict(kv.split("=", 1) for kv in line.split(","))
        soil1 = float(parts["SOIL1"])
        soil2 = float(parts["SOIL2"])
        temp = float(parts["TEMP"])
        hum = float(parts["HUM"])
        light = float(parts["LIGHT"])
        soil_max = max(soil1, soil2)
        x = np.array([[soil1, soil2, soil_max, temp, hum, light]], dtype=np.float32)
        return x
    except Exception:
        return None


def main():
    ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
    time.sleep(2)
    print(f"[INFO] UART open on {PORT} at {BAUD} baud.")

    while True:
        raw = ser.readline().decode(errors="ignore").strip()
        if not raw:
            continue
        print(f"[RX] {raw}")

        x = parse_line(raw)
        if x is None:
            print("[WARN] bad payload, skipping.")
            continue

        y = predict(x)
        decision = decision_from_output(y, threshold=0.5)

        ser.write((decision + "\n").encode())
        print(f"[TX] {decision}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[INFO] Stopped by user.")
