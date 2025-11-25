import serial
import time

# Simple Bluetooth SPP reader for testing
# It reads from /dev/rfcomm0 (ESP32 SPP) and prints incoming lines.

PORT = "/dev/rfcomm0"
BAUDRATE = 9600  # symbolic for SPP, but required by pyserial
TIMEOUT = 1.0    # seconds


def main():
    print("[INFO] Opening Bluetooth serial port:", PORT)
    ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)

    try:
        while True:
            line = ser.readline()
            if line:
                try:
                    text = line.decode("utf-8", errors="replace").strip()
                except Exception:
                    text = str(line)
                print(f"[BT] Received: {text}")
            else:
                # No data in this interval, avoid busy loop
                time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n[INFO] Stopping bt_reader...")
    finally:
        ser.close()
        print("[INFO] Serial port closed.")


if __name__ == "__main__":
    main()
