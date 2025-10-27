# Project Development Log – Smart Irrigation System (IoT + Edge AI)

## 🧩 Context
York St John University – Module COM6017M: The Internet of Things  
Student: Victor López  
Supervisor: Dr. Aminu Usman  
Credits: 20 (Portfolio Assessment – 2000 words report + artefact)

---

## 🔹 Week 1 – System Setup and Planning
**Date:** 16–22 October 2025  
- Defined project problem: inefficient water use in plant irrigation.  
- Designed IoT architecture: sensors → Arduino UNO (TinyML) → UART → ESP32 → ThingSpeak Cloud → Pump.  
- Created GitHub repo and README (English only).  
- Produced circuit and data flow diagrams using Tinkercad and Draw.io.  
- Installed required hardware libraries and set up PlatformIO for ESP32 gateway.

---

## 🔹 Week 2 – Cloud Integration (ThingSpeak)
**Date:** 23–27 October 2025  
- Implemented UART-ready gateway on ESP32 with HTTP connection to ThingSpeak.  
- Verified Wi-Fi connection and API integration (`HTTP Response: 200`).  
- Generated simulated sensor data locally and uploaded to ThingSpeak (fields 1–6).  
- Achieved successful data visualization on cloud dashboard.  
- Created documentation: `dataset_description.md` and TinyML dataset structure.  
- Left the system running to collect continuous random data (approx. 1,000+ entries per day).  

🧠 **Reflection:**  
The cloud pipeline works reliably. Data transmission frequency limited to 16 s to comply with ThingSpeak free-tier API restrictions.  
Next steps: export dataset, clean data in Python, and prepare TinyML model.

---

## 🧩 Upcoming Tasks
- Export CSV from ThingSpeak (dataset_raw.csv)  
- Clean and normalize data (dataset_clean.csv)  
- Perform exploratory analysis (EDA)  
- Train TinyML binary classifier (“irrigate / not irrigate”)  
- Integrate model with Arduino UNO via TensorFlow Lite

---

## 🗂️ Files Created So Far
| File | Purpose |
|------|----------|
| `tinkercad_circuit_complete.png` | Circuit simulation |
| `hardware_architecture.drawio.jpeg` | Hardware architecture |
| `system_context_diagram.drawio.png` | System context |
| `esp32_gateway/main.cpp` | Cloud gateway firmware |
| `arduino_edge/arduino_edge.ino` | Edge device firmware |
| `docs/dataset_description.md` | Dataset description and preprocessing plan |
| `docs/project_log.md` | Project development record |

---

## 🔹 Week 3 – Dataset Preparation
**Date:** 28–31 October 2025  
- Exported dataset_raw.csv from ThingSpeak.  
- Removed empty rows and outliers.  
- Created dataset_clean.csv.  
- Visualized correlations between soil moisture and irrigation decisions.  