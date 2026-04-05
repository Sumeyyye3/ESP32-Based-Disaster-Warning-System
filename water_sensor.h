#ifndef WATER_SENSOR_H
#define WATER_SENSOR_H

#include <Arduino.h>

#define WATER_PIN 34

// Fonksiyon prototipleri
void setupWaterSensor();
int readWaterLevel();
String getWaterStatus(int rawValue);
int getFrequencyByStatus(int rawValue);

#endif