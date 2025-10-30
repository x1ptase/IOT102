#include <Wire.h>
#include <RTClib.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// ===== LED MATRIX CONFIG =====
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN 13
#define DATA_PIN 11
#define CS_PIN 10
MD_Parola matrix = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ===== BLUETOOTH CONFIG =====
#define BT_RX 2
#define BT_TX 3
SoftwareSerial BT(BT_RX, BT_TX);

// ===== RTC CONFIG =====
RTC_DS1307 rtc;

// ===== LM35 CONFIG =====
#define LM35_PIN A0

// ===== EEPROM ADDRESS =====
#define ADDR_MODE 0
#define ADDR_EFFECT 1
#define ADDR_MSG 10
#define MSG_MAX_LEN 50

// ===== CONTROL VARIABLES =====
int mode = 1;       // 1: Time | 2: Date | 3: Message | 4: Temperature
int effectMode = 0; // 0: Left | 1: Right | 2: Up | 3: Down | 4: Static
String customMessage = "HELLO FPT";
String btBuffer = "";

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  // RTC setup
  if (!rtc.begin()) {
    Serial.println("Không tìm thấy DS1307!");
    while (1);
  }

  // ⚠️ CHỈ DÙNG DÒNG DƯỚI KHI MUỐN CÀI LẠI GIỜ (SAU ĐÓ COMMENT LẠI)
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (!rtc.isrunning()) {
    Serial.println("RTC chưa chạy, đang khởi động...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // LED setup
  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();

  // Load EEPROM
  loadEEPROM();

  Serial.println("=== SYSTEM READY ===");
  Serial.println("Bluetooth Commands:");
  Serial.println("1 → Show Time");
  Serial.println("2 → Show Date");
  Serial.println("3 + message → Show Custom Message (VD: 3HELLO)");
  Serial.println("4 → Show Temperature");
  Serial.println("L → Scroll Left to Right");
  Serial.println("R → Scroll Right to Left");
  Serial.println("U → Scroll Up");
  Serial.println("D → Scroll Down");
  Serial.println("S → Static Display / Show EEPROM");
  Serial.println("TYYYY-MM-DD-HH:MM → Set new time (VD: T2025-10-25-14:30)");
}

// ========== MAIN LOOP ==========
void loop() {
  DateTime now = rtc.now();
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;

  // Debug serial
  Serial.print("Thoi gian: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" | Nhiet do: ");
  Serial.print(temperature);
  Serial.println(" *C");
  delay(1000);

  // Bluetooth receive
  while (BT.available()) {
    char c = BT.read();
    if (c == '\n' || c == '\r') continue;
    if (btBuffer.length() < 50) btBuffer += c;
    delay(5);
  }

  if (btBuffer.length() > 0) {
    Serial.print("Bluetooth gửi: ");
    Serial.println(btBuffer);

    char cmd = btBuffer.charAt(0);
    if (cmd == 'T') {
      
      int year = btBuffer.substring(1, 5).toInt();
      int month = btBuffer.substring(6, 8).toInt();
      int day = btBuffer.substring(9, 11).toInt();
      int hour = btBuffer.substring(12, 14).toInt();
      int minute = btBuffer.substring(15, 17).toInt();
      rtc.adjust(DateTime(year, month, day, hour, minute, 0));
      Serial.println("🕒 Đã cập nhật thời gian mới từ Bluetooth!");
    } 
    else {
      switch (cmd) {
        case '1': mode = 1; saveEEPROM(); break;
        case '2': mode = 2; saveEEPROM(); break;
        case '3':
          mode = 3;
          if (btBuffer.length() > 1) {
            customMessage = btBuffer.substring(1);
            customMessage.trim();
            customMessage.toUpperCase();
          }
          saveEEPROM();
          break;
        case '4': mode = 4; saveEEPROM(); break;
        case 'L': effectMode = 0; saveEEPROM(); break;
        case 'R': effectMode = 1; saveEEPROM(); break;
        case 'U': effectMode = 2; saveEEPROM(); break;
        case 'D': effectMode = 3; saveEEPROM(); break;
        case 'S':
        case 's': effectMode = 4; showEEPROM(); saveEEPROM(); break;
      }
    }

    btBuffer = "";
  }

  switch (mode) {
    case 1: showTime(); break;
    case 2: showDate(); break;
    case 3: showMessage(customMessage); break;
    case 4: showTemperature(); break;
  }
}

// ========== EEPROM FUNCTIONS ==========
void saveEEPROM() {
  EEPROM.write(ADDR_MODE, mode);
  EEPROM.write(ADDR_EFFECT, effectMode);
  for (int i = 0; i < MSG_MAX_LEN; i++) {
    if (i < customMessage.length()) EEPROM.write(ADDR_MSG + i, customMessage[i]);
    else EEPROM.write(ADDR_MSG + i, 0);
  }
  Serial.println("EEPROM đã được lưu!");
}

void loadEEPROM() {
  mode = EEPROM.read(ADDR_MODE);
  if (mode < 1 || mode > 4) mode = 1;

  effectMode = EEPROM.read(ADDR_EFFECT);
  if (effectMode < 0 || effectMode > 4) effectMode = 0;

  char msg[MSG_MAX_LEN];
  for (int i = 0; i < MSG_MAX_LEN; i++) msg[i] = EEPROM.read(ADDR_MSG + i);
  msg[MSG_MAX_LEN - 1] = '\0';
  customMessage = String(msg);
  customMessage.trim();

  Serial.print("Đã load EEPROM - Mode: ");
  Serial.print(mode);
  Serial.print(" | Effect: ");
  Serial.print(effectMode);
  Serial.print(" | Msg: ");
  Serial.println(customMessage);
}

void showEEPROM() {
  Serial.println("==== EEPROM DATA ====");
  Serial.print("Mode: "); Serial.println(EEPROM.read(ADDR_MODE));
  Serial.print("Effect: "); Serial.println(EEPROM.read(ADDR_EFFECT));
  Serial.print("Message: ");
  for (int i = 0; i < MSG_MAX_LEN; i++) {
    char c = EEPROM.read(ADDR_MSG + i);
    if (c == 0) break;
    Serial.print(c);
  }
  Serial.println();
  Serial.println("=====================");
}

// ========== DISPLAY FUNCTION ==========
void displayText(String text) {
  textEffect_t effectIn = PA_NO_EFFECT;
  textEffect_t effectOut = PA_NO_EFFECT;
  int speed = 80;
  int pause = 0;

  switch (effectMode) {
    case 0: effectIn = PA_SCROLL_RIGHT; effectOut = PA_SCROLL_RIGHT; matrix.setTextAlignment(PA_RIGHT); break;
    case 1: effectIn = PA_SCROLL_LEFT;  effectOut = PA_SCROLL_LEFT;  matrix.setTextAlignment(PA_LEFT); break;
    case 2: effectIn = PA_SCROLL_UP;    effectOut = PA_SCROLL_UP;    matrix.setTextAlignment(PA_CENTER); break;
    case 3: effectIn = PA_SCROLL_DOWN;  effectOut = PA_SCROLL_DOWN;  matrix.setTextAlignment(PA_CENTER); break;
    case 4: effectIn = PA_PRINT;        effectOut = PA_PRINT;        matrix.setTextAlignment(PA_CENTER); break;
  }

  matrix.displayText(text.c_str(), PA_CENTER, speed, pause, effectIn, effectOut);
  while (!matrix.displayAnimate()) {}
}

// ========== SHOW FUNCTIONS ==========
void showTime() {
  DateTime now = rtc.now();
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());
  displayText(String(timeStr));
}

void showDate() {
  DateTime now = rtc.now();
  char dateStr[11];
  sprintf(dateStr, "%02d/%02d/%02d", now.day(), now.month(), now.year() % 100);
  displayText(String(dateStr));
}

void showMessage(String msg) {
  if (msg.length() > 20) msg = msg.substring(0, 20);
  displayText(msg);
}

void showTemperature() {
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;
  char tempStr[16];
  sprintf(tempStr, "%d*C", temperature);
  displayText(String(tempStr));
}
