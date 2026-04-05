/* * 
 * Açıklama: Buzzer (Alarm) sistemi için donanım pin tanımlamaları 
 * ve fonksiyon prototiplerini içeren başlık dosyası.
 */

#ifndef ALARM_SYSTEM_H    // Makro Koruması: Bu dosyanın birden fazla kez 
#define ALARM_SYSTEM_H    // include edilip hata vermesini engeller (Include Guard).

#include <Arduino.h>      // Arduino temel kütüphanesini içeri aktar (Pin ve tip tanımları için).

// --- Donanım ve Parametre Tanımlamaları ---
#define BUZZER_PIN 25     // Buzzer'ın fiziksel olarak bağlı olduğu ESP32 pini (GPIO 25).
#define BUZZER_CHANNEL 0  // ESP32'nin LEDC (PWM) birimi için kullanılacak kanal (0-15 arası).
#define BUZZER_RES 8      // PWM sinyalinin çözünürlüğü (8 bit = 0-255 arası hassasiyet).

// --- Fonksiyon Prototipleri ---
// (Bu fonksiyonların asıl gövdesi AlarmSystem.cpp dosyasındadır)

// Buzzer ve PWM kanalının başlangıç ayarlarını yapar.
void setupBuzzer();

// Belirtilen frekansta (Hz cinsinden) ses üretilmesini sağlar.
void buzzerOn(int freq);

// Sesi tamamen kapatır.
void buzzerOff();

#endif // ALARM_SYSTEM_H sona erdi.