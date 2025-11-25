import os
import numpy as np
from joblib import load

# Try to use tflite-runtime (typical on Raspberry Pi).
# Fallback to TensorFlow Lite Interpreter if needed.
try:
    import tflite_runtime.interpreter as tflite
    TFLITE_INTERPRETER = tflite.Interpreter
except ImportError:
    from tensorflow.lite import Interpreter as TFLiteInterpreter
    TFLITE_INTERPRETER = TFLiteInterpreter


class EdgeIrrigationModel:
    """
    TinyML model wrapper for the Smart Irrigation System.

    It loads:
      - a MinMaxScaler saved as joblib (trained on 6 features)
      - a TFLite model (binary classifier: 0 = no irrigation, 1 = irrigation)
    """

    def __init__(
        self,
        model_path: str = "model/model.tflite",
        scaler_path: str = "model/scaler.joblib"
    ):
        base_dir = os.path.dirname(os.path.abspath(__file__))

        model_full = os.path.join(base_dir, model_path)
        scaler_full = os.path.join(base_dir, scaler_path)

        print(f"[MODEL] Loading scaler from: {scaler_full}")
        self.scaler = load(scaler_full)

        print(f"[MODEL] Loading TFLite model from: {model_full}")
        self.interpreter = TFLITE_INTERPRETER(model_path=model_full)
        self.interpreter.allocate_tensors()

        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()

        print(f"[MODEL] Input details: {self.input_details}")
        print(f"[MODEL] Output details: {self.output_details}")

    def predict(self, raw_features):
        """
        Run inference on a single sample.

        raw_features: iterable of 6 floats:
          [soil1, soil2, soil_mean, temperature, humidity, light]

        Returns:
          prob: float in [0, 1] (model raw output)
          label: int (0 = WATER_OFF, 1 = WATER_ON)
        """
        x = np.array(raw_features, dtype=np.float32).reshape(1, -1)

        # Scale features using the same MinMaxScaler used in training.
        x_scaled = self.scaler.transform(x)

        # Set input tensor
        self.interpreter.set_tensor(self.input_details[0]["index"], x_scaled)

        # Run inference
        self.interpreter.invoke()

        # Read output tensor
        output_data = self.interpreter.get_tensor(self.output_details[0]["index"])
        prob = float(output_data[0][0])

        # Simple threshold at 0.5 (can be adjusted based on evaluation)
        label = 1 if prob >= 0.5 else 0

        return prob, label
