/* *
 * Açıklama: Su seviye sensörü (Flood Detection) modülü için donanım pin 
 * tanımlamaları ve fonksiyon prototiplerini içeren başlık dosyası.
 */

#ifndef WATER_SENSOR_H    // Makro Koruması: Bu dosyanın birden fazla kez 
#define WATER_SENSOR_H    // derlenmesini (redefinition error) engelleyen yapı.

#include <Arduino.h>      // Standart Arduino kütüphanesi (int, String tipleri için).

// --- Donanım Tanımlamaları ---
// ESP32'nin ADC (Analog-Dijital Dönüştürücü) özellikli bir pini. 
// GPIO 34, sadece giriş (input-only) olan ve paraziti az olan bir pindir.
#define WATER_PIN 34      

// --- Fonksiyon Prototipleri ---
// (Bu fonksiyonların detaylı işleyişi WaterSensor.cpp içerisindedir)

// Sensörün bağlı olduğu pin ayarlarını (pinMode) yapar.
void setupWaterSensor();

// Sensörden gelen voltajı 0-4095 arasında dijital bir sayıya dönüştürerek döner.
int readWaterLevel();

// Gelen sayısal veriyi ("Kuru", "Orta", "Cok Islak") gibi metinlere çevirir.
String getWaterStatus(int rawValue);

// Su seviyesine göre çalacak alarmın şiddetini (Hz) belirler.
int getFrequencyByStatus(int rawValue);

#endif // WATER_SENSOR_H sona erdi.