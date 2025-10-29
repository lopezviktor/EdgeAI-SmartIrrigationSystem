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
**Date: 28 October 2025**
- Create and activated Python virtual environment(.venv) for data analysis.
- Installed ipykernel, pandas, and matplotlib libraries.
- Imported dataset from Thinkspeak (dataset_clean.csv)
- Cleaned column names (field1-field6 -> soil1, soil2, tem_c, humidity, light, decision).
- Removed non-numeric and missing values.
- Performed Exploratory Data Analysis (EDA) in Jupyter Notebook:
    - Generated descriptive statistics (df.describe())
    - Verified balanced binary labels
    - Plotted histograms for all sensor variables.
    - Computed correlation matrix and heatmap between features and decision.
    ![Feature Distributions](figures/eda_feature_distributions.png)
    *Fugure - Feature Distribution of the simulated sensor dataset.*
- Confirmed dataset is clean and structurally ready for TinyML preprocessing.
**Reflection: 
The EDA confirmed that the simulated data follows consistent numeric ranges and a balanced decision distribution.
Although the dataset is syntethic, it sucesfully validates the data pipeline from Arduino -> ESP32 -> ThingSpeak -> Python, enabling the next step: normalization and baseline TinyML model training.

**Date: 29 October 2025**
- Normalized sensor features to a [0, 1] range using 'MinMaxScler' and split the dataset into training (80%) and testin (20%) subsets.
- Trained a baseline **Decision Tree Classifier** (max_depth=5) to simulate TinyML inference behavior.
- Achieved 100% accuracy on the simulated dataset, as the model correctly captured the same logic used for label generation ('irrigate = 1 if soil > 600 or soil2 > 600').
- Visualized the decision structure confirming that 'soil1' and 'soil2' dominate the irrigation rule.
- Exported the trained model and scaler as '.joblib' files, along with metadata for future TensorFlow Lite conversion.

![Decision Tree Baseline](figures/decision_tree_baseline%20.png)
The baseline model validates the entire IoT -> Cloud -> tinyML pipeline.
While the dataset is synthetic, the model perfectly reflects the irrigation logic, confirming data consistency and feature relevance.
This phase completes the data preprocessing and establishes a foundaion for deploying the classifier on Arduino via tensorFlow Lite (TinyML comversion - next phase)
