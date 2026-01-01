import json
import joblib
import numpy as np
import pandas as pd

TRAIN_PATH = "data/training/train.csv"
MODEL_PATH = "models/rf_dose_regressor_prod.joblib"
FEATURES_PATH = "models/rf_dose_features_prod.json"

ALLOWED_SECONDS = np.array([8.0, 14.0, 18.0, 24.0])

def snap_to_allowed_seconds(y_pred: float, allowed: np.ndarray) -> float:
    idx = int(np.argmin(np.abs(allowed - y_pred)))
    return float(allowed[idx])

def main():
    # Load model
    model = joblib.load(MODEL_PATH)

    # Load feature contract
    with open(FEATURES_PATH, "r", encoding="utf-8") as f:
        feature_names = json.load(f)["features"]

    # Load training set (just to get a real example vector)
    df = pd.read_csv(TRAIN_PATH)

    # Build X using the exact feature order expected by the model
    missing = [c for c in feature_names if c not in df.columns]
    if missing:
        raise ValueError(f"train.csv is missing required production features: {missing}")

    sample = df.iloc[0]  # smoke-test with the first sample
    X = pd.DataFrame([[sample[c] for c in feature_names]], columns=feature_names)

    # Predict
    pred_seconds = float(model.predict(X)[0])
    snapped = snap_to_allowed_seconds(pred_seconds, ALLOWED_SECONDS)

    # Ground truth (if present)
    y_true = float(sample["irrigation_seconds"]) if "irrigation_seconds" in df.columns else None

    print("---- Production inference smoke test ----")
    print(f"Model: {MODEL_PATH}")
    print(f"Features contract: {FEATURES_PATH}")
    print(f"Input features count: {len(feature_names)}")
    if y_true is not None:
        print(f"Ground truth (from train.csv): {y_true:.1f} s")
    print(f"Predicted (continuous): {pred_seconds:.3f} s")
    print(f"Predicted (snapped): {snapped:.1f} s")
    print(f"Allowed set: {ALLOWED_SECONDS.tolist()}")

if __name__ == "__main__":
    main()