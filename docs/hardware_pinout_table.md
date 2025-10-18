# 🔌 Hardware Pinout Table — Smart Irrigation System

This document lists all electrical connections between microcontrollers, sensors, actuators, and the communication gateway (ESP32) for the Smart Irrigation System prototype.

| Module | Component | Arduino/ESP32 Pin | Type | Signal Direction | Description |
|----------|------------|-------------------|-------|------------------|--------------|
| **Arduino UNO R4 WiFi** | Soil Moisture Sensor 1 | A0 | Analog Input | → MCU | Reads first soil moisture level |
| **Arduino UNO R4 WiFi** | Soil Moisture Sensor 2 | A1 | Analog Input | → MCU | Reads second soil moisture level |
| **Arduino UNO R4 WiFi** | LDR + 10kΩ Divider | A2 | Analog Input | → MCU | Measures ambient light intensity |
| **Arduino UNO R4 WiFi** | DHT22 (Temp/Humidity) | D7 | Digital Input | → MCU | Reads temperature and humidity |
| **Arduino UNO R4 WiFi** | TIP120 Transistor (Pump Control) | D8 | Digital Output | MCU → Pump | Activates/deactivates the water pump |
| **Arduino UNO R4 WiFi** | TX (UART Output) | TX1 (Pin 1) | Serial Output | MCU → ESP32 | Sends telemetry and inference data to ESP32 |
| **Arduino UNO R4 WiFi** | RX (UART Input) | RX0 (Pin 0) | Serial Input | ESP32 → MCU | Receives commands (optional) |
| **ESP32 (Gateway)** | RX | GPIO16 | Serial Input | ← Arduino | Receives UART data |
| **ESP32 (Gateway)** | TX | GPIO17 | Serial Output | → Arduino | Sends data back (optional) |
| **ESP32 (Gateway)** | WiFi | — | Network | → Cloud | Uploads data to ThingSpeak IoT Cloud |
| **Power Supply** | VCC 5V | 5V | Power | → System | Supplies sensors and Arduino |
| **Power Supply** | VCC 12V | External | Power | → Pump | Supplies the motor/pump circuit |
| **Power Supply** | GND | Common | Ground | Shared | Common reference between Arduino, ESP32, and motor circuit |

---

## ⚙️ Notes

- **UART communication** uses logic-level shifting if necessary (Arduino TX → ESP32 RX protected via 1kΩ–2kΩ voltage divider).  
- **All grounds (GND)** must be shared between Arduino UNO R4 WiFi, ESP32, and the external 12 V power supply.  
- The **DHT22** sensor uses a single-wire digital interface; its data pin is connected to D7 with a 10 kΩ pull-up resistor.  
- The **pump control circuit** uses a TIP120 NPN Darlington transistor, with a 1N4007 flyback diode across the motor terminals.  

---

## 📁 File References

- `hardware/tinkercad/tinkercad_circuit_complete.png` — Physical breadboard layout  
- `docs/diagrams/schematic_diagram.pdf` — Electrical schematic  
- `firmware/arduino/main.ino` — Sensor acquisition & TinyML inference  
- `firmware/esp32/main.ino` — UART receiver & cloud uploader
