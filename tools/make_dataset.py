#!/usr/bin/env python3
import glob
import os
import pandas as pd

RAW_DIR = os.path.join("tools", "dataset", "raw")
OUT_DIR = os.path.join("tools", "dataset", "processed")
OUT_FILE = os.path.join(OUT_DIR, "dataset_base.csv")

COLUMN_MAP = {
    "created_at": "timestamp",
    "entry_id": "entry_id",
    "field1": "soil1",
    "field2": "soil2",
    "field3": "temperature",
    "field4": "humidity",
    "field5": "light",
    "field6": "decision",
}

KEEP_COLS = ["timestamp", "entry_id", "soil1", "soil2", "temperature", "humidity", "light", "decision"]

def load_raw_files():
    files = sorted(glob.glob(os.path.join(RAW_DIR, "*.csv")))
    if not files:
        raise FileNotFoundError(f"No CSV files found in {RAW_DIR}")
    dfs = []
    for f in files:
        df = pd.read_csv(f)
        dfs.append(df)
    return pd.concat(dfs, ignore_index=True)

def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    df = load_raw_files()

    # Rename columns to stable names
    df = df.rename(columns=COLUMN_MAP)

    # Keep only relevant columns (ignore latitude/longitude/elevation/status)
    missing = [c for c in KEEP_COLS if c not in df.columns]
    if missing:
        raise ValueError(f"Missing expected columns: {missing}. Found: {list(df.columns)}")

    df = df[KEEP_COLS].copy()

    # Parse timestamp
    df["timestamp"] = pd.to_datetime(df["timestamp"], utc=True, errors="coerce")

    # Convert numeric columns
    for c in ["soil1", "soil2", "temperature", "humidity", "light", "decision"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")

    # Drop invalid rows
    df = df.dropna(subset=["timestamp", "soil1", "soil2", "temperature", "humidity", "light"])

    # Hard sanity bounds (defendible + conservative)
    df = df[(df["soil1"].between(0, 1023)) & (df["soil2"].between(0, 1023))]
    df = df[(df["humidity"].between(0, 100))]  # DHT humidity range

    # Sort + deduplicate timestamps
    df = df.sort_values("timestamp").drop_duplicates(subset=["timestamp"], keep="last")

    # Feature engineering
    df["soil_avg"] = (df["soil1"] + df["soil2"]) / 2.0
    df["soil_diff"] = (df["soil1"] - df["soil2"]).abs()

    # NOTE: Outlier handling will be added in next step (so we keep this version minimal + reproducible)
    df.to_csv(OUT_FILE, index=False)
    print(f"Saved: {OUT_FILE} | rows={len(df)}")

if __name__ == "__main__":
    main()