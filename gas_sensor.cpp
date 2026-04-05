/* *l
 * Açıklama: Gaz sensöründen (MQ-2/MQ-5) analog verilerin okunması ve 
 * bu verilerin tehlike sınırına göre analiz edilmesini sağlayan fonksiyonlar.
 */

#include "GasSensor.h"

// Gaz sensörünün başlangıç ayarlarını yapan fonksiyon
void setupGasSensor() {
    // Gaz sensörünün bağlı olduğu pini (GPIO 35) giriş (INPUT) olarak ayarla
    pinMode(GAS_PIN, INPUT);
}

// Sensörden anlık analog değeri okuyan fonksiyon
int readGasLevel() {
    // ESP32'nin ADC birimini kullanarak 0-4095 arası bir değer döner
    return analogRead(GAS_PIN);
}

// Okunan değerin tehlikeli olup olmadığını kontrol eden fonksiyon
bool isGasDangerous(int rawValue) {
    /* * Eşik Değeri (Threshold): 1200
     * Bu değer ortamdaki temiz hava kalitesine göre test edilerek ayarlanmalıdır.
     * Genellikle 1000-1500 arası değerler sızıntı/duman başlangıcı kabul edilir.
     * Eğer okunan değer 1200'den büyükse 'true' (doğru), küçükse 'false' (yanlış) döner.
     */
    return (rawValue > 1200); 
}