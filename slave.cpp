#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- YENİ EKLENDİ: TELEFON HOTSPOT BİLGİLERİ ---
const char* ssid = "OPPO Reno11 FS"; 
const char* password = "bugra123";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int btnNext = 14;   
const int btnBack = 13;   
const int btnSelect = 12; 
const int buzzerPin = 27; 
const int servoPin = 26; 

Servo valfServo;

int menuIndex = 0;
int currentScreen = 0; 
bool alarmActive = false;
bool valveIsClosed = false;

typedef struct struct_message {
    int sensorType; 
    float temp;
    float hum;
    float gasVal;
} struct_message;

struct_message incomingData;
SemaphoreHandle_t xMutex;

// --- BAĞIMSIZ KANAL (LEDC) BUZZER KONTROLÜ ---
void buzzerOtsun() {
    ledcWrite(7, 128); 
}

void buzzerSussun() {
    ledcWrite(7, 0); 
}

void valfiKapat() {
    valfServo.attach(servoPin, 500, 2400);
    for (int pos = 10; pos <= 100; pos += 2) { 
        valfServo.write(pos);
        vTaskDelay(20 / portTICK_PERIOD_MS); 
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    valfServo.detach(); 
}

void valfiAc() {
    valfServo.attach(servoPin, 500, 2400);
    for (int pos = 100; pos >= 10; pos -= 2) { 
        valfServo.write(pos);
        vTaskDelay(20 / portTICK_PERIOD_MS); 
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    valfServo.detach();
}

void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
    if (len == sizeof(struct_message)) {
        struct_message temp;
        memcpy(&temp, data, sizeof(temp));
        
        xSemaphoreTake(xMutex, portMAX_DELAY);
        if(temp.sensorType >= 1 && temp.sensorType <= 3) {
            if(!alarmActive) {
                incomingData = temp;
                alarmActive = true;
            }
        } else if (temp.sensorType == 4) {
            incomingData.temp = temp.temp;
            incomingData.hum = temp.hum;
            if(!alarmActive) incomingData.gasVal = temp.gasVal;
        }
        xSemaphoreGive(xMutex);
    }
}

void TaskDisplay(void *pvParameters) {
    for (;;) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextColor(WHITE);
        
        xSemaphoreTake(xMutex, portMAX_DELAY);
        if (currentScreen == 0) {
            display.println("--- GUVENLIK SISTEMI ---");
            display.println("");
            const char* menuItems[] = {"Sistem Durumu", "Sensor Verileri", "Sistem Testi", "Sustur / Reset"};
            for (int i = 0; i < 4; i++) {
                display.println((i == menuIndex ? "> " : "  ") + String(menuItems[i]));
            }
        } else {
            if (menuIndex == 0) {
                display.println("[ DURUM ]");
                if (alarmActive) {
                    if (incomingData.sensorType == 1) display.println("Alarm: GAZ SIZINTISI!");
                    else if (incomingData.sensorType == 2) display.println("Alarm: SU BASKINI!");
                    else display.println("Alarm: TEHLIKE!");
                } else {
                    display.println("Alarm: Normal");
                }
                display.print("Valf: "); display.println(alarmActive ? "KAPALI" : "ACIK");
            } else if (menuIndex == 1) {
                display.println("[ VERILER ]");
                display.printf("Isi: %.1f C\n", incomingData.temp);
                display.printf("Nem: %%%.1f\n", incomingData.hum);
                if(alarmActive && incomingData.sensorType == 2) {
                    display.printf("Su Seviye: %d\n", (int)incomingData.gasVal);
                } else {
                    display.printf("Gaz: %d\n", (int)incomingData.gasVal);
                }
            }
            display.setCursor(0, 55); display.println("<- Geri don (SEC)");
        }
        xSemaphoreGive(xMutex);
        
        display.display();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void TaskControl(void *pvParameters) {
    for (;;) {
        if (alarmActive && !valveIsClosed) {
            buzzerOtsun(); 
            valfiKapat();
            valveIsClosed = true;
        } 
        else if (!alarmActive && valveIsClosed) {
            buzzerSussun();  
            valfiAc();
            valveIsClosed = false;
        }

        if (digitalRead(btnNext) == LOW) {
            if (currentScreen == 0) menuIndex = (menuIndex + 1) % 4;
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
        if (digitalRead(btnBack) == LOW) {
            if (currentScreen == 0) menuIndex = (menuIndex - 1 + 4) % 4;
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
        if (digitalRead(btnSelect) == LOW) {
            if (currentScreen == 0) {
                if (menuIndex == 2) { 
                    buzzerOtsun(); 
                    valfiKapat();  
                    valveIsClosed = true;
                    
                    vTaskDelay(2500 / portTICK_PERIOD_MS); 
                    
                    if(!alarmActive) {             
                        valfiAc();      
                        buzzerSussun(); 
                        valveIsClosed = false;
                    }
                } else if (menuIndex == 3) { 
                    buzzerSussun();  
                    xSemaphoreTake(xMutex, portMAX_DELAY);
                    alarmActive = false; 
                    incomingData.sensorType = 0;
                    xSemaphoreGive(xMutex);
                } else { currentScreen = 1; }
            } else { currentScreen = 0; }
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
    Serial.begin(115200);
    delay(1000); 

    Serial.println("\n--- SLAVE SISTEMI BASLATILDI ---");
    
    // --- YENİ EKLENDİ: FREKANS EŞİTLEMEK İÇİN HOTSPOT'A BAĞLAN ---
    WiFi.mode(WIFI_STA);
    Serial.print("Master ile ayni kanala (Hotspot'a) baglaniliyor: ");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nBaglanti Basarili! Kanal Esitlendi.");
    Serial.print("BU CIHAZIN MAC ADRESI: ");
    Serial.println(WiFi.macAddress()); 

    ESP32PWM::allocateTimer(0);
    valfServo.setPeriodHertz(50);

    xMutex = xSemaphoreCreateMutex();
    pinMode(btnNext, INPUT_PULLUP);
    pinMode(btnBack, INPUT_PULLUP);
    pinMode(btnSelect, INPUT_PULLUP);
    
    ledcSetup(7, 2000, 8); 
    ledcAttachPin(buzzerPin, 7);
    buzzerSussun(); 

    Wire.begin();
    Wire.setTimeOut(20); 

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        for(;;);
    }
    display.clearDisplay();

    valfiAc();
    valveIsClosed = false;

    // WiFi bagliyken ESP-NOW'i baslat ki anten aynı kanalda kalsin
    if (esp_now_init() == ESP_OK) {
        esp_now_register_recv_cb(OnDataRecv);
    }

    xTaskCreatePinnedToCore(TaskDisplay, "Disp", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskControl, "Ctrl", 4096, NULL, 1, NULL, 0);
}

void loop() { vTaskDelete(NULL); }