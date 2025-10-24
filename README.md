# 🌱 Smart Irrigation System – IoT + Edge AI (TinyML)

## 1. Project Overview

This project implements a **Smart Irrigation System** that leverages **Internet of Things (IoT)** and **Edge Artificial Intelligence (TinyML)** technologies to automate and optimize the watering process in diverse environments — from small private gardens to large agricultural fields.  

The system continuously monitors environmental and soil conditions through low-cost sensors and performs on-device inference using **TinyML models** deployed on an **Arduino UNO R4 WiFi**.  
Based on the prediction, it controls the irrigation mechanism autonomously while transmitting sensor data to the cloud via an **ESP32 Gateway** for visualization and analysis on **ThingSpeak Cloud**.  

By combining **local decision-making (Edge AI)** and **cloud-based analytics**, this architecture enables efficient, scalable, and sustainable water management with minimal human intervention.

---

## 2. Problem Statement

Traditional irrigation systems—whether in large agricultural fields or small private gardens—often rely on manual scheduling or fixed-timer control, leading to inefficient water use and suboptimal plant growth.  
Such approaches fail to adapt to real-time environmental conditions such as soil moisture, temperature, and light intensity, which are critical factors for plant health and sustainable water management.

This project proposes the design and implementation of a **Smart Irrigation System** based on **IoT and Edge Artificial Intelligence (TinyML)**.  
The system monitors environmental variables and autonomously decides the optimal watering action using local inference, without continuous cloud dependence.  
By integrating low-cost sensors, **Edge AI inference** on **Arduino UNO R4 WiFi**, and **cloud-based analytics** through **ThingSpeak**, the system provides a scalable solution applicable to both **small-scale home gardens** and **large agricultural environments**, ensuring **water efficiency and healthy plant growth**.


---

## 3. Objectives

- Develop a **reliable IoT-based irrigation system** capable of autonomous operation.  
- Implement **Edge AI (TinyML)** to perform on-device inference (“water” / “no water”) without cloud dependency.  
- Enable **real-time data transmission and visualization** through **ThingSpeak Cloud**.  
- Demonstrate the benefits of combining **IoT, Edge AI, and Cloud** for sustainable resource management.

---

## 4. System Architecture

> The full circuit simulation and schematic are available in `/hardware/tinkercad/`.  
> Detailed architecture diagrams are stored in `/docs/diagrams/`.

### 4.1 System Context Diagram
<p align="center">
  <img src="docs/diagrams/system_context_diagram.drawio.png" alt="System Context Diagram" width="65%">
</p>

### 4.2 Data Flow Diagram (M2M)
<p align="center">
  <img src="docs/diagrams/data_flow_diagram.drawio.png" alt="Data Flow Diagram" width="70%">
</p>

### 4.3 Hardware Architecture
<p align="center">
  <img src="docs/diagrams/hardware_architecture.drawio.png" alt="Hardware Architecture Diagram" width="70%">
</p>

### 4.4 Components

| Category | Component | Description |
|-----------|------------|-------------|
| **Microcontrollers** | Arduino UNO R4 WiFi, ESP32 | Edge processing and communication gateway |
| **Sensors** | 2× Soil Moisture Sensors, DHT22, LDR | Environmental and soil condition sensing |
| **Actuator** | Peristaltic Water Pump | Controls irrigation based on AI decision |
| **Power Supply** | 5V / 12V DC Adapter | Provides power to sensors, controllers, and pump |
| **Connectivity** | UART (M2M), WiFi (Cloud) | Local serial link between Arduino UNO R4 WiFi and ESP32; WiFi uplink to ThingSpeak Cloud |
| **Cloud Platform** | ThingSpeak | IoT data analytics, visualization, and dashboards |

---

## 5. Data Flow Description

