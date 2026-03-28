/**
 * receiver.ino — ESP32 Base Station
 *
 * Receives SensorData packets from one or more sender nodes via ESP-NOW,
 * evaluates disaster thresholds independently, and triggers alerts through:
 *   • Serial output (always)
 *   • Buzzer tone pattern (GPIO 25)
 *   • Individual alert LEDs: Earthquake (GPIO 26), Fire (GPIO 27),
 *                             Flood (GPIO 14), Avalanche (GPIO 12)
 *
 * No internet or router is required — ESP-NOW works in pure Station mode.
 *
 * Dependencies (install via Arduino Library Manager)
 * ───────────────────────────────────────────────────
 *  • esp_now.h / WiFi.h  (bundled with ESP32 Arduino core)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#include "../shared/disaster_types.h"

// ─── Pin assignments ───────────────────────────────────────────────────────
#define PIN_BUZZER          25
#define PIN_LED_EARTHQUAKE  26
#define PIN_LED_FIRE        27
#define PIN_LED_FLOOD       14
#define PIN_LED_AVALANCHE   12
#define PIN_LED_STATUS       2   // built-in LED — blinks on each received packet

// ─── Alert durations ──────────────────────────────────────────────────────
// How long (ms) LEDs and buzzer remain active after a disaster is detected.
#define ALERT_HOLD_MS  5000

// ─── Buzzer tone frequencies (Hz) ────────────────────────────────────────
#define BUZZER_FREQ_EARTHQUAKE  880
#define BUZZER_FREQ_FIRE        1760
#define BUZZER_FREQ_FLOOD       660
#define BUZZER_FREQ_AVALANCHE   1320
#define BUZZER_LEDC_CHANNEL     0
#define BUZZER_LEDC_RESOLUTION  8   // 8-bit duty cycle

// ─── Internal state ───────────────────────────────────────────────────────
struct AlertState {
    uint32_t earthquakeUntil;
    uint32_t fireUntil;
    uint32_t floodUntil;
    uint32_t avalancheUntil;
    uint32_t buzzerUntil;    // buzzer active until this millis() timestamp
    uint32_t buzzerFreqHz;   // frequency currently scheduled for the buzzer
};

static AlertState alerts = {};

// ─── Helpers ──────────────────────────────────────────────────────────────
static const char* disasterName(uint8_t flag)
{
    switch (flag) {
        case FLAG_EARTHQUAKE: return "EARTHQUAKE";
        case FLAG_FIRE:       return "FIRE";
        case FLAG_FLOOD:      return "FLOOD";
        case FLAG_AVALANCHE:  return "AVALANCHE";
        default:              return "UNKNOWN";
    }
}

static void printSensorData(const SensorData &pkt)
{
    Serial.println("──────────────────────────────────────────");
    Serial.printf(" Node     : %d (%s)\n", pkt.nodeId, pkt.location);
    Serial.printf(" Uptime   : %u s\n",    pkt.uptimeSec);
    Serial.printf(" Accel    : X=%.2f  Y=%.2f  Z=%.2f  |a|=%.2f m/s²\n",
                  pkt.accelX, pkt.accelY, pkt.accelZ, pkt.accelMag);
    Serial.printf(" Accel exceed duration: %u ms\n", pkt.accelExceedDurationMs);
    Serial.printf(" Temp     : %.1f °C\n",  pkt.temperature);
    Serial.printf(" Flame ADC: %d\n",       pkt.flameAdc);
    Serial.printf(" Water ADC: %d\n",       pkt.waterAdc);
    Serial.printf(" Flags (sender): 0x%02X\n", pkt.disasterFlags);
}

static void scheduleBuzzer(uint32_t freqHz, uint32_t durationMs)
{
    // Non-blocking: record when the buzzer should stop; the main loop
    // drives the LEDC peripheral so no delay() blocks packet reception.
    alerts.buzzerFreqHz = freqHz;
    alerts.buzzerUntil  = millis() + durationMs;
}

static void evaluateAndAlert(const SensorData &pkt)
{
    uint32_t now      = millis();
    uint8_t  newFlags = FLAG_NONE;

    // ── Fire ──────────────────────────────────────────────────────────────
    if (pkt.temperature >= FIRE_TEMP_THRESHOLD_C ||
        pkt.flameAdc    <= FIRE_FLAME_ADC_THRESHOLD) {
        newFlags |= FLAG_FIRE;
    }

    // ── Flood ─────────────────────────────────────────────────────────────
    if (pkt.waterAdc >= FLOOD_WATER_ADC_THRESHOLD) {
        newFlags |= FLAG_FLOOD;
    }

    // ── Earthquake ────────────────────────────────────────────────────────
    if (pkt.accelMag >= EARTHQUAKE_ACCEL_THRESHOLD_MS2) {
        newFlags |= FLAG_EARTHQUAKE;
    }

    // ── Avalanche ────────────────────────────────────────────────────────
    if (pkt.accelMag    >= AVALANCHE_ACCEL_THRESHOLD_MS2 &&
        pkt.accelExceedDurationMs >= AVALANCHE_MIN_DURATION_MS) {
        newFlags |= FLAG_AVALANCHE;
    }

    // Print computed receiver-side flags
    Serial.printf(" Flags (receiver): 0x%02X\n", newFlags);

    // ── Activate alerts ───────────────────────────────────────────────────
    if (newFlags == FLAG_NONE) {
        Serial.println(" Status   : [OK] No disaster detected");
        return;
    }

    // Process each set flag
    const uint8_t allFlags[] = {
        FLAG_EARTHQUAKE, FLAG_FIRE, FLAG_FLOOD, FLAG_AVALANCHE
    };
    const uint32_t alertFreqs[] = {
        BUZZER_FREQ_EARTHQUAKE, BUZZER_FREQ_FIRE,
        BUZZER_FREQ_FLOOD, BUZZER_FREQ_AVALANCHE
    };
    uint32_t *alertUntil[] = {
        &alerts.earthquakeUntil, &alerts.fireUntil,
        &alerts.floodUntil,      &alerts.avalancheUntil
    };

    for (int i = 0; i < 4; i++) {
        if (newFlags & allFlags[i]) {
            Serial.printf(" *** ALERT: %s DETECTED! ***\n",
                          disasterName(allFlags[i]));
            *alertUntil[i] = now + ALERT_HOLD_MS;
            scheduleBuzzer(alertFreqs[i], 300);
        }
    }
}

static void updateAlertOutputs()
{
    uint32_t now = millis();

    digitalWrite(PIN_LED_EARTHQUAKE, now < alerts.earthquakeUntil ? HIGH : LOW);
    digitalWrite(PIN_LED_FIRE,       now < alerts.fireUntil       ? HIGH : LOW);
    digitalWrite(PIN_LED_FLOOD,      now < alerts.floodUntil      ? HIGH : LOW);
    digitalWrite(PIN_LED_AVALANCHE,  now < alerts.avalancheUntil  ? HIGH : LOW);

    // Non-blocking buzzer: start/stop tone based on scheduled window
    if (now < alerts.buzzerUntil) {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, alerts.buzzerFreqHz);
    } else {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
    }
}

// ─── ESP-NOW receive callback ────────────────────────────────────────────
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len)
{
    // Blink built-in LED to indicate reception
    digitalWrite(PIN_LED_STATUS, HIGH);

    if (len != sizeof(SensorData)) {
        Serial.printf("[WARN] Unexpected packet size %d (expected %d)\n",
                      len, (int)sizeof(SensorData));
        digitalWrite(PIN_LED_STATUS, LOW);
        return;
    }

    SensorData pkt;
    memcpy(&pkt, data, sizeof(SensorData));

    Serial.printf("\n[ESP-NOW] Packet from %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    printSensorData(pkt);
    evaluateAndAlert(pkt);

    digitalWrite(PIN_LED_STATUS, LOW);
}

// ─── Setup ────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);

    // Output pins
    pinMode(PIN_LED_STATUS,     OUTPUT);
    pinMode(PIN_LED_EARTHQUAKE, OUTPUT);
    pinMode(PIN_LED_FIRE,       OUTPUT);
    pinMode(PIN_LED_FLOOD,      OUTPUT);
    pinMode(PIN_LED_AVALANCHE,  OUTPUT);

    // Buzzer via LEDC (PWM)
    ledcSetup(BUZZER_LEDC_CHANNEL, 2000, BUZZER_LEDC_RESOLUTION);
    ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);

    // ESP-NOW — station mode, no actual WiFi connection
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.printf("[INFO] Receiver MAC: %s\n", WiFi.macAddress().c_str());

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERROR] ESP-NOW init failed");
        while (true) { delay(1000); }
    }

    esp_now_register_recv_cb(onDataReceived);

    Serial.println("[OK] ESP32 Disaster Warning Receiver ready");
    Serial.println("     Waiting for sensor data…");
}

// ─── Main loop ────────────────────────────────────────────────────────────
void loop()
{
    // Keep alert LEDs updated (they auto-expire after ALERT_HOLD_MS)
    updateAlertOutputs();
    delay(10);
}
