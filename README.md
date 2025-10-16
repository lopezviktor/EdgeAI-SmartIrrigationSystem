# 🌱 Smart Irrigation System – IoT + Edge AI (TinyML)

This project implements a **Smart Irrigation System** using **IoT** and **Edge Artificial Intelligence (TinyML)** to optimize water usage in agricultural environments.  
It integrates **Arduino UNO**, **ESP32**, and **ThingSpeak Cloud** to make intelligent decisions about watering needs in real time.

---

## Objectives

- Develop a **reliable IoT-based irrigation system** capable of operating autonomously.
- Use **Edge AI (TinyML)** to decide locally whether to **water or not** without depending on the cloud.
- Enable **data transmission and visualization** through **ThingSpeak Cloud**.
- Demonstrate the benefits of combining **IoT + Edge AI + Cloud** for sustainable resource management.

---

## System Architecture Overview


> Full circuit simulation and schematic are available in `/hardware/tinkercad/`  
> Architecture diagram stored in `/docs/media/architecture.png`

---

## Components

| Category | Component | Description |
|-----------|------------|--------------|
| **Microcontrollers** | Arduino UNO, ESP32 | Main processing and WiFi connectivity |
| **Sensors** | 2× Soil Moisture Sensors, DHT22, LDR | Environmental data acquisition |
| **Actuator** | Peristaltic Water Pump | Executes watering action |
| **Power** | 5V DC Power Supply | Supplies pump and microcontrollers |
| **Connectivity** | UART, WiFi | Local and cloud communication |
| **Cloud** | ThingSpeak | IoT data analytics and dashboard |

---

## Data Flow

1. **Sensors** collect environmental data (soil moisture, temperature, light).  
2. **Arduino UNO** performs **TinyML inference** to predict “Water” or “No Water”.  
3. Prediction and sensor readings are sent via **UART** to **ESP32**.  
4. **ESP32** uploads the data to **ThingSpeak Cloud** via WiFi.  
5. Data is visualized in real time on a **ThingSpeak dashboard**.  
6. Optional: Cloud analytics or alerts (e.g., water efficiency metrics).

---

## Edge AI (TinyML)

- Model trained using a **custom dataset** of environmental conditions (`/data/dataset.csv`).
- Binary classification:  
  - `0 → No Water`  
  - `1 → Water`
- Model trained in **TensorFlow**, converted to **TensorFlow Lite Micro**, and deployed on **Arduino UNO**.
- Inference performed locally on-device → **low latency and improved privacy**.

---

##  Cloud Platform (ThingSpeak)

- Platform: **MATLAB ThingSpeak IoT Analytics**
- Channel Fields:
  - `field1` → Soil Moisture 1  
  - `field2` → Soil Moisture 2  
  - `field3` → Temperature  
  - `field4` → Light Intensity (LDR)  
  - `field5` → TinyML Prediction (0/1)
- Includes real-time **graphs and data visualizations**.
- Dashboard screenshots and analytics results are stored in `/docs/media/`.

---

## Firmware Overview

### Arduino UNO (Edge Device)
Located in: `/firmware/arduino/`

Responsibilities:
- Read sensors  
- Run **TinyML inference** (TensorFlow Lite Micro)  
- Send data and AI results via UART to ESP32  

### ESP32 (Communication Gateway)
Located in: `/firmware/esp32/`

Responsibilities:
- Receive UART data from Arduino  
- Establish WiFi connection  
- Upload sensor readings and prediction results to ThingSpeak Cloud  

---

## System Design (Tinkercad / Simulation)

- The circuit was designed and simulated in **Tinkercad**.  
- Components include:
  - 2× soil sensors connected to analog pins  
  - DHT22 (temperature and humidity)  
  - LDR for light intensity  
  - MOSFET driver for pump activation  
- Simulation link (public or shareable) included in `/hardware/tinkercad/README.md`.

---

## Data Collection and Analysis

- Data collected over multiple environmental conditions.  
- Stored in `/data/dataset.csv`.  
- Used for model training and validation (accuracy, precision, and latency metrics included).  
- Performance logs and evaluation graphs included in `/docs/media/`.

---

## Dashboard Example

> ThingSpeak dashboard visualizes live environmental data and TinyML predictions.  
> Example fields:
> - Moisture levels (two channels)
> - Temperature trend
> - Light intensity
> - Watering decision (binary)
> - Overall water efficiency metric (calculated field)

---

## Project Roadmap (Week 1–10)

| Week | Focus | Deliverables |
|------|--------|--------------|
| 1 | Architecture & planning | Problem statement, architecture diagram, GitHub repo |
| 2–3 | Hardware setup | Sensor validation, UART comms |
| 4–5 | Data collection + TinyML training | Dataset & model inference |
| 6 | Cloud integration | ThingSpeak dashboard |
| 7 | Testing | Validation and logs |
| 8–9 | Report writing | Academic report draft |
| 10 | Presentation | Final video + review |

---

## 🧑‍💻 Technical Tools

- **Arduino IDE / PlatformIO**
- **TensorFlow Lite Micro**
- **ThingSpeak IoT Analytics**
- **Tinkercad / Draw.io** (for circuit and data flow diagrams)
- **GitHub** for version control
- **Python** (for data preprocessing and training)

---

## 🔐 Security and Ethics

- No personal or sensitive data collected.  
- Complies with **data minimization** and **privacy-by-design** principles.  
- Promotes **sustainability and water conservation** through automation.  
- Designed considering **accessibility, inclusion, and low-cost scalability**.

---

## 📄 License

MIT License — see `LICENSE` for details.  
You may reuse this code and documentation for academic or research purposes with proper attribution.

---

## 🏫 Academic Context

York St John University  
**Module:** COM6017M – *The Internet of Things (Level 6)*  
**Project Type:** Individual IoT + Edge AI Artefact + Technical Report  
**Deadline:** 16 January 2026  

---
