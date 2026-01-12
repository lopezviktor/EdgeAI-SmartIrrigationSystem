import json
from dataclasses import dataclass
from typing import Dict, List

import joblib
import numpy as np


ALLOWED_SECONDS = np.array([8.0, 14.0, 18.0, 24.0], dtype=float)


@dataclass
class DosePrediction:
    continuous_seconds: float
    snapped_seconds: float


class DoseRegressor:
    def __init__(self, model_path: str, features_path: str):
        self.model = joblib.load(model_path)

        with open(features_path, "r", encoding="utf-8") as f:
            payload = json.load(f)

        self.feature_names: List[str] = payload["features"]

    @staticmethod
    def snap_to_allowed(y_pred: float) -> float:
        idx = int(np.argmin(np.abs(ALLOWED_SECONDS - y_pred)))
        return float(ALLOWED_SECONDS[idx])

    def predict_from_features(self, features: Dict[str, float]) -> DosePrediction:
        missing = [name for name in self.feature_names if name not in features]
        if missing:
            raise ValueError(f"Missing required features: {missing}")

        row = np.array(
            [[float(features[name]) for name in self.feature_names]], dtype=float
        )

        pred = float(self.model.predict(row)[0])
        snapped = self.snap_to_allowed(pred)

        return DosePrediction(continuous_seconds=pred, snapped_seconds=snapped)
