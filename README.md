# ESP32-Based Disaster Warning System

Wireless disaster early-warning system built on **ESP32** and the **ESP-NOW** peer-to-peer protocol — no internet connection or router required. Detects **earthquakes**, **fires**, **floods**, and **avalanches** in real time and raises immediate local alerts.

---

## Table of Contents
1. [Features](#features)
2. [Architecture](#architecture)
3. [Project Structure](#project-structure)
4. [Hardware](#hardware)
5. [Wiring Guide](#wiring-guide)
6. [Software Setup](#software-setup)
7. [Configuration](#configuration)
8. [How It Works](#how-it-works)
9. [Serial Output Example](#serial-output-example)
10. [License](#license)

---

## Features
| Disaster | Primary Sensor | Threshold |
|---|---|---|
| Earthquake | MPU-6050 accelerometer | `|a| >= 15 m/s²` (brief spike) |
| Avalanche | MPU-6050 accelerometer | `|a| >= 12 m/s²` sustained >= 800 ms |
| Fire | DS18B20 + flame sensor | `T ≥ 60 °C` **or** `flame ADC ≤ 2000` |
| Flood | Resistive water-level sensor | `water ADC ≥ 2500` |

* **No internet / no router** — ESP-NOW operates at the Wi-Fi MAC layer.
* **Multi-node** — one receiver can monitor multiple sender nodes simultaneously.
* Disaster flags are **double-evaluated** (sender + receiver) for redundancy.
* Alerts: dedicated **LEDs** + **buzzer tones** per disaster type.
* All thresholds are tunable in a single header (`shared/disaster_types.h`).

---

## Architecture

```
┌──────────────────────────────┐          ┌──────────────────────────────┐
│       Sender Node            │          │      Receiver (Base Station) │
│  ┌──────────┐  ┌──────────┐  │ ESP-NOW  │  ┌──────────┐  ┌──────────┐ │
│  │ MPU-6050 │  │ DS18B20  │  │ ──────►  │  │  Buzzer  │  │  4 LEDs  │ │
│  └──────────┘  └──────────┘  │ (2.4GHz) │  └──────────┘  └──────────┘ │
│  ┌──────────┐  ┌──────────┐  │          │  ┌──────────────────────────┐│
│  │  Flame   │  │  Water   │  │          │  │  Serial Monitor (alerts) ││
│  │  Sensor  │  │  Level   │  │          │  └──────────────────────────┘│
│  └──────────┘  └──────────┘  │          └──────────────────────────────┘
└──────────────────────────────┘
```

Each **Sender** node reads its sensors every second, packages the readings into a compact `SensorData` struct, and broadcasts it via ESP-NOW.  
The **Receiver** node listens passively, evaluates thresholds independently, and drives alert outputs.

---

## Project Structure

```
ESP32-Based-Disaster-Warning-System/
├── shared/
│   └── disaster_types.h   # SensorData struct, thresholds, disaster flags
├── sender/
│   └── sender.ino         # Sensor node firmware (Arduino / ESP32 core)
├── receiver/
│   └── receiver.ino       # Base-station firmware (Arduino / ESP32 core)
└── README.md
```

---

## Hardware

### Sender Node
| Component | Purpose |
|---|---|
| ESP32 (any variant) | MCU + ESP-NOW radio |
| MPU-6050 | 3-axis accelerometer/gyro → earthquake & avalanche |
| DS18B20 | Waterproof temperature probe → fire detection |
| Analog flame sensor (IR) | Detects open flame → fire detection |
| Resistive water-level sensor | Submersion level → flood detection |
| 4.7 kΩ resistor | DS18B20 pull-up |
| Status LED + 330 Ω resistor | TX indicator (optional) |

### Receiver (Base Station)
| Component | Purpose |
|---|---|
| ESP32 (any variant) | MCU + ESP-NOW radio |
| Passive buzzer | Audio alert |
| 4 × LEDs + 330 Ω resistors | Per-disaster visual alerts |

---

## Wiring Guide

### Sender — MPU-6050 (I²C)
| MPU-6050 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Sender — DS18B20 (OneWire)
| DS18B20 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| DATA | GPIO 4 (+ 4.7 kΩ pull-up to 3.3 V) |

### Sender — Analog Sensors
| Sensor | ESP32 ADC Pin |
|---|---|
| Flame sensor (analogue out) | GPIO 34 |
| Water-level sensor | GPIO 35 |

### Receiver — Alert Outputs
| Component | ESP32 Pin |
|---|---|
| Buzzer (+) | GPIO 25 |
| LED — Earthquake | GPIO 26 |
| LED — Fire | GPIO 27 |
| LED — Flood | GPIO 14 |
| LED — Avalanche | GPIO 12 |

> **Note:** GPIOs 34 and 35 are input-only on ESP32 and have no internal
> pull-up; they are ideal for ADC-only sensor connections.

---

## Software Setup

### 1. Install the Arduino IDE (≥ 2.x) and ESP32 board package
Follow the [Espressif Arduino core installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

### 2. Install required libraries (via Library Manager)
* **Adafruit MPU6050** (`Adafruit_MPU6050`)
* **Adafruit Unified Sensor** (`Adafruit_Sensor`)
* **OneWire** (`OneWire`)
* **DallasTemperature** (`DallasTemperature`)

`esp_now.h` and `WiFi.h` are bundled with the ESP32 Arduino core — no separate installation needed.

### 3. Flash the receiver first
1. Open `receiver/receiver.ino` in the Arduino IDE.
2. Select your ESP32 board and the correct COM port.
3. Upload the sketch.
4. Open the Serial Monitor at **115 200 baud** — the receiver will print its MAC address:
   ```
   [INFO] Receiver MAC: AA:BB:CC:DD:EE:FF
   ```

### 4. Configure the sender with the receiver's MAC address
1. Open `sender/sender.ino`.
2. Replace the placeholder in `RECEIVER_MAC`:
   ```cpp
   static uint8_t RECEIVER_MAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
   ```
3. Upload to the sender ESP32.

### 5. Verify
Open the Serial Monitor for the **receiver** — you should see a new data block every second containing sensor readings and any active disaster flags.

---

## Configuration

All thresholds and timing constants are defined in `shared/disaster_types.h`:

| Constant | Default | Description |
|---|---|---|
| `EARTHQUAKE_ACCEL_THRESHOLD_MS2` | `15.0` | Acceleration magnitude (m/s²) for earthquake |
| `AVALANCHE_ACCEL_THRESHOLD_MS2` | `12.0` | Acceleration magnitude (m/s²) for avalanche |
| `AVALANCHE_MIN_DURATION_MS` | `800` | Min exceedance duration (ms) to classify as avalanche |
| `FIRE_TEMP_THRESHOLD_C` | `60.0` | Temperature (°C) for fire alarm |
| `FIRE_FLAME_ADC_THRESHOLD` | `2000` | ADC reading below which a flame is detected |
| `FLOOD_WATER_ADC_THRESHOLD` | `2500` | ADC reading above which flooding is detected |

Adjust these values to suit your sensor hardware and deployment environment.

---

## How It Works

1. **Sender** reads all sensors every `SEND_INTERVAL_MS` (1 second).
2. It computes the acceleration magnitude `|a| = √(ax²+ay²+az²)` and tracks how long it has been above the avalanche threshold to distinguish a brief earthquake spike from a sustained avalanche slide.
3. Disaster flags are set in a bitmask (`FLAG_EARTHQUAKE | FLAG_FIRE | FLAG_FLOOD | FLAG_AVALANCHE`) and included in the packet.
4. The packet is transmitted to the receiver via ESP-NOW (broadcast by default, or unicast to a specific MAC).
5. **Receiver** independently re-evaluates all thresholds from the raw values in the packet and drives LEDs and buzzer accordingly.
6. Alert outputs remain active for `ALERT_HOLD_MS` (5 seconds) after the last triggering packet.

---

## Serial Output Example

```
[ESP-NOW] Packet from AA:BB:CC:DD:EE:FF
──────────────────────────────────────────
 Node     : 1 (SensorNode-1)
 Uptime   : 42 s
 Accel    : X=0.12  Y=-0.05  Z=9.78  |a|=9.78 m/s²
 Accel exceed duration: 0 ms
 Temp     : 24.3 °C
 Flame ADC: 3800
 Water ADC: 210
 Flags (sender): 0x00
 Flags (receiver): 0x00
 Status   : [OK] No disaster detected
```

When a fire is detected:
```
 Flags (receiver): 0x02
 *** ALERT: FIRE DETECTED! ***
```

---

## License

This project is released under the **MIT License**. See [LICENSE](LICENSE) for details.
