# tools/convert_to_tflite.py
import os
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "2"
os.environ["TF_ENABLE_ONEDNN_OPTS"] = "0"

import tensorflow as tf
from pathlib import Path
import shutil

# Deshabilitar GPU (Metal)
try:
    tf.config.set_visible_devices([], "GPU")
except Exception:
    pass

tf.config.threading.set_intra_op_parallelism_threads(1)
tf.config.threading.set_inter_op_parallelism_threads(1)

project = Path(__file__).resolve().parents[1]
models_dir = project / "models"

keras_model_path = models_dir / "baseline_dense.keras"
saved_model_dir  = models_dir / "baseline_dense_saved"
tflite_path      = models_dir / "baseline_dense.tflite"

print("Project root:", project)
print("Keras model:", keras_model_path)

# Limpiar carpeta SavedModel si existe
if saved_model_dir.exists():
    shutil.rmtree(saved_model_dir)

# 1) Exportar a SavedModel (Keras 3)
model = tf.keras.models.load_model(keras_model_path)
model.export(saved_model_dir)   # <-- clave en Keras 3
print("SavedModel written to:", saved_model_dir)

# 2) Conversión a TFLite desde el SavedModel
converter = tf.lite.TFLiteConverter.from_saved_model(str(saved_model_dir))
converter.optimizations = [tf.lite.Optimize.DEFAULT]
# Si diera guerra, prueba:
# converter.optimizations = []
# converter._experimental_lower_tensor_list_ops = False

tflite_model = converter.convert()
tflite_path.write_bytes(tflite_model)

print("TFLite saved:", tflite_path, f"({tflite_path.stat().st_size/1024:.2f} KB)")
print("Header bytes:", list(tflite_model[:12]))