"""
Predictive Maintenance - Flask Server + ML Prediction
-------------------------------------------------------
Receives sensor data from ESP32 via HTTP POST,
runs ML model prediction, and stores results.

Run: python server.py
Then open: http://localhost:5000
"""

from flask import Flask, request, jsonify, render_template_string
import joblib
import numpy as np
import pandas as pd
from datetime import datetime
import json

app = Flask(__name__)

# ─── Load ML Model ───────────────────────────────────────────
try:
    model   = joblib.load("models/fault_model.pkl")
    encoder = joblib.load("models/label_encoder.pkl")
    print("[INFO] ML model loaded successfully.")
except Exception as e:
    model, encoder = None, None
    print(f"[WARNING] Model not found. Run train_model.py first. ({e})")

# ─── In-memory data store ─────────────────────────────────────
readings = []

# ─── HTML Dashboard ──────────────────────────────────────────
DASHBOARD_HTML = """
<!DOCTYPE html>
<html>
<head>
  <title>Predictive Maintenance Dashboard</title>
  <meta http-equiv="refresh" content="3">
  <style>
    body { font-family: Arial, sans-serif; background: #f4f6f8; margin: 0; padding: 20px; }
    h1   { color: #2c3e50; }
    .cards { display: flex; gap: 16px; flex-wrap: wrap; margin: 20px 0; }
    .card { background: white; border-radius: 10px; padding: 20px; min-width: 140px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.08); text-align: center; }
    .card h3 { margin: 0 0 8px; font-size: 13px; color: #888; }
    .card p  { margin: 0; font-size: 26px; font-weight: bold; color: #2c3e50; }
    .status  { padding: 12px 20px; border-radius: 8px; font-size: 18px;
               font-weight: bold; display: inline-block; margin: 10px 0; }
    .NORMAL      { background: #d5f5e3; color: #1e8449; }
    .OVERHEAT    { background: #fdecea; color: #c0392b; }
    .VIBRATION   { background: #fef9e7; color: #d68910; }
    .OVERCURRENT { background: #fdecea; color: #c0392b; }
    .NOISE       { background: #eaf4fd; color: #1a5276; }
    table { width: 100%; background: white; border-collapse: collapse;
            border-radius: 10px; overflow: hidden; box-shadow: 0 2px 8px rgba(0,0,0,0.08); }
    th { background: #2c3e50; color: white; padding: 10px; font-size: 13px; }
    td { padding: 10px; font-size: 13px; border-bottom: 1px solid #eee; text-align: center; }
  </style>
</head>
<body>
  <h1>🏭 Predictive Maintenance Dashboard</h1>
  {% if readings %}
    {% set last = readings[-1] %}
    <div class="cards">
      <div class="card"><h3>Temperature</h3><p>{{ last.temperature }}°C</p></div>
      <div class="card"><h3>Vibration</h3><p>{{ last.vibration }} g</p></div>
      <div class="card"><h3>Current</h3><p>{{ last.current }} A</p></div>
      <div class="card"><h3>Noise</h3><p>{{ last.noise }}</p></div>
      <div class="card"><h3>Humidity</h3><p>{{ last.humidity }}%</p></div>
    </div>
    <p>Machine Status:</p>
    <div class="status {{ last.predicted_fault }}">{{ last.predicted_fault }}</div>
    <h3>Recent Readings (Last 20)</h3>
    <table>
      <tr><th>Time</th><th>Temp °C</th><th>Vibration g</th><th>Current A</th><th>Noise</th><th>Status</th></tr>
      {% for r in readings[-20:]|reverse %}
      <tr>
        <td>{{ r.timestamp }}</td>
        <td>{{ r.temperature }}</td>
        <td>{{ r.vibration }}</td>
        <td>{{ r.current }}</td>
        <td>{{ r.noise }}</td>
        <td><span class="status {{ r.predicted_fault }}" style="padding:4px 10px;font-size:12px">{{ r.predicted_fault }}</span></td>
      </tr>
      {% endfor %}
    </table>
  {% else %}
    <p>Waiting for sensor data from ESP32...</p>
  {% endif %}
</body>
</html>
"""

# ─── Routes ──────────────────────────────────────────────────
@app.route("/")
def dashboard():
    return render_template_string(DASHBOARD_HTML, readings=readings)

@app.route("/data", methods=["POST"])
def receive_data():
    data = request.json
    if not data:
        return jsonify({"error": "No data received"}), 400

    # ML Prediction
    predicted_fault = data.get("fault", "UNKNOWN")
    if model and encoder:
        features = np.array([[
            data.get("temperature", 0),
            data.get("vibration",   0),
            data.get("current",     0),
            data.get("noise",       0),
            data.get("humidity",    50),
        ]])
        pred_idx        = model.predict(features)[0]
        predicted_fault = encoder.inverse_transform([pred_idx])[0]

    data["predicted_fault"] = predicted_fault
    data["timestamp"]       = datetime.now().strftime("%H:%M:%S")
    readings.append(data)

    # Keep only last 100 readings in memory
    if len(readings) > 100:
        readings.pop(0)

    print(f"[DATA] {data['timestamp']} | Fault: {predicted_fault}")
    return jsonify({"status": "ok", "predicted_fault": predicted_fault})

@app.route("/api/latest")
def api_latest():
    if readings:
        return jsonify(readings[-1])
    return jsonify({"message": "No data yet"})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
