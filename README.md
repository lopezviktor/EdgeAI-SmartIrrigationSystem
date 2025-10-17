# 🌱 Smart Irrigation System – IoT + Edge AI (TinyML)

## 1. Project Overview

In regions affected by water scarcity, traditional irrigation systems often rely on fixed schedules or manual control, leading to significant water waste.  
This project proposes a **low-cost, intelligent irrigation solution** that combines **IoT sensing**, **Edge AI inference**, and **cloud-based monitoring** to achieve efficient and autonomous water management.

The system integrates **Arduino UNO R4 WiFi**, **ESP32 Gateway**, and **ThingSpeak Cloud** to make real-time, data-driven irrigation decisions directly at the edge.

---

## 2. Objectives

- Develop a **reliable IoT-based irrigation system** capable of autonomous operation.  
- Implement **Edge AI (TinyML)** to perform on-device inference (“water” / “no water”) without cloud dependency.  
- Enable **real-time data transmission and visualization** through **ThingSpeak Cloud**.  
- Demonstrate the benefits of combining **IoT, Edge AI, and Cloud** for sustainable resource management.

---

## 3. System Architecture

> The full circuit simulation and schematic are available in `/hardware/tinkercad/`.  
> Detailed architecture diagrams are stored in `/docs/diagrams/`.

### 3.1 System Context Diagram
<p align="center">
  <img src="docs/diagrams/system_context_diagram.drawio.png" alt="System Context Diagram" width="65%">
</p>

### 3.2 Data Flow Diagram (M2M)
<p align="center">
  <img src="docs/diagrams/data_flow_diagram.drawio.png" alt="Data Flow Diagram" width="70%">
</p>

### 3.3 Components

| Category | Component | Description |
|-----------|------------|-------------|
| **Microcontrollers** | Arduino UNO R4 WiFi, ESP32 | Edge processing and communication gateway |
| **Sensors** | 2× Soil Moisture Sensors, DHT22, LDR | Environmental and soil condition sensing |
| **Actuator** | Peristaltic Water Pump | Controls irrigation based on AI decision |
| **Power Supply** | 5V / 12V DC Adapter | Provides power to sensors, controllers, and pump |
| **Connectivity** | UART (M2M), WiFi (Cloud) | Local serial link between Arduino UNO R4 WiFi and ESP32; WiFi uplink to ThingSpeak Cloud |
| **Cloud Platform** | ThingSpeak | IoT data analytics, visualization, and dashboards |

---

## 4. Data Flow Description

1. Environmental sensors collect **soil moisture**, **temperature**, and **light intensity** data.  
2. The **Arduino UNO R4 WiFi** preprocesses and performs **TinyML inference**, classifying the state as “water” or “no water.”  
3. The decision and raw sensor readings are transmitted via **UART (M2M)** to the **ESP32 gateway**.  
4. The **ESP32** sends the data to **ThingSpeak Cloud** using HTTPS.  
5. ThingSpeak visualizes the data and AI predictions in real time through its dashboard interface.  
6. Optional: Additional cloud analytics and efficiency metrics can be implemented.

---

## 5. Edge AI (TinyML)

- The model was trained on a **custom dataset** of environmental parameters stored in `/data/dataset.csv`.  
- **Classification goal:**  
  - `0 → No Water`  
  - `1 → Water`  
- **Model pipeline:**  
  Trained using TensorFlow → converted to TensorFlow Lite → quantized for **TensorFlow Lite Micro** → deployed on Arduino UNO R4 WiFi.  
- The inference is executed locally on-device, ensuring **low latency**, **offline operation**, and **data privacy**.

---

## 6. Cloud Platform (ThingSpeak)

- **Platform:** MATLAB ThingSpeak IoT Analytics  
- **Channel Fields:**
  - `field1` → Soil Moisture Sensor 1  
  - `field2` → Soil Moisture Sensor 2  
  - `field3` → Air Temperature  
  - `field4` → Light Intensity (LDR)  
  - `field5` → TinyML Prediction (0 / 1)
- Provides real-time **graphical visualization** and **data logging**.  
- Dashboard screenshots and analysis results are stored in `/docs/media/`.

---

## 7. Firmware Structure

### 7.1 Arduino UNO R4 WiFi (Edge Node)
**Path:** `/firmware/arduino/`

Responsibilities:
- Acquire sensor readings.  
- Execute TinyML inference.  
- Transmit data and inference results to ESP32 via UART.

### 7.2 ESP32 (Communication Gateway)
**Path:** `/firmware/esp32/`

Responsibilities:
- Receive and parse UART packets.  
- Manage WiFi connection and HTTPS POST requests.  
- Upload telemetry to ThingSpeak Cloud.

---

## 8. System Design (Tinkercad Simulation)

The electronic circuit was simulated using **Tinkercad** for early validation of sensor readings and pump control.  
Components include:
- Two soil moisture sensors (analog inputs).  
- DHT22 for temperature and humidity.  
- LDR for light intensity.  
- TIP120 transistor driver for the peristaltic pump.  

The public simulation link and configuration details are provided in `/hardware/tinkercad/README.md`.

---

## 9. Data Collection and Model Evaluation

- Data recorded under varying environmental conditions.  
- Stored in `/data/dataset.csv`.  
- Used for TinyML model training and validation.  
- Evaluation metrics include **accuracy**, **precision**, **latency**, and **memory footprint**.  
- Graphical summaries and logs are archived in `/docs/media/`.

---

## 10. Dashboard and Visualization

The **ThingSpeak Dashboard** provides real-time insights into environmental parameters and irrigation actions.  
Displayed metrics include:
- Soil moisture (2 channels)  
- Temperature  
- Light intensity  
- Irrigation decision (binary)  
- Calculated water efficiency metric  

---

## 11. Project Roadmap

| Week | Focus | Deliverables |
|------|--------|--------------|
| 1 | Architecture & Planning | Problem statement, diagrams, GitHub repository |
| 2–3 | Hardware Setup | Sensor validation, UART communication |
| 4–5 | Data Collection & Model Training | Dataset creation, TinyML model |
| 6 | Cloud Integration | ThingSpeak dashboard setup |
| 7 | Testing & Validation | End-to-end data flow testing |
| 8–9 | Report Writing | Academic report draft |
| 10 | Final Presentation | Demonstration video and project defense |

---

## 12. Tools and Technologies

- **Arduino IDE / PlatformIO** — firmware development  
- **TensorFlow Lite Micro** — TinyML model deployment  
- **ThingSpeak IoT Analytics** — cloud storage and visualization  
- **Tinkercad / Draw.io** — circuit and diagram design  
- **GitHub** — version control and documentation  
- **Python** — data preprocessing and model training  

---

## 13. Security and Ethics

- No personal or sensitive data are collected.  
- Implements **data minimization** and **privacy-by-design** principles.  
- Aligns with **UN Sustainable Development Goal 6 (Clean Water and Sanitation)** by promoting responsible water use through automated control.  
- Follows **Responsible IoT design** principles ensuring transparency, accessibility, and scalability.

---

## 14. License

Licensed under the **MIT License**.  
This project may be reused for educational or research purposes with appropriate citation.

---

## 15. Academic Context

**Institution:** York St John University  
**Module:** *The Internet of Things (Level 6)*  
**Project Type:** Individual IoT + Edge AI Artefact + Technical Report  
**Submission Deadline:** 16 January 2026  
