/**
 * disaster_types.h
 * Shared data structures, constants, and thresholds used by both the
 * sender (sensor node) and receiver (base station) firmware.
 *
 * Hardware targets: ESP32 (Arduino core) + ESP-NOW
 */

#pragma once

#include <stdint.h>

// ─── Node identification ───────────────────────────────────────────────────
#define MAX_LOCATION_LEN 20

// ─── Disaster flags (bitmask carried in SensorData::disasterFlags) ─────────
#define FLAG_NONE       0x00
#define FLAG_EARTHQUAKE 0x01
#define FLAG_FIRE       0x02
#define FLAG_FLOOD      0x04
#define FLAG_AVALANCHE  0x08

// ─── Detection thresholds ──────────────────────────────────────────────────

// Earthquake / Avalanche (MPU-6050, raw ±2 g range → 16384 LSB/g)
// Magnitude of the acceleration vector is computed on the sender.
// Values are in m/s² (after conversion: raw / 16384.0 * 9.81)
#define EARTHQUAKE_ACCEL_THRESHOLD_MS2   15.0f   // sudden spike > ~1.53 g (15.0/9.81)
#define AVALANCHE_ACCEL_THRESHOLD_MS2    12.0f   // sustained tilt/slide > ~1.22 g (12.0/9.81)
// Avalanche is distinguished from earthquake by requiring the event to
// persist for at least AVALANCHE_MIN_DURATION_MS milliseconds.
#define AVALANCHE_MIN_DURATION_MS        800

// Fire (DS18B20 temperature sensor + analog flame sensor)
#define FIRE_TEMP_THRESHOLD_C            60.0f   // °C — ambient smoke/flame heat
#define FIRE_FLAME_ADC_THRESHOLD         2000    // 12-bit ADC; lower = more IR light

// Flood (resistive water-level sensor, 12-bit ADC, 0–4095)
#define FLOOD_WATER_ADC_THRESHOLD        2500    // ~60 % submersion

// ─── Packet structure sent over ESP-NOW ───────────────────────────────────
// Must fit inside the 250-byte ESP-NOW payload limit.
typedef struct __attribute__((packed)) {
    uint8_t  nodeId;                       // unique ID of the sending node
    char     location[MAX_LOCATION_LEN];   // human-readable label, e.g. "RoomA"

    // Raw sensor readings
    float    accelX;        // m/s²
    float    accelY;        // m/s²
    float    accelZ;        // m/s²
    float    accelMag;      // |a| m/s² — pre-computed magnitude
    float    temperature;   // °C
    int16_t  flameAdc;      // 12-bit ADC reading (lower = more flame)
    int16_t  waterAdc;      // 12-bit ADC reading (higher = more water)

    // Duration (ms) of the current accelerometer exceedance — used by
    // receiver to distinguish earthquake spikes from avalanche slides.
    uint32_t accelExceedDurationMs;

    // Bitmask of detected disasters (pre-evaluated on sender for redundancy)
    uint8_t  disasterFlags;

    // Uptime of the sender node in seconds
    uint32_t uptimeSec;
} SensorData;
