import os
import json
import math
import joblib
import numpy as np
import pandas as pd

from sklearn.ensemble import RandomForestRegressor
from sklearn.model_selection import LeaveOneOut
from sklearn.metrics import mean_absolute_error, mean_squared_error

# Train a production-ready model (decision-time only features).
# True  -> use only PRE + at_event features (usable on the Raspberry Pi before watering)
# False -> allow PRE + POST features (analysis mode, not usable for real-time decision)
PRODUCTION_MODE = True

# ---- Paths ----
TRAIN_PATH = "data/training/train.csv"
MODEL_PATH = (
    "models/rf_dose_regressor_prod.joblib"
    if PRODUCTION_MODE
    else "models/rf_dose_regressor.joblib"
)
FEATURES_PATH = (
    "models/rf_dose_features_prod.json"
    if PRODUCTION_MODE
    else "models/rf_dose_features.json"
)


# ---- Config ----
RANDOM_STATE = 42


# Since you only allow a fixed set of pump durations, we can evaluate
# both continuous predictions and "snapped" predictions.
ALLOWED_SECONDS = np.array([8.0, 14.0, 18.0, 24.0], dtype=float)


def snap_to_allowed_seconds(y_pred: float, allowed: np.ndarray) -> float:
    """Snap a continuous prediction to the closest allowed pump duration."""
    idx = int(np.argmin(np.abs(allowed - y_pred)))
    return float(allowed[idx])


def load_training_data(path: str) -> pd.DataFrame:
    if not os.path.exists(path):
        raise FileNotFoundError(f"Training file not found: {path}")
    df = pd.read_csv(path)
    if df.empty:
        raise ValueError("train.csv is empty.")
    return df


def split_xy(df: pd.DataFrame):
    if "irrigation_seconds" not in df.columns:
        raise ValueError("Missing target column 'irrigation_seconds' in train.csv")

    drop_cols = []
    # event_timestamp is not a feature
    if "event_timestamp" in df.columns:
        drop_cols.append("event_timestamp")

    y = df["irrigation_seconds"].astype(float).to_numpy()
    X = df.drop(columns=["irrigation_seconds"] + drop_cols, errors="ignore")

    # Keep only numeric columns for RF
    X = X.select_dtypes(include=[np.number])

    # Exclude light-related features for the dose model (known early contamination + not used in this design)
    light_cols = [c for c in X.columns if c == "light" or c.startswith("light_")]
    if light_cols:
        X = X.drop(columns=light_cols, errors="ignore")

    if X.shape[1] == 0:
        raise ValueError("No numeric feature columns found after preprocessing.")
    if len(y) != len(X):
        raise ValueError("X and y have different number of rows.")

    # Production mode: keep only features available BEFORE irrigation (decision time)
    if PRODUCTION_MODE:
        keep_cols = [c for c in X.columns if ("_pre_" in c) or c.endswith("_at_event")]
        X = X[keep_cols].copy()

        if X.shape[1] == 0:
            raise ValueError(
                "PRODUCTION_MODE=True but no decision-time features were found. "
                "Expected columns containing '_pre_' or ending with '_at_event'."
            )

    return X, y


def build_model() -> RandomForestRegressor:
    # Small dataset: keep model simple-ish to reduce overfitting.
    return RandomForestRegressor(
        n_estimators=300,
        random_state=RANDOM_STATE,
        max_depth=6,
        min_samples_leaf=1,
        min_samples_split=2,
        n_jobs=-1,
    )


def loocv_evaluate(X: pd.DataFrame, y: np.ndarray):
    loo = LeaveOneOut()

    y_true = []
    y_pred_cont = []
    y_pred_snap = []

    for train_idx, test_idx in loo.split(X):
        X_train, X_test = X.iloc[train_idx], X.iloc[test_idx]
        y_train, y_test = y[train_idx], y[test_idx]

        model = build_model()
        model.fit(X_train, y_train)

        pred = float(model.predict(X_test)[0])
        pred_snap = snap_to_allowed_seconds(pred, ALLOWED_SECONDS)

        y_true.append(float(y_test[0]))
        y_pred_cont.append(pred)
        y_pred_snap.append(pred_snap)

    y_true = np.array(y_true, dtype=float)
    y_pred_cont = np.array(y_pred_cont, dtype=float)
    y_pred_snap = np.array(y_pred_snap, dtype=float)

    mae_cont = mean_absolute_error(y_true, y_pred_cont)
    rmse_cont = math.sqrt(mean_squared_error(y_true, y_pred_cont))

    mae_snap = mean_absolute_error(y_true, y_pred_snap)
    rmse_snap = math.sqrt(mean_squared_error(y_true, y_pred_snap))

    return {
        "n_samples": int(len(y_true)),
        "mae_continuous": float(mae_cont),
        "rmse_continuous": float(rmse_cont),
        "mae_snapped": float(mae_snap),
        "rmse_snapped": float(rmse_snap),
        "y_true": y_true.tolist(),
        "y_pred_continuous": y_pred_cont.tolist(),
        "y_pred_snapped": y_pred_snap.tolist(),
    }


def train_final_and_save(X: pd.DataFrame, y: np.ndarray):
    model = build_model()
    model.fit(X, y)

    os.makedirs(os.path.dirname(MODEL_PATH), exist_ok=True)
    joblib.dump(model, MODEL_PATH)

    with open(FEATURES_PATH, "w", encoding="utf-8") as f:
        json.dump({"features": list(X.columns)}, f, indent=2)

    return model


def main():
    df = load_training_data(TRAIN_PATH)
    X, y = split_xy(df)

    print("---- Training set ----")
    print(f"Rows: {len(df)}")
    print(f"Features: {X.shape[1]}")
    print("Feature columns:")
    for c in X.columns:
        print(f"  - {c}")

    print("\n---- LOOCV evaluation (small dataset friendly) ----")
    metrics = loocv_evaluate(X, y)
    print(f"Samples: {metrics['n_samples']}")
    print(f"MAE (continuous): {metrics['mae_continuous']:.3f} s")
    print(f"RMSE (continuous): {metrics['rmse_continuous']:.3f} s")
    print(
        f"MAE (snapped to {ALLOWED_SECONDS.tolist()}): {metrics['mae_snapped']:.3f} s"
    )
    print(f"RMSE (snapped): {metrics['rmse_snapped']:.3f} s")

    print("\n---- Train final model on all data & save artefacts ----")
    _ = train_final_and_save(X, y)
    print(f"Saved model: {MODEL_PATH}")
    print(f"Saved feature list: {FEATURES_PATH}")

    # Optional: quick feature importance preview
    try:
        model = joblib.load(MODEL_PATH)
        importances = getattr(model, "feature_importances_", None)
        if importances is not None:
            pairs = sorted(
                zip(X.columns, importances), key=lambda x: x[1], reverse=True
            )
            print("\nTop feature importances:")
            for name, val in pairs[:10]:
                print(f"  {name}: {val:.4f}")
    except Exception as e:
        print(f"(Feature importance skipped: {e})")


if __name__ == "__main__":
    main()
