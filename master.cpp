#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h> 
#include <DHT.h>
// soc kütüphaneleri ESP32'nin çekirdek donanım ayarlarına müdahale etmek içindir
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- 1. DONANIM VE PİN TANIMLAMALARI ---
#define DHTPIN 15       // Isı ve Nem sensörünün bağlı olduğu dijital pin
#define DHTTYPE DHT11   // Kullanılan DHT sensörünün modeli
#define MQ2_PIN 32      // Gaz sensörünün bağlı olduğu analog pin (ADC)
#define WATER_PIN 35    // Su seviye sensörünün bağlı olduğu analog pin (ADC)
#define BUZZER_PIN 25   // Alarm sesini verecek buzzer'ın dijital pini

// --- 2. AĞ (NETWORK) BİLGİLERİ ---
// Sistemin dış dünya ve MacroDroid ile haberleşmesi için bağlanacağı ağ
const char* ssid = "OPPO Reno11 FS"; 
const char* password = "bugra123";

// 80 portu üzerinden HTTP isteklerini dinleyecek minik web sunucusu
WebServer sunucu(80); 
DHT dht(DHTPIN, DHTTYPE);

// Slave (Alıcı) ESP32'nin MAC adresi (Sinyalin kime gideceğini belirler)
uint8_t broadcastAddress[] = {0x68, 0xFE, 0x71, 0xFA, 0x44, 0x0C}; 

// --- 3. VERİ PAKETİ YAPISI (STRUCT) ---
// ESP-NOW üzerinden havadan gönderilecek veri paketinin şablonu
typedef struct struct_message {
    int sensorType; // Hangi alarmın çaldığını belirtir (1: Gaz, 2: Su, 3: Deprem, 4: Normal Veri)
    float temp;     // Sıcaklık değeri
    float hum;      // Nem değeri
    float gasVal;   // Okunan gaz veya suyun sayısal şiddeti
} struct_message;

struct_message myData;           // Gönderilecek verileri tutan obje
esp_now_peer_info_t peerInfo;    // Bağlantı kurulacak Slave cihazın ayarlarını tutan obje

unsigned long lastExtraData = 0; // Periyodik veri gönderim zamanını tutar (Gecikme/Delay yerine millis kullanıyoruz)
unsigned long lastAlarm = 0;     // Peş peşe alarm yollayıp ağı boğmamak için zaman tutar

// ESP-NOW paketi karşıya ulaştığında (veya ulaşmadığında) tetiklenen geri bildirim fonksiyonu
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Seri port ekranı çok kalabalık olmasın diye buradaki logları gizledik
}

// --- 4. WEB SUNUCUSU: DEPREM TETİKLEYİCİSİ ---
// MacroDroid'den (telefondan) IP_ADRESI/deprem isteği geldiğinde bu fonksiyon çalışır
void handleDeprem() {
    myData.sensorType = 3; // Paketin tipini 3 (Deprem) olarak ayarla
    
    // ESP-NOW ile Slave cihaza "Deprem oldu" paketini fırlat
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); 
    
    Serial.println(">>> MACRODROID'DEN DEPREM SINYALI ALINDI! VALF KAPATILIYOR! <<<");
    digitalWrite(BUZZER_PIN, HIGH); // Master cihazın buzzer'ını öttür
    
    // Telefondaki MacroDroid'e "İsteğini aldım, görev tamam" diye HTTP 200 (Başarılı) cevabı döndür
    sunucu.send(200, "text/plain", "ESP32: Deprem Sinyali Alindi, Valf Kapandi!");
    
    delay(2000); // Buzzer 2 saniye ötsün
    digitalWrite(BUZZER_PIN, LOW); // Buzzer'ı sustur
}

