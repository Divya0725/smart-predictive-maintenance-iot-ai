# Smart Predictive Maintenance System with Real-Time Fault Detection Using IoT and AI

A smart, affordable real-time machine monitoring system using **IoT sensors**, **ESP32**, and **Machine Learning** to detect faults and predict failures before they cause downtime.

---

## System Architecture

![System Diagram](images/system_diagram.png)

---

## Components Used

| Component | Purpose |
|---|---|
| ESP32 | Main IoT microcontroller with built-in Wi-Fi |
| MPU6050 | Vibration / acceleration sensor |
| DHT22 | Temperature and humidity sensor |
| ACS712 | Current sensor |
| KY-037 | Noise / sound level sensor |
| OLED 128x64 | Live status display |

![Components](images/components.png)

---

## Features

- Monitors vibration, temperature, current, and noise in real time
- AI/ML model (Random Forest) predicts faults before they happen
- Sends live data to Flask web dashboard via Wi-Fi
- OLED display shows machine status on-site
- Fault types detected: OVERHEAT, VIBRATION, OVERCURRENT, NOISE
- Scalable for small and medium industries (Industry 4.0)

---

## Circuit Diagram

![Circuit](images/circuit_diagram.png)

---

## Project Structure

```
predictive-maintenance/
├── sensor_node.ino      # ESP32 Arduino code
├── train_model.py       # ML model training
├── server.py            # Flask server + dashboard
├── requirements.txt     # Python dependencies
└── models/              # Saved ML model files
```

---

## Setup Instructions

### Arduino (ESP32)
1. Install in Arduino IDE:
   - `Adafruit MPU6050`
   - `Adafruit Unified Sensor`
   - `DHT sensor library`
   - `Adafruit SSD1306`
2. Open `sensor_node.ino`
3. Replace `YOUR_WIFI_NAME`, `YOUR_WIFI_PASSWORD`, `YOUR_SERVER_IP`
4. Upload to ESP32

### Python Server
```bash
pip install -r requirements.txt
python train_model.py       # Train ML model first
python server.py            # Start Flask server
```

Open browser: `http://localhost:5000`

---

## Fault Detection Thresholds

| Sensor | Normal | Fault Trigger |
|---|---|---|
| Temperature | < 80°C | OVERHEAT |
| Vibration | < 2.5 g | VIBRATION FAULT |
| Current | < 10 A | OVERCURRENT |
| Noise | < 800 | NOISE FAULT |

---

## Author

**Divya M**  
B.E. Electronics & Communication (Advanced Communication Technology)  
Chennai Institute of Technology  
[GitHub](https://github.com/Divya0725)
