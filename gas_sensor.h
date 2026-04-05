/* * 
 * Açıklama: Gaz (MQ-2/MQ-5) sensörü modülü için donanım pin tanımlamaları 
 * ve fonksiyon prototiplerini içeren başlık dosyası.
 */

#ifndef GAS_SENSOR_H    // Makro Koruması: Bu dosyanın birden fazla kez 
#define GAS_SENSOR_H    // include edilip derleme hatası vermesini engeller.

#include <Arduino.h>    // Standart Arduino kütüphanesi (pinMode, analogRead vb. için).

// --- Donanım Tanımlamaları ---
// ESP32 üzerinde analog okuma (ADC) yapabilen uygun bir pin. 
// GPIO 35 genellikle sensör verileri için güvenli bir analog pindir.
#define GAS_PIN 35      

// --- Fonksiyon Prototipleri ---
// (Bu fonksiyonların işleyiş kodları GasSensor.cpp dosyasındadır)

// Gaz sensörünün bağlı olduğu pini giriş olarak yapılandırır.
void setupGasSensor();

// Sensörden gelen analog sinyali 0-4095 (12-bit) aralığında okur.
int readGasLevel();

// Okunan ham değerin, belirlenen tehlike eşiğinin üzerinde olup olmadığını denetler.
// Tehlike varsa 'true', yoksa 'false' döndürür.
bool isGasDangerous(int rawValue);

#endif // GAS_SENSOR_H sona erdi.