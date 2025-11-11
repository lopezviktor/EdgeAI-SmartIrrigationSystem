# Project Development Log – Smart Irrigation System (IoT + Edge AI)

## Context
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

**Reflection:**  
The cloud pipeline works reliably. Data transmission frequency limited to 16 s to comply with ThingSpeak free-tier API restrictions.  
Next steps: export dataset, clean data in Python, and prepare TinyML model.

---

## Upcoming Tasks
- Export CSV from ThingSpeak (dataset_raw.csv)  
- Clean and normalize data (dataset_clean.csv)  
- Perform exploratory analysis (EDA)  
- Train TinyML binary classifier (“irrigate / not irrigate”)  
- Integrate model with Arduino UNO via TensorFlow Lite

---

## Files Created So Far
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

**Date: 28 October 2025**
- Create and activated Python virtual environment(`.venv`) for data analysis.
- Installed ipykernel, pandas, and matplotlib libraries.
- Imported dataset from Thingspeak (dataset_clean.csv)
- Cleaned column names (field1-field6 -> soil1, soil2, temp, humidity, light, decision).
- Removed non-numeric and missing values.
- Performed Exploratory Data Analysis (EDA) in Jupyter Notebook:
    - Generated descriptive statistics (df.describe())
    - Verified balanced binary labels
    - Plotted histograms for all sensor variables.
    - Computed correlation matrix and heatmap between features and decision.
    ![Feature Distributions](figures/eda_feature_distributions.png)
    *Fugure - Feature Distribution of the simulated sensor dataset.*
- The dataset was confirmed to be clear and structurally ready for TinyML preprocessing.
**Reflection: 
The EDA confirmed that the simulated data follows consistent numeric ranges and a balanced decision distribution.
Although the dataset is syntethic, it still successfully validates the complete data pipeline from Arduino -> ESP32 -> ThingSpeak -> Python, enabling the next step of normalization and TinyML training.

**Date: 29 October 2025**
- Normalized sensor features to a [0, 1] range using `MinMaxScler` and split the dataset into training (80%) and testing (20%) subsets.
- Trained a baseline **Decision Tree Classifier** (max_depth=5) to simulate TinyML inference behavior.
- Achieved 100% accuracy on the simulated dataset, as the model correctly captured the same logic used for label generation (`irrigate = 1 if soil1 > 600 or soil2 > 600`).
- Visualized the decision structure confirming that `soil1` and `soil2` dominate the irrigation rule.
- Exported the trained model and scaler as `.joblib` files, along with metadata for future TensorFlow Lite conversion.

![Decision Tree Baseline](figures/decision_tree_baseline%20.png)
The baseline model validates the entire IoT -> Cloud -> TinyML pipeline.
While the dataset is synthetic, the model perfectly reflects the irrigation logic, confirming data consistency and feature relevance.
This phase completes the data preprocessing and establishes a foundation for deploying the classifier on Arduino via TensorFlow Lite (TinyML comversion - next phase)

**Date: 30 October 2025**
- Exported `baseline_decision_tree` rules to C++ format using Python function `emit_rules_as_cpp()`.
- Generated the header file `predict_need_water.h` containing the decision logic (if/else structure).
- Created `scaler.h` implementing Min-Max normalization with `DATA_MIN` and `DATA_MAX` arrays based on dataset statistics.
- Integrated both headers into the Arduino firmware (`arduino_edge.ino`) to enable on-device inference without external ML libraries.
- Successfully compiled and uploaded to Arduino UNO R4 WiFi.
- Verified live sensor readings:
S1:288, S2:231, T:20.2, H:65.2, L:283, PRED:0
confirming that the model runs locally and produces binary irrigation predictions (`0 = no irrigation`, `1 = irrigation`).
- Performed sensor calibration and debugging of the DHT22 module; replaced defective sensor and confirmed stable temperature/humidity readings.
- Added serial telemetry output (`S1,S2,T,H,L,PRED`) for UART communication with the ESP32 gateway.

