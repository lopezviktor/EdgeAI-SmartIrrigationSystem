"""
Local sanity check for TensorFlow Lite inference on Raspberry Pi (or Mac)
before wiring UART communication.
It loads `model/model.tflite` and, if present, `model/scaler.joblib`
to replicate the training preprocessing steps.

Inputs are expected in the order (6 features):
[soil1, soil2, soil_max, temp_c, humidity, light]
Adjust the sample values below to realistic ranges from your sensors.
"""

import os
import sys
import numpy as np

# Prefer the lightweight runtime on Raspberry Pi.
try:
    import tflite_runtime.interpreter as tflite  # for Raspberry Pi (ARM)
except Exception:
    # Desktop fallbacks (different TF layouts)
    try:
        from tensorflow.lite import Interpreter as _TFInterpreter          # TF 2.14+
    except Exception:
        from tensorflow.lite.python.interpreter import Interpreter as _TFInterpreter  # older paths
    class tflite:  # shim to keep API identical
        Interpreter = _TFInterpreter

_SCALER_PATH = "model/scaler.joblib"
_SCALER = None
if os.path.exists(_SCALER_PATH):
    try:
        from joblib import load
        _SCALER = load(_SCALER_PATH)
        print("[INFO] Using scaler found at model/scaler.joblib")
    except Exception as e:
        print(f"[WARN] Could not load scaler at {_SCALER_PATH}: {e}", file=sys.stderr)

_MODEL_PATH = "model/model.tflite"


def load_interpreter(model_path: str):
    """Load and allocate tensors for a TFLite interpreter."""
    print(f"[INFO] Loading TFLite model: {model_path}")
    interpreter = tflite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()
    return interpreter


def preprocess(sample: np.ndarray) -> np.ndarray:
    """
    Apply the same preprocessing used at training time.
    If a scaler is available, transform the input; otherwise, pass-through.
    """
    if _SCALER is not None:
        return _SCALER.transform(sample).astype(np.float32)
    return sample.astype(np.float32)


def predict(sample: np.ndarray) -> np.ndarray:
    """
    Run a forward pass and return the raw model output.
    For a binary classifier, this is usually a single sigmoid probability.
    """
    interpreter = load_interpreter(_MODEL_PATH)
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    # Sanity print
    print(f"[INFO] Model input details: {input_details}")
    print(f"[INFO] Model output details: {output_details}")

    x = preprocess(sample)
    interpreter.set_tensor(input_details[0]["index"], x)
    interpreter.invoke()
    y = interpreter.get_tensor(output_details[0]["index"])
    return y


def decision_from_output(y: np.ndarray, threshold: float = 0.5) -> str:
    """
    Map raw output to a human-readable decision.
    Assumes sigmoid output for binary classification.
    """
    score = float(np.squeeze(y))
    return "WATER_ON" if score >= threshold else "WATER_OFF"


if __name__ == "__main__":
    # Example raw readings observed by your sensors
    soil1 = 45.0       # soil moisture sensor 1 (%)
    soil2 = 52.0       # soil moisture sensor 2 (%)
    temp_c = 23.5      # ambient temperature (°C)
    humidity = 48.0    # relative humidity (%)
    light = 300.0      # light (lux, or your chosen unit)

    # Engineered feature used during training
    soil_max = max(soil1, soil2)

    # Order must match training: ["soil1","soil2","soil_max","temp_c","humidity","light"]
    sample_input = np.array([[soil1, soil2, soil_max, temp_c, humidity, light]], dtype=np.float32)

    print(f"[INFO] Sample input (raw): {sample_input}")

    try:
        y_pred = predict(sample_input)
        decision = decision_from_output(y_pred, threshold=0.5)
        print(f"[RESULT] Model raw output: {y_pred}")
        print(f"[RESULT] Decision: {decision}")
    except Exception as e:
        print(f"[ERROR] Inference failed: {e}", file=sys.stderr)
        sys.exit(1)