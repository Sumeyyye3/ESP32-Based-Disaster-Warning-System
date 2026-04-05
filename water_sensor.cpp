#include "WaterSensor.h"

void setupWaterSensor() {
    pinMode(WATER_PIN, INPUT);
}

int readWaterLevel() {
    return analogRead(WATER_PIN);
}

String getWaterStatus(int rawValue) {
    if (rawValue < 1000) return "Kuru";
    if (rawValue < 2000) return "Az Islak";
    if (rawValue < 3500) return "Orta";
    return "Cok Islak / Suda";
}

int getFrequencyByStatus(int rawValue) {
    if (rawValue < 1000) return 0;
    if (rawValue < 2000) return 800;
    if (rawValue < 3500) return 1200;
    return 2000;
}