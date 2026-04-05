/* * Proje: ESP32 Tabanlı Afet Uyarı Sistemi
 * Açıklama: Su ve Gaz sensörlerinden gelen verileri modüler bir yapıda 
 * işleyen ve tehlike durumuna göre farklı frekanslarda buzzer uyarısı veren ana kod.
 */

#include "WaterSensor.h"  // Su sensörü fonksiyonlarını içeren başlık dosyası
#include "AlarmSystem.h"  // Buzzer (alarm) kontrol fonksiyonlarını içeren başlık dosyası
#include "GasSensor.h"    // Gaz sensörü fonksiyonlarını içeren başlık dosyası

// Sabit Tanımlamalar
#define READ_INTERVAL 500 // Sensörlerin kaç milisaniyede bir okunacağını belirler (500ms = 0.5sn)

// Global Değişkenler
unsigned long lastRead = 0; // Son okuma zamanını hafızada tutmak için kullanılan değişken

void setup() {
    // Seri haberleşmeyi 115200 baud hızında başlat (Terminal ekranı için)
    Serial.begin(115200);
    
    // Modüllerin başlangıç ayarlarını yap (İlgili .cpp dosyalarındaki setup fonksiyonlarını çağırır)
    setupWaterSensor(); // Su sensörü pin ayarlarını yap
    setupBuzzer();      // Buzzer PWM (LEDC) ayarlarını ve başlangıç testini yap
    setupGasSensor();   // Gaz sensörü pin ayarlarını yap
    
    Serial.println("=== Afet Uyari Sistemi (Su + Gaz) Aktif ===");
}

void loop() {
    // Çalışma zamanını milisaniye cinsinden al
    unsigned long now = millis();

    // "Non-blocking" (gecikmesiz) zaman kontrolü: Belirlenen süre (500ms) geçti mi?
    if (now - lastRead >= READ_INTERVAL) {
        lastRead = now; // Son okuma zamanını güncelle

        // --- 1. VERİ TOPLAMA ---
        // Sensörlerin bağlı olduğu analog pinlerden ham (0-4095) değerleri oku
        int waterVal = readWaterLevel();
        int gasVal = readGasLevel();
        
        // --- 2. VERİ ANALİZİ ---
        // Okunan değerleri mantıksal sonuçlara dönüştür
        bool gasAlert = isGasDangerous(gasVal);         // Gaz seviyesi eşik değerin üzerinde mi?
        int waterFreq = getFrequencyByStatus(waterVal); // Su seviyesine göre çalacak buzzer frekansını al

        // --- 3. BİLGİLENDİRME (DEBUG) ---
        // Terminal ekranına sensör değerlerini yazdır
        Serial.printf("[%lu ms] Su: %4d | Gaz: %4d ", now, waterVal, gasVal);

        // --- 4. KARAR VE ALARM MEKANİZMASI ---
        
        // ÖNCELİK 1: Gaz/Duman Alarmı (Hayat kurtarmada daha kritik olduğu için ilk kontrol edilir)
        if (gasAlert) {
            Serial.println("-> DURUM: TEHLIKELI GAZ/DUMAN!");
            buzzerOn(2500); // Gaz tehlikesi için keskin (2500Hz) bir uyarı sesi ver
        } 
        
        // ÖNCELİK 2: Su Baskını Alarmı
        else if (waterFreq > 0) {
            // Su seviyesine göre durum metnini al (Kuru, Az Islak, Orta, Çok Islak)
            Serial.printf("-> DURUM: %s\n", getWaterStatus(waterVal).c_str());
            buzzerOn(waterFreq); // Su seviyesinin ciddiyetine göre değişen frekansta ses ver
        } 
        
        // DURUM GÜVENLİ: Herhangi bir tehlike yoksa
        else {
            Serial.println("-> DURUM: GUVENLI");
            buzzerOff(); // Alarmı sustur
        }
    }
}