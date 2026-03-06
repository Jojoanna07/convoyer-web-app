#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WebSocketsClient.h> // Library Baru untuk Railway

#define IR_PIN 2
#define SDA_PIN 21
#define SCL_PIN 22

#define ENA_PIN 15
#define IN1_PIN 16
#define IN2_PIN 17

LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- ISI DENGAN HOTSPOT HP KAMU ---
const char* ssid = "Joanna R";
const char* password = "yayayaya";

// --- ISI DENGAN URL RAILWAY (TANPA wss:// DAN TANPA / DI AKHIR) ---
const char* websocket_server = "websocket-production-f022.up.railway.app";
const uint16_t websocket_port = 443; // 443 Wajib untuk WSS (Aman)

WebSocketsClient webSocket;

const int PWM_CH = 0;
const int PWM_FREQ = 20000;
const int PWM_RES = 8;
volatile int pwmValue = 0;

uint32_t counter = 0;
int lastIR = HIGH;
const unsigned long COUNT_LOCKOUT_MS = 80;
unsigned long lastCountMs = 0;
const unsigned long LCD_INTERVAL = 200;
unsigned long lastLcdMs = 0;

typedef struct __attribute__((packed)) { uint8_t emergency; } EspNowMsg;
volatile bool emergencyStop = false;
bool lastEmergencyStop = false;

void motorForward(int pwm) {
  if(emergencyStop) return;
  pwm = constrain(pwm, 0, 255);
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  ledcWrite(PWM_CH, pwm);
}

void motorStop() {
  ledcWrite(PWM_CH, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
}

void lcdShowEmergency() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCY STOP!!");
  lcd.setCursor(0, 1);
  lcd.print("Motor berhenti");
}

void lcdShowRunTemplate() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Count: ");
  lcd.setCursor(0, 1);
  lcd.print("PWM: ");
}

void lcdUpdateRunValues() {
  lcd.setCursor(7, 0);
  lcd.print(counter);
  lcd.print("     ");
  lcd.setCursor(5, 1);
  lcd.print((int)pwmValue);
  lcd.print("     ");
}

void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(EspNowMsg)) return;
  EspNowMsg msg;
  memcpy(&msg, data, sizeof(msg));
  emergencyStop = (msg.emergency == 1);
  if (emergencyStop) motorStop();
  else motorForward(pwmValue);
}

// Fungsi jika dapat data dari Railway
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if(type == WStype_TEXT) {
        String msg = (char*)payload;
        pwmValue = msg.toInt(); 
        Serial.print("Dari Railway -> PWM: ");
        Serial.println(pwmValue);
        
        if(!emergencyStop) motorForward(pwmValue);
    }
}

void setup() {
  Serial.begin(115200); delay(800);

  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA_PIN, PWM_CH);

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init(); lcd.backlight();

  // 1. Konek ke Hotspot HP
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.print("Menyambung ke Internet...");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nInternet Terhubung!");

  // 2. Setup ESP-NOW
  if (esp_now_init() != ESP_OK) Serial.println("ESP-NOW FAIL!");
  esp_now_register_recv_cb(onEspNowRecv);

  // 3. Konek ke Railway
  webSocket.beginSSL(websocket_server, websocket_port, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Coba konek lagi tiap 5 detik kalau putus

  lcdShowRunTemplate();
  lcdUpdateRunValues();
  Serial.println("Node B Siap Menerima Perintah dari Railway!");
}

void loop() {
  webSocket.loop(); // Wajib ada agar ESP32 terus ngecek pesan dari Railway

  if (emergencyStop != lastEmergencyStop) {
    lastEmergencyStop = emergencyStop;
    if (emergencyStop) lcdShowEmergency();
    else { lcdShowRunTemplate(); lcdUpdateRunValues(); }
  }
  if (emergencyStop) { delay(20); return; }

  int irNow = digitalRead(IR_PIN);
  if (lastIR == HIGH && irNow == LOW) {
    unsigned long now = millis();
    if (now - lastCountMs >= COUNT_LOCKOUT_MS) {
      lastCountMs = now;
      counter++;
    }
  }
  lastIR = irNow;

  if (millis() - lastLcdMs >= LCD_INTERVAL) {
    lastLcdMs = millis();
    lcdUpdateRunValues();
  }
}