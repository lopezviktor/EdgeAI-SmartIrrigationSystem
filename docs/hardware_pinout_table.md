# 🔌 Hardware Pinout Table — Smart Irrigation System

This updated table reflects the **final architecture** of the Smart Irrigation System, including the Arduino UNO R4 (sensing + pump control), ESP32 (gateway), and Raspberry Pi (Edge AI Node with Bluetooth SPP).

| Module | Component | Pin / Interface | Type | Direction | Description |
|--------|-----------|-----------------|-------|-----------|-------------|
| **Arduino UNO R4 WiFi** | Soil Moisture Sensor 1 | A0 | Analog Input | Sensor → MCU | Reads first soil moisture level |
| **Arduino UNO R4 WiFi** | Soil Moisture Sensor 2 | A1 | Analog Input | Sensor → MCU | Reads second soil moisture level |
| **Arduino UNO R4 WiFi** | LDR + 10kΩ Divider | A2 | Analog Input | Sensor → MCU | Measures ambient light intensity |
| **Arduino UNO R4 WiFi** | DHT22 | D7 | Digital Input | Sensor → MCU | Reads temperature and humidity |
| **Arduino UNO R4 WiFi** | TIP122 (Pump Control) | D8 | Digital Output | MCU → TIP122 Base | Drives transistor to control pump |
| **Arduino UNO R4 WiFi** | UART TX | TX1 (Pin 1) | Serial Output | MCU → ESP32 | Sends telemetry to ESP32 |
| **Arduino UNO R4 WiFi** | UART RX | RX0 (Pin 0) | Serial Input | ESP32 → MCU | Receives irrigation decision |

| **ESP32 (Gateway)** | UART RX | GPIO16 | Serial Input | Arduino → ESP32 | Receives telemetry from Arduino |
| **ESP32 (Gateway)** | UART TX | GPIO17 | Serial Output | ESP32 → Arduino | Sends TinyML decision to Arduino |
| **ESP32 (Gateway)** | Bluetooth SPP | — | Wireless | ESP32 ↔ RPi | Sends telemetry and receives decisions |
| **ESP32 (Gateway)** | WiFi | — | Network | ESP32 → Cloud | Uploads data to ThingSpeak |

| **Raspberry Pi 4 (Edge AI Node)** | Bluetooth SPP | /dev/rfcomm0 | Wireless Serial | ESP32 ↔ RPi | Receives telemetry and returns inference |
| **Raspberry Pi 4 (Edge AI Node)** | USB-C Power | 5V | Power | PSU → RPi | Powers the Edge AI device |
| **Raspberry Pi 4 (Edge AI Node)** | MicroSD | — | Storage | — | Stores OS + TFLite model |
| **Raspberry Pi 4 (Edge AI Node)** | Python AI Service | bt_inference_service.py | Software | — | Performs TFLite inference |


| **Pump Control (TIP122)** | Base (B) | From Arduino D8 through 1kΩ resistor | Digital | MCU → TIP122 | Controls pump via transistor switching |
| **Pump Control (TIP122)** | Collector (C) | To Pump (–) terminal | Power Path | Pump → TIP122 | Handles motor negative lead |
| **Pump Control (TIP122)** | Emitter (E) | To GND (shared with Arduino/ESP32/PSU) | Ground | TIP122 → GND | Common ground for transistor |
| **Flyback Diode (1N4007)** | Across Pump Terminals | Cathode (stripe) to Pump (+), Anode to Pump (–) | Protection | Motor → Diode | Suppresses inductive voltage spikes |

| **Peristaltic Pump (12V)** | (+) | From 12V Adapter (+) | Power | PSU → Pump | Main positive supply |
| **Peristaltic Pump (12V)** | (–) | To TIP122 Collector (C) | Power Path | Pump → TIP122 | Switched negative return |

| **Power Supply** | 12V PSU | — | Power | → Pump Circuit | Main pump power |
| **Power Supply** | 5V USB | — | Power | → Arduino + ESP32 | Logic power |
| **Power Supply** | GND | Common | Ground | Shared | Common reference for all modules |

---

## ⚙️ Notes (Updated)

- **All grounds (GND)** from Arduino, ESP32, TIP122 circuit, and the 12V PSU must be connected together.
- UART communication operates at logic levels: Arduino TX → ESP32 RX must be protected using a **1k–2kΩ series resistor**.
- The **TIP122 transistor** controls the pump via low-side switching on the negative lead.
- The pump circuit requires a **flyback diode (1N4007)** across the motor terminals (cathode to +12V).
- The Raspberry Pi runs the `bt_inference_service.py` TinyML inference loop and connects to ESP32 using **Bluetooth SPP (RFCOMM)**.

---

## 📁 File References (Updated)

- `firmware/arduino/main.ino` — Sensor acquisition & actuator control  
- `firmware/esp32_gateway/src/main.cpp` — UART gateway, Bluetooth M2M & ThingSpeak upload  
- `raspberry_pi/bt_inference_service.py` — Edge AI inference service  
- `docs/diagrams/` — Schematic, DFD, and system architecture diagrams
