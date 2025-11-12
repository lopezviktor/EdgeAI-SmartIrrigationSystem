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


# ---- Interpreter cache (add this) ----
import threading

_VERBOSE = False
_INTERP = None
_INPUT_INDEX = None
_OUTPUT_INDEX = None
_INIT_LOCK = threading.Lock()

def set_verbose(flag: bool):
    global _VERBOSE
    _VERBOSE = bool(flag)

def _load_interpreter_once(model_path: str = "model/model.tflite"):
    """Create and allocate a single TFLite interpreter (singleton)."""
    global _INTERP, _INPUT_INDEX, _OUTPUT_INDEX
    if _INTERP is not None:
        return
    with _INIT_LOCK:
        if _INTERP is not None:
            return
        if _VERBOSE:
            print(f"[INFO] Loading TFLite model: {model_path}")
        _INTERP = tflite.Interpreter(model_path=model_path)
        _INTERP.allocate_tensors()
        in_details = _INTERP.get_input_details()
        out_details = _INTERP.get_output_details()
        _INPUT_INDEX = in_details[0]["index"]
        _OUTPUT_INDEX = out_details[0]["index"]

def predict(x_scaled_np):
    """
    x_scaled_np: np.ndarray shape (1, 6) float32 (already scaled).
    return: np.ndarray shape (1, 1) float32
    """
    _load_interpreter_once()
    _INTERP.set_tensor(_INPUT_INDEX, x_scaled_np.astype("float32"))
    _INTERP.invoke()
    y = _INTERP.get_tensor(_OUTPUT_INDEX)
    return y
# -------------------------------------

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
    Run a forward pass using the cached TFLite interpreter.
    - sample: np.ndarray con shape (1, 6) o (6,) en el orden:
      [soil1, soil2, temp_c, humidity, light]
      (si pasas (6,), se remodela a (1, 6))
    - Devuelve: np.ndarray shape (1, 1) float32 (probabilidad / score)
    """
    # Asegura forma (1, 6)
    if sample.ndim == 1:
        sample = sample.reshape(1, -1)

    # 1) Preprocesa (aplica scaler si existe)
    x = preprocess(sample).astype(np.float32)

    # 2) Carga intérprete una sola vez (cache)
    _load_interpreter_once(_MODEL_PATH)

    # 3) Inferencia
    _INTERP.set_tensor(_INPUT_INDEX, x)
    _INTERP.invoke()
    y = _INTERP.get_tensor(_OUTPUT_INDEX)
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
