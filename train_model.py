"""
Predictive Maintenance - ML Fault Detection Model
---------------------------------------------------
Trains a Random Forest classifier on sensor data
to predict machine faults before they happen.

Run: python train_model.py
"""

import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, confusion_matrix
from sklearn.preprocessing import LabelEncoder
import joblib
import os

# ─── Generate Sample Training Data ───────────────────────────
def generate_data(n=2000):
    np.random.seed(42)
    data = {
        "temperature": np.random.normal(55, 15, n),
        "vibration":   np.random.normal(1.2, 0.6, n),
        "current":     np.random.normal(5.0, 2.0, n),
        "noise":       np.random.randint(200, 900, n),
        "humidity":    np.random.normal(50, 10, n),
    }
    df = pd.DataFrame(data)

    # Label faults based on thresholds
    def label(row):
        if row["temperature"] > 80:   return "OVERHEAT"
        if row["vibration"]   > 2.5:  return "VIBRATION"
        if row["current"]     > 10.0: return "OVERCURRENT"
        if row["noise"]       > 800:  return "NOISE"
        return "NORMAL"

    df["fault"] = df.apply(label, axis=1)
    return df

# ─── Train Model ─────────────────────────────────────────────
def train():
    print("[INFO] Generating training data...")
    df = generate_data(2000)
    print(df["fault"].value_counts())

    X = df[["temperature", "vibration", "current", "noise", "humidity"]]
    y = df["fault"]

    le = LabelEncoder()
    y_encoded = le.fit_transform(y)

    X_train, X_test, y_train, y_test = train_test_split(
        X, y_encoded, test_size=0.2, random_state=42
    )

    print("\n[INFO] Training Random Forest model...")
    model = RandomForestClassifier(n_estimators=100, random_state=42)
    model.fit(X_train, y_train)

    y_pred = model.predict(X_test)
    print("\n[RESULTS] Classification Report:")
    print(classification_report(y_test, y_pred, target_names=le.classes_))

    # Save model and encoder
    os.makedirs("models", exist_ok=True)
    joblib.dump(model, "models/fault_model.pkl")
    joblib.dump(le,    "models/label_encoder.pkl")
    print("\n[INFO] Model saved to models/fault_model.pkl")

if __name__ == "__main__":
    train()
