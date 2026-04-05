/* *
 * Açıklama: Su seviye sensöründen gelen analog verilerin okunması, 
 * anlamlandırılması ve bu verilere uygun ses frekanslarının belirlenmesi.
 */

#include "WaterSensor.h"

// Su sensörünün bağlı olduğu pin ayarlarını yapan fonksiyon
void setupWaterSensor() {
    // Sensör verisini okuyacağımız için pini (GPIO 34) giriş moduna alıyoruz
    pinMode(WATER_PIN, INPUT);
}

// Sensörden anlık ham analog veriyi okuyan fonksiyon
int readWaterLevel() {
    // ESP32'nin 12-bit ADC birimi sayesinde 0 ile 4095 arasında bir değer döner
    return analogRead(WATER_PIN);
}

// Okunan sayısal değeri insan tarafından anlaşılır bir metne dönüştüren fonksiyon
String getWaterStatus(int rawValue) {
    /* * Eşik Değerleri Analizi:
     * 1000 altı: Sensör tamamen kuru veya çok az nemli.
     * 1000-2000 arası: Yüzeyde hafif su damlacıkları var.
     * 2000-3500 arası: Sensörün bir kısmı suya temas ediyor.
     * 3500 üstü: Sensör tamamen suyun içinde veya yoğun sıvı temasında.
     */
    if (rawValue < 1000) return "Kuru";
    if (rawValue < 2000) return "Az Islak";
    if (rawValue < 3500) return "Orta";
    
    return "Cok Islak / Suda"; // Yukarıdaki şartların hiçbiri sağlanmazsa bu döner
}

// Su seviyesinin ciddiyetine göre çalacak buzzer frekansını belirleyen fonksiyon
int getFrequencyByStatus(int rawValue) {
    /* * Frekans Mantığı (Hz):
     * Tehlike arttıkça (su seviyesi yükseldikçe) frekans değeri artar.
     * Bu sayede kullanıcı sesin incelmesinden durumun aciliyetini anlar.
     */
    if (rawValue < 1000) return 0;    // Tehlike yok, ses çıkarma
    if (rawValue < 2000) return 800;  // Hafif uyarı (Kalın ses)
    if (rawValue < 3500) return 1200; // Orta seviye uyarı
    
    return 2000; // Kritik seviye (İnce ve keskin ses)
}