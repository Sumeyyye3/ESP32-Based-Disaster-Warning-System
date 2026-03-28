/**
 * sender.ino — ESP32 Sensor Node
 *
 * Reads multiple disaster-detection sensors and broadcasts a SensorData
 * packet to all registered receiver nodes via ESP-NOW every
 * SEND_INTERVAL_MS milliseconds.
 *
 * Sensors wired to this node
 * ──────────────────────────
 *  • MPU-6050  (I²C, SDA=21 SCL=22) — earthquake & avalanche detection
 *  • DS18B20   (OneWire, GPIO 4)     — ambient temperature → fire detection
 *  • Flame sensor (analog, GPIO 34)  — IR flame → fire detection
 *  • Water-level sensor (analog, GPIO 35) — flood detection
 *  • Status LED (GPIO 2, built-in)   — blinks on each transmission
 *
 * Dependencies (install via Arduino Library Manager)
 * ───────────────────────────────────────────────────
 *  • esp_now.h / WiFi.h  (bundled with ESP32 Arduino core)
 *  • Adafruit MPU6050    (Adafruit_MPU6050)
 *  • Adafruit Unified Sensor (Adafruit_Sensor)
 *  • OneWire             (OneWire)
 *  • DallasTemperature   (DallasTemperature)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Include shared definitions (copy disaster_types.h into the sketch folder
// or use a symlink / relative path from the IDE).
#include "../shared/disaster_types.h"

// ─── Node configuration ────────────────────────────────────────────────────
#define NODE_ID       1
#define NODE_LOCATION "SensorNode-1"

// ─── Pin assignments ───────────────────────────────────────────────────────
#define PIN_ONEWIRE    4    // DS18B20 data line
#define PIN_FLAME_ADC 34    // Flame sensor (analogue, input-only GPIO)
#define PIN_WATER_ADC 35    // Water-level sensor (analogue, input-only GPIO)
#define PIN_STATUS_LED 2    // Built-in LED

// ─── Timing ───────────────────────────────────────────────────────────────
#define SEND_INTERVAL_MS 1000   // transmit every 1 second

// ─── Receiver MAC address ─────────────────────────────────────────────────
// Set this to the MAC address of your receiver ESP32.
// Run the receiver sketch and read its MAC from Serial to obtain it.
static uint8_t RECEIVER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // broadcast

// ─── Globals ──────────────────────────────────────────────────────────────
Adafruit_MPU6050   mpu;
OneWire            oneWire(PIN_ONEWIRE);
DallasTemperature  tempSensor(&oneWire);

static uint32_t lastSendMs         = 0;
static uint32_t accelExceedStartMs = 0;   // when the current exceedance began
static bool     accelExceeding     = false;

// ─── ESP-NOW send callback ────────────────────────────────────────────────
void onDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.print("[ESP-NOW] Send to ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? " ✓ OK" : " ✗ FAIL");
}

// ─── Sensor reading helpers ───────────────────────────────────────────────
static float readTemperatureC()
{
    tempSensor.requestTemperatures();
    float t = tempSensor.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C) {
        Serial.println("[ERROR] DS18B20 disconnected — temperature reading invalid");
        return DEVICE_DISCONNECTED_C;   // caller must check before comparing
    }
    return t;
}

static void evaluateDisasterFlags(SensorData &pkt)
{
    pkt.disasterFlags = FLAG_NONE;

    // ── Fire ──────────────────────────────────────────────────────────────
    // DEVICE_DISCONNECTED_C (-127) is safely below the threshold, so a
    // disconnected DS18B20 does not produce a false positive.
    if (pkt.temperature >= FIRE_TEMP_THRESHOLD_C ||
        pkt.flameAdc    <= FIRE_FLAME_ADC_THRESHOLD) {
        pkt.disasterFlags |= FLAG_FIRE;
    }

    // ── Flood ─────────────────────────────────────────────────────────────
    if (pkt.waterAdc >= FLOOD_WATER_ADC_THRESHOLD) {
        pkt.disasterFlags |= FLAG_FLOOD;
    }

    // ── Earthquake vs Avalanche ──────────────────────────────────────────
    // Both require |a| > threshold; avalanche additionally requires the
    // exceedance to persist for at least AVALANCHE_MIN_DURATION_MS.
    if (pkt.accelMag >= EARTHQUAKE_ACCEL_THRESHOLD_MS2) {
        pkt.disasterFlags |= FLAG_EARTHQUAKE;
    }
    if (pkt.accelMag    >= AVALANCHE_ACCEL_THRESHOLD_MS2 &&
        pkt.accelExceedDurationMs >= AVALANCHE_MIN_DURATION_MS) {
        pkt.disasterFlags |= FLAG_AVALANCHE;
    }
}

// ─── Setup ────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);
    pinMode(PIN_STATUS_LED, OUTPUT);

    // ── MPU-6050 ──────────────────────────────────────────────────────────
    Wire.begin();
    if (!mpu.begin()) {
        Serial.println("[ERROR] MPU-6050 not found — check wiring!");
        // Halt with visual indicator
        while (true) {
            digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
            delay(200);
        }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("[OK] MPU-6050 initialised");

    // ── DS18B20 ──────────────────────────────────────────────────────────
    tempSensor.begin();
    Serial.printf("[OK] DS18B20 — %d device(s) found\n",
                  tempSensor.getDeviceCount());

    // ── ESP-NOW ──────────────────────────────────────────────────────────
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW init failed");
        while (true) { delay(1000); }
    }
    esp_now_register_send_cb(onDataSent);

    // Register peer (broadcast or unicast to receiver MAC)
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, RECEIVER_MAC, 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[ERROR] Failed to add ESP-NOW peer");
    }

    Serial.printf("[OK] Sender node %d (\"%s\") ready\n",
                  NODE_ID, NODE_LOCATION);
    Serial.printf("[INFO] My MAC: %s\n", WiFi.macAddress().c_str());
}

// ─── Main loop ────────────────────────────────────────────────────────────
void loop()
{
    uint32_t now = millis();
    if (now - lastSendMs < SEND_INTERVAL_MS) return;
    lastSendMs = now;

    // ── Build packet ──────────────────────────────────────────────────────
    SensorData pkt = {};
    pkt.nodeId     = NODE_ID;
    strncpy(pkt.location, NODE_LOCATION, MAX_LOCATION_LEN - 1);
    pkt.uptimeSec  = now / 1000;

    // MPU-6050
    sensors_event_t accelEvt, gyroEvt, tempEvt;
    mpu.getEvent(&accelEvt, &gyroEvt, &tempEvt);
    pkt.accelX = accelEvt.acceleration.x;
    pkt.accelY = accelEvt.acceleration.y;
    pkt.accelZ = accelEvt.acceleration.z;
    pkt.accelMag = sqrtf(pkt.accelX * pkt.accelX +
                         pkt.accelY * pkt.accelY +
                         pkt.accelZ * pkt.accelZ);

    // Track how long |a| has been above the lower avalanche threshold
    if (pkt.accelMag >= AVALANCHE_ACCEL_THRESHOLD_MS2) {
        if (!accelExceeding) {
            accelExceeding     = true;
            accelExceedStartMs = now;
        }
        pkt.accelExceedDurationMs = now - accelExceedStartMs;
    } else {
        accelExceeding            = false;
        pkt.accelExceedDurationMs = 0;
    }

    // Temperature (DS18B20)
    pkt.temperature = readTemperatureC();

    // Flame sensor (lower ADC value = stronger IR signal = closer flame)
    pkt.flameAdc = (int16_t)analogRead(PIN_FLAME_ADC);

    // Water-level sensor (higher ADC value = higher water level)
    pkt.waterAdc = (int16_t)analogRead(PIN_WATER_ADC);

    // Evaluate disaster flags locally (receiver also evaluates independently)
    evaluateDisasterFlags(pkt);

    // ── Transmit via ESP-NOW ──────────────────────────────────────────────
    esp_err_t result = esp_now_send(RECEIVER_MAC,
                                    (const uint8_t *)&pkt,
                                    sizeof(pkt));
    if (result != ESP_OK) {
        Serial.printf("[ERROR] esp_now_send: %s\n", esp_err_to_name(result));
    }

    // ── Status LED blink ─────────────────────────────────────────────────
    digitalWrite(PIN_STATUS_LED, HIGH);
    delay(50);
    digitalWrite(PIN_STATUS_LED, LOW);

    // ── Debug output ─────────────────────────────────────────────────────
    Serial.printf("[DATA] uptime=%us | accel=(%.2f,%.2f,%.2f) mag=%.2f m/s² "
                  "| temp=%.1f°C | flame=%d | water=%d | flags=0x%02X\n",
                  pkt.uptimeSec,
                  pkt.accelX, pkt.accelY, pkt.accelZ, pkt.accelMag,
                  pkt.temperature, pkt.flameAdc, pkt.waterAdc,
                  pkt.disasterFlags);
}
