import os
import pandas as pd
import numpy as np

# ---- Configuration ----
DATASET_PATH = "tools/dataset/processed/dataset_base.csv"
EVENTS_PATH = "data/labels/irrigation_events.csv"
OUT_PATH = "data/training/train.csv"

SAMPLE_MINUTES = 3
PRE_MINUTES = 30
POST_MINUTES = 240

# Minimum samples required inside each window to accept an event (robustness guard)
MIN_PRE_SAMPLES = 5
MIN_POST_SAMPLES = 20

# ---- Helper functions ----
def safe_mean(x):
    return float(np.nanmean(x)) if len(x) else np.nan


def safe_std(x):
    return float(np.nanstd(x)) if len(x) else np.nan


def safe_min(x):
    return float(np.nanmin(x)) if len(x) else np.nan


def safe_max(x):
    return float(np.nanmax(x)) if len(x) else np.nan


def build_row(pre_df, post_df, event_row):
    # Base feature columns (adjust if your CSV uses different names)
    base_cols = [
        "soil1",
        "soil2",
        "soil_avg",
        "soil_diff",
        "temperature",
        "humidity",
        "light",
    ]

    feats = {}

    # --- Statistics from PRE-irrigation window ---
    for c in base_cols:
        v = pre_df[c].to_numpy()
        feats[f"{c}_pre_mean"] = safe_mean(v)
        feats[f"{c}_pre_std"] = safe_std(v)
        feats[f"{c}_pre_min"] = safe_min(v)
        feats[f"{c}_pre_max"] = safe_max(v)

    # --- Statistics from POST-irrigation window ---
    for c in ["soil_avg", "soil_diff", "soil1", "soil2"]:
        v = post_df[c].to_numpy()
        feats[f"{c}_post_min"] = safe_min(v)
        feats[f"{c}_post_max"] = safe_max(v)
        feats[f"{c}_post_mean"] = safe_mean(v)

    # --- Delta features (system response to irrigation) ---
    pre_soil_avg_mean = feats["soil_avg_pre_mean"]
    post_soil_avg_min = feats["soil_avg_post_min"]
    feats["delta_soil_avg_min_vs_pre_mean"] = post_soil_avg_min - pre_soil_avg_mean

    # time_to_min_soil_avg (minutes from irrigation event to minimum soil_avg in POST window)
    if len(post_df):
        idx_min = int(np.nanargmin(post_df["soil_avg"].to_numpy()))
        t_min = post_df.iloc[idx_min]["timestamp"]
        feats["time_to_min_soil_avg_minutes"] = float((t_min - event_row["timestamp"]).total_seconds() / 60.0)
    else:
        feats["time_to_min_soil_avg_minutes"] = np.nan

    # Context at the irrigation event (last PRE sample)
    last_pre = pre_df.iloc[-1]
    feats["soil_avg_at_event"] = float(last_pre["soil_avg"])
    feats["soil_diff_at_event"] = float(last_pre["soil_diff"])
    feats["temp_at_event"] = float(last_pre["temperature"])
    feats["humidity_at_event"] = float(last_pre["humidity"])
    feats["light_at_event"] = float(last_pre["light"])

    # Regression target
    feats["irrigation_seconds"] = float(event_row["irrigation_seconds"])
    feats["event_timestamp"] = str(event_row["timestamp"])

    return feats


def main():
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)

    df = pd.read_csv(DATASET_PATH)
    events = pd.read_csv(EVENTS_PATH)

    # Parse timestamps while preserving timezone (UTC)
    df["timestamp"] = pd.to_datetime(df["timestamp"], utc=True)
    events["timestamp"] = pd.to_datetime(events["timestamp"], utc=True)
    events["irrigation_seconds"] = pd.to_numeric(events.get("irrigation_seconds"), errors="coerce")

    # Sort data chronologically
    df = df.sort_values("timestamp").reset_index(drop=True)
    events = events.sort_values("timestamp").reset_index(drop=True)

    # Ensure required feature columns exist
    required = {
        "soil1",
        "soil2",
        "soil_avg",
        "soil_diff",
        "temperature",
        "humidity",
        "light",
    }
    missing = required - set(df.columns)
    if missing:
        raise ValueError(f"Missing required columns in dataset_base.csv: {missing}")

    rows = []
    for _, ev in events.iterrows():
        t = ev["timestamp"]

        # Skip events without a regression label (seconds)
        if pd.isna(ev.get("irrigation_seconds")):
            print(f"[SKIP] Event {t} has no irrigation_seconds label.")
            continue

        # Time-based windows to be robust to irregular sampling
        pre_start_time = t - pd.Timedelta(minutes=PRE_MINUTES)
        post_end_time = t + pd.Timedelta(minutes=POST_MINUTES)

        pre_df = df[(df["timestamp"] >= pre_start_time) & (df["timestamp"] < t)].copy()
        post_df = df[(df["timestamp"] >= t) & (df["timestamp"] < post_end_time)].copy()

        if len(pre_df) < MIN_PRE_SAMPLES or len(post_df) < MIN_POST_SAMPLES:
            print(f"[SKIP] Event {t} does not have enough samples in PRE/POST windows (pre={len(pre_df)}, post={len(post_df)}).")
            continue

        row = build_row(pre_df, post_df, ev)
        rows.append(row)

    out = pd.DataFrame(rows)
    out.to_csv(OUT_PATH, index=False)

    print(f"OK -> {OUT_PATH}")
    print(f"Eventos usados: {len(out)} / {len(events)}")
    print("Preview:")
    print(out.head(3).to_string(index=False))


if __name__ == "__main__":
    main()
