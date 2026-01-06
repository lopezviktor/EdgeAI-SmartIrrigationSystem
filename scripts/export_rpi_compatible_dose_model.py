import joblib

SRC_MODEL = "models/rf_dose_regressor_prod.joblib"
SRC_FEATS = "models/rf_dose_features_prod.json"

DST_MODEL = "edge/raspberry_pi/model/dose/rf_dose_regressor_prod.joblib"
DST_FEATS = "edge/raspberry_pi/model/dose/rf_dose_features_prod.json"

model = joblib.load(SRC_MODEL)

# Re-save using a protocol that Python 3.7 can read
joblib.dump(model, DST_MODEL, compress=3, protocol=4)

# Copy the features contract (json) as-is
import shutil

shutil.copyfile(SRC_FEATS, DST_FEATS)

print("OK: exported RPi compatible artifacts")
print(" ->", DST_MODEL)
print(" ->", DST_FEATS)