// --- 5. İLK KURULUM (SETUP) ---
void setup() {
    // ESP32 anlık güç çekimlerinde voltaj düşerse kendini resetlemesin diye Brown-Out korumasını kapatıyoruz
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
    
    Serial.begin(115200);
    delay(1000); 

    Serial.println("\n--- MASTER SISTEMI BASLATILDI ---");
    dht.begin();
    pinMode(BUZZER_PIN, OUTPUT);
    
    // --- WI-FI VE KANAL SENKRONİZASYONU ---
    WiFi.mode(WIFI_STA); // ESP32'yi İstasyon (İstemci) moduna al
    Serial.print("Telefona Baglaniliyor: ");
    WiFi.begin(ssid, password); // Hotspot'a bağlan
    
    // İnternete bağlanana kadar bekle
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nHotspot Baglantisi Basarili!");
    Serial.print("MACRODROID ICIN IP ADRESIN: ");
    Serial.println(WiFi.localIP()); // Dışarıdan gelecek tetiklemeler için IP adresini ekrana yaz

    // ÇOK ÖNEMLİ: ESP-NOW'ın anten frekansını (kanalını), telefonun Wi-Fi kanalıyla aynı yapıyoruz.
    // Eğer bunu yapmazsak ESP32 iki farklı kanalı dinlemeye çalışırken kilitlenir.
    int wifiKanal = WiFi.channel();
    Serial.printf("Telefonun Wi-Fi Kanali: %d (ESP-NOW buna sabitlendi)\n", wifiKanal);
    
    if (esp_now_init() != ESP_OK) return; // ESP-NOW başlatılamazsa dur
    esp_now_register_send_cb(OnDataSent); // Gönderim durumunu takip eden fonksiyonu kaydet

    // Slave cihazı eşleşme listesine ekle
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = wifiKanal; // Frekansı eşitliyoruz
    peerInfo.encrypt = false;     // Şifreleme kullanmıyoruz (Hızı artırmak için)
    esp_now_add_peer(&peerInfo);

    // --- WEB SUNUCUYU BAŞLAT ---
    // Eğer ağ üzerinden IP_ADRESI/deprem adresine bir istek gelirse, handleDeprem fonksiyonuna yönlendir
    sunucu.on("/deprem", handleDeprem); 
    sunucu.begin(); // Sunucuyu dinlemeye başla
}

// --- 6. ANA DÖNGÜ (LOOP) ---
void loop() {
    // Web sunucusuna gelen yeni bir istek var mı diye sürekli kontrol et
    sunucu.handleClient(); 

    // Analog sensörlerden güncel değerleri oku
    int gasRaw = analogRead(MQ2_PIN);
    int waterRaw = analogRead(WATER_PIN);
    bool alarmTriggered = false; // O an alarm olup olmadığını tutan bayrak (flag)

    // --- SENARYO 1: GAZ VEYA SU ALARMI (ACİL DURUM) ---
    // Eğer gaz veya su eşik değerin üzerine çıkarsa sistemi tetikle
    if (gasRaw > 2800 || waterRaw > 1800) { 
        // Ağı boğmamak için aynı alarmı en fazla 2 saniyede bir gönder (Rate Limiting)
        if (millis() - lastAlarm > 2000) { 
            myData.sensorType = (gasRaw > 2800) ? 1 : 2; // Sorun gazda mı suda mı belirle
            myData.gasVal = (float)(gasRaw > 2800 ? gasRaw : waterRaw); // Şiddeti pakete yaz
            
            esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // Acil durum paketini fırlat
            lastAlarm = millis();
            Serial.println(">>> DIKKAT: SENSOR ALARMI TETIKLENDI! <<<");
        }
        digitalWrite(BUZZER_PIN, HIGH); // Alarm anında sireni çal
        alarmTriggered = true;          // Alarm bayrağını kaldır ki DHT okuması bekletilsin
    } else {
        digitalWrite(BUZZER_PIN, LOW);  // Her şey normalse sireni sustur
    }

    // --- SENARYO 2: PERİYODİK VERİ GÖNDERİMİ (NORMAL DURUM) ---
    // Alarm yoksa VE son gönderimden bu yana 5 saniye geçmişse (sistemi yormamak için)
    if (!alarmTriggered && (millis() - lastExtraData > 5000)) {
        float tempT = dht.readTemperature(); // Sıcaklığı oku
        float tempH = dht.readHumidity();    // Nemi oku

        // --- NAN (Not a Number) KALKANI ---
        // DHT sensörleri hassastır, bazen okuma yapamaz ve "nan" (sayı değil) döndürür.
        // Eğer okunan değer düzgün bir sayıysa paketle. Değilse eski değeri koru (Sistemi çökertme).
        if (!isnan(tempT) && !isnan(tempH)) {
            myData.temp = tempT;
            myData.hum = tempH;
        }

        myData.sensorType = 4; // Tip 4: Her şey yolunda, bu sadece bilgi paketidir
        myData.gasVal = (float)gasRaw; // Normal durumda da ekranda gaz seviyesi görünsün
        
        esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData)); // Normal veriyi fırlat
        lastExtraData = millis(); // Zamanlayıcıyı sıfırla
        
        // Okunan verileri test için ekrana yazdır
        Serial.printf("Isi: %.1f C | Nem: %.1f %% | Gaz: %d | Su: %d\n", myData.temp, myData.hum, gasRaw, waterRaw);
    }
    
    // İşlemciyi (CPU) %100 yükte kitlememek ve arka plandaki Wi-Fi işlemlerine nefes aldırmak için minik bir gecikme
    delay(10); 
}