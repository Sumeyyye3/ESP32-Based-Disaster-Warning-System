# ESP32-Based Disaster Warning System

## 📖 Overview
Developed in **C++** by a team of 5, this system is a wireless early warning system that detects natural disasters in real time without requiring an internet connection. Communication between ESP32 modules is handled via the ESP-NOW protocol.

---

## ⚠️ Detected Disasters
| Disaster | Sensor |
|----------|--------|
| Earthquake / Avalanche | MPU6050 (6-Axis Accelerometer & Gyroscope) |
| Fire / Gas Leak | MQ-2 Gas & Smoke Sensor |
| Flood | Water Level Sensor |

---

## 🛠️ Hardware Used
**Control & Communication**
- ESP32 DevKit V1
- ESP-NOW Protocol (No Wi-Fi required)

**Sensors**
- MPU6050 — Earthquake & avalanche detection
- MQ-2 — Fire & gas detection
- Water Level Sensor — Flood detection

**Alert System**
- 0.96" I2C OLED Display
- WS2812B Addressable RGB LED Strip
- 5V Active Buzzer

**Power**
- 18650 Li-ion Battery
- TP4056 Charging Module
- MT3608 Boost Converter

---

## 📡 How It Works
1. Sensor unit continuously monitors environmental data
2. ESP32 processes sensor readings in real time via C++ software
3. If a threat is detected, a wireless alert is sent via ESP-NOW
4. Receiver unit triggers the buzzer, LED strip, and OLED display
5. Visual and audio alerts notify users immediately

---

## ✨ Key Feature
> Unlike similar systems, this project works **completely offline** — no Wi-Fi or internet connection needed. ESP-NOW enables direct device-to-device communication.

---

## 💻 Technologies
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white)

---

## 👥 Team
| Role | Members |
|------|---------|
| Software Development | 2 |
| Hardware & Other | 3 |
| **Total** | **5** |