1. Environmental sensors collect **soil moisture**, **temperature**, and **light intensity** data.  
2. The **Arduino UNO R4 WiFi** preprocesses and performs **TinyML inference**, classifying the state as “water” or “no water.”  
3. The decision and raw sensor readings are transmitted via **UART (M2M)** to the **ESP32 gateway**.  
4. The **ESP32** sends the data to **ThingSpeak Cloud** using HTTPS.  
5. ThingSpeak visualizes the data and AI predictions in real time through its dashboard interface.  
6. Optional: Additional cloud analytics and efficiency metrics can be implemented.

---

## 6. Edge AI (TinyML)

- The model was trained on a **custom dataset** of environmental parameters stored in `/data/dataset.csv`.  
- **Classification goal:**  
  - `0 → No Water`  
  - `1 → Water`  
- **Model pipeline:**  
  Trained using TensorFlow → converted to TensorFlow Lite → quantized for **TensorFlow Lite Micro** → deployed on Arduino UNO R4 WiFi.  
- The inference is executed locally on-device, ensuring **low latency**, **offline operation**, and **data privacy**.

---

## 7. Cloud Platform (ThingSpeak)

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

## 8. Firmware Structure

The `firmware` directory contains two independent but connected modules:

### 8.1 Arduino UNO R4 WiFi (Edge Node)
**Path:** `/firmware/arduino_edge/`

Responsibilities:
- Acquire sensor and environmental readings.  
- Execute TinyML inference locally (classify “Water” / “No Water”).  
- Control the peristaltic pump via GPIO output.  
- Transmit sensor values and inference results to the ESP32 gateway via UART.

### 8.2 ESP32 (Communication Gateway)
**Path:** `/firmware/esp32_gateway/`

Responsibilities:
- Receive and parse UART packets from Arduino UNO R4 WiFi.  
- Manage Wi-Fi connectivity and HTTPS communication.  
- Upload telemetry and AI predictions to ThingSpeak Cloud for visualization.

---

## 9. System Design (Tinkercad Simulation)

The electronic circuit was simulated using **Tinkercad** for early validation of sensor readings and pump control.  
Components include:
- Two soil moisture sensors (analog inputs).  
- DHT22 for temperature and humidity.  
- LDR for light intensity.  
- TIP120 transistor driver for the peristaltic pump.  

The public simulation link and configuration details are provided in `/hardware/tinkercad/README.md`.

---

## 10. Data Collection and Model Evaluation

- Data recorded under varying environmental conditions.  
- Stored in `/data/dataset.csv`.  
- Used for TinyML model training and validation.  
- Evaluation metrics include **accuracy**, **precision**, **latency**, and **memory footprint**.  
- Graphical summaries and logs are archived in `/docs/media/`.

---

## 11. Dashboard and Visualization

The **ThingSpeak Dashboard** provides real-time insights into environmental parameters and irrigation actions.  
Displayed metrics include:
- Soil moisture (2 channels)  
- Temperature  
- Light intensity  
- Irrigation decision (binary)  
- Calculated water efficiency metric  

---

## 12. Project Roadmap

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

## 13. Tools and Technologies

- **Arduino IDE / PlatformIO** — firmware development  
- **TensorFlow Lite Micro** — TinyML model deployment  
- **ThingSpeak IoT Analytics** — cloud storage and visualization  
- **Tinkercad / Draw.io** — circuit and diagram design  
- **GitHub** — version control and documentation  
- **Python** — data preprocessing and model training  

---

## 14. Security and Ethics

- No personal or sensitive data are collected.  
- Implements **data minimization** and **privacy-by-design** principles.  
- Aligns with **UN Sustainable Development Goal 6 (Clean Water and Sanitation)** by promoting responsible water use through automated control.  
- Follows **Responsible IoT design** principles ensuring transparency, accessibility, and scalability.

---

## 15. License

Licensed under the **MIT License**.  
This project may be reused for educational or research purposes with appropriate citation.

---

## 16. Academic Context

**Institution:** York St John University  
**Module:** *The Internet of Things (Level 6)*  
**Project Type:** Individual IoT + Edge AI Artefact + Technical Report  
**Submission Deadline:** 16 January 2026  
