# Dataset Description (TinyML - Smart Irrigation System)

## 📊 Source of Data
- **Platform:** ThinkSpeak Cloud
- **Fields used:**
    1. Soil Moisture Sensor 1
    2. Soil Moisture Sensor 2
    3. Temperature (ºC)
    4. Humidity (%)
    5. Light Intensity (LDR)
    6. Decision (0 = No irrigation, 1 = Irrigation)
- **Sampling interval:** ~16 seconds (ThinkSpeak free tier)

---

## ⚙️ Data Collection Process
- Data was collected automatically from the ESP32 gateway.
- For now, simulated data was used to test the pipeline (UART integration pending).
- Once hardware UART is active, real sensor readings will replace the simulated data.
- Data can be exported from ThingSpeak as `.csv` via:
  - *Channel → Export → Download Data → CSV format.*

---

## 🧩 Feature Overview
| Feature | Description | Unit / Type |
|----------|--------------|-------------|
| soil1 | Soil moisture sensor 1 (analog value) | int |
| soil2 | Soil moisture sensor 2 (analog value) | int |
| temp_c | Temperature | °C |
| humidity | Relative humidity | % |
| light | LDR value (0–1023 raw) | int |
| decision | Model output (binary) | 0/1 |

---

## 🧹 Preprocessing Plan
- Handle missing or duplicate samples.
- Normalize all features to [0,1] range.
- Create label column (`label`) for binary classification.
- Split dataset into 80% train / 20% test.
- Save as `dataset_clean.csv` in `/data/`.

---

## 🧠 Future Work
- Replace simulated values with real sensor data (via UART).
- Use this dataset to train the TinyML binary classifier for “irrigate / not irrigate”. 