**Reflection:**  
This week concludes the TinyML and Edge AI phase.  
The decision-tree classifier was successfully deployed on the Arduino UNO R4 and performs local inference in real time.  
The device now operates autonomously—collecting, normalizing, and classifying sensor data without cloud dependency—meeting the Edge AI objectives for latency reduction and offline resilience.  
Next step: integrate UART communication with the ESP32 to upload telemetry and predictions to ThingSpeak Cloud.
This successful implementation on Arduino establishes the baseline for migrating the Edge AI inference to a Raspberry Pi node in the next development phase, where higher computational capacity and logging capabilities will be leveraged.
---
End of Week 3 - TinyML Edge AI successfully running on Arduino UNO R4

## 🔹 Week 4 - Edge AI Deployment and System Integration
**Date:** 1–7 November 2025  
- Installed and configured the Raspberry Pi as the new Edge AI node for running local TinyML inference, replacing the Arduino UNO’s limited computation.  
- Structured the directory `edge/raspberry_pi/` with modular subfolders (`app/`, `config/`, `model/`, `system/`) to follow clean software architecture practices.  
- Migrated the TinyML model (`baseline_dense.tflite`) and scaler (`minmax_scaler_keras.joblib`) into the Raspberry Pi environment.  
- Set up a dedicated Python virtual environment (`.venv-tflite`) and installed required dependencies (`numpy`, `joblib`, `tflite_runtime`, later replaced by `tensorflow-macos` for testing on macOS).  
- Implemented and tested the script `inference_test.py`, capable of loading the TFLite model, applying normalization, and executing local inference to predict the irrigation decision (`WATER_ON` / `WATER_OFF`).  
- Resolved import and compatibility issues with TensorFlow Lite runtime on macOS, ensuring smooth fallback execution.  
- Validated the complete inference process using simulated sensor readings, confirming consistency between the model output and the expected decision logic.  
- Verified correct usage of the final scaler (`minmax_scaler_keras.joblib` – 6-feature pipeline including `soil_max`).  

**Results:**  
- Successful local inference on the Raspberry Pi using the 6-feature input vector `[soil1, soil2, soil_max, temp_c, humidity, light]`.  
- Model prediction verified with stable output:  

[RESULT] Model raw output: [[0.006]]
[RESULT] Decision: WATER_OFF

- This confirms that the TinyML model and scaler are aligned, and the Raspberry Pi can now perform autonomous inference.  

**Reflection:**  
This phase marks the transition from microcontroller-level inference to a more capable Edge AI platform (Raspberry Pi).  
The system now supports faster inference, local processing, and future scalability (e.g., data logging, OTA updates, or advanced models).  
Next steps involve enabling UART communication between the Raspberry Pi and ESP32 for real-time data exchange.

---

## 🔹 Week 5 – UART Integration and Communication Bridge
**Date:** 11–17 November 2025  
- Created new Git branch `feat/uart-pi-bridge` derived from `feat/edge-on-raspberry` to isolate UART-related development.  
- Added new module `edge/raspberry_pi/app/uart_service.py`, implementing a serial communication bridge between the Raspberry Pi and ESP32/Arduino.  
- Defined and documented a custom UART protocol:  
- **Incoming (from ESP32):** `SOIL1=<f>,SOIL2=<f>,TEMP=<f>,HUM=<f>,LIGHT=<f>`  
- **Processing:** The Raspberry Pi computes `soil_max = max(soil1, soil2)` and runs local inference using the 6-feature TinyML model.  
- **Outgoing (to ESP32):** Sends `WATER_ON` or `WATER_OFF` decision via UART response.  
- Implemented modular Python code for serial reading, parsing, inference, and reply transmission.  
- Verified branch and PR workflow: opened **draft pull request** (base: `feat/edge-on-raspberry`) following best Git practices.  
- Planned hardware configuration: enable UART interface on Raspberry Pi (`/dev/serial0`), set baud rate (9600), and prepare TX/RX/GND wiring for upcoming integration test.  

**Reflection:**  
This week introduces the communication layer connecting the Edge AI node (Raspberry Pi) and the IoT gateway (ESP32).  
With the UART bridge in place, the system moves toward real-time interaction—allowing the Pi to classify sensor data locally and transmit decisions for cloud synchronization and actuator control.  
Next week will focus on enabling UART in the Raspberry Pi, conducting loopback tests, and validating end-to-end serial communication between both devices.