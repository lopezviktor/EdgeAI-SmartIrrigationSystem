# tools/make_synth_dataset.py
# Generate a synthetic dataset for Smart Irrigation (TinyML baseline)

import csv, random
from datetime import datetime, timedelta

N = 1000  # rows
start = datetime.now() - timedelta(minutes=N * 0.3)  # ~18s spacing


def clamp(x, a, b):
    return max(a, min(b, x))


rows = []
t = start
for i in range(N):
    # Base distributions resembling your ESP32 simulation ranges
    soil1 = random.randint(400, 750)
    soil2 = random.randint(400, 750)
    temp = round(random.uniform(18.0, 30.0), 2)
    hum = round(random.uniform(35.0, 75.0), 2)
    light = random.randint(150, 900)

    # Simple heuristic label (you can adjust later to match your TinyML target)
    # Irrigate when soil is "dry-ish" (high raw value on your current mapping)
    decision = 1 if (soil1 > 600 or soil2 > 600) else 0

    rows.append(
        {
            "created_at": t.isoformat(),
            "soil1": soil1,
            "soil2": soil2,
            "temp_c": temp,
            "humidity": hum,
            "light": light,
            "decision": decision,
        }
    )
    t += timedelta(seconds=18)  # ~ThingSpeak cadence

# Write CSV
out_path = "firmware/data/dataset_synth.csv"
with open(out_path, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=rows[0].keys())
    w.writeheader()
    w.writerows(rows)

print(f"✔ Wrote {len(rows)} rows to {out_path}")
