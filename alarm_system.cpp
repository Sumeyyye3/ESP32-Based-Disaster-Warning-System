/* * 
 * Açıklama: ESP32'nin LEDC (PWM) birimini kullanarak buzzer üzerinden 
 * farklı frekanslarda ses üretilmesini sağlayan fonksiyonların gövdesi.
 */

#include "AlarmSystem.h"

// Buzzer'ın başlangıç ayarlarını yapan fonksiyon
void setupBuzzer() {
    // LEDC kanalını yapılandır: 
    // BUZZER_CHANNEL: Kullanılacak kanal (0)
    // 2000: Başlangıç frekansı (Hz)
    // BUZZER_RES: Çözünürlük (8 bit)
    ledcSetup(BUZZER_CHANNEL, 2000, BUZZER_RES);
    
    // Yapılandırılan LEDC kanalını fiziksel pine (GPIO 25) bağla
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    
    // --- Başlangıç Testi ---
    // Cihaz ilk açıldığında çalıştığını anlamak için kısa bir ses ver
    ledcWriteTone(BUZZER_CHANNEL, 1000); // 1000 Hz frekansında sesi başlat
    delay(300);                          // 300 milisaniye bekle
    ledcWriteTone(BUZZER_CHANNEL, 0);    // Sesi durdur (0 Hz)
}

// Belirlenen frekansta buzzer'ı sürekli çaldıran fonksiyon
void buzzerOn(int freq) {
    // Parametre olarak gelen 'freq' değerinde kare dalga üretir
    ledcWriteTone(BUZZER_CHANNEL, freq);
}

// Buzzer sesini tamamen kapatan fonksiyon
void buzzerOff() {
    // Frekansı 0 yaparak sesi keser
    ledcWriteTone(BUZZER_CHANNEL, 0);
}