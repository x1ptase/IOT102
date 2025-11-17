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
int effectMode = 0; // 0:L | 1:R | 2:U | 3:D | 4:Static
String customMessage = "HELLO FPT";
String btBuffer = "";

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  if (!rtc.begin()) while (1);
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();
  matrix.setFont(nullptr);

  loadEEPROM();
  Serial.println("=== SYSTEM READY ===");
}


// ========== MAIN LOOP ==========
void loop() {
  DateTime now = rtc.now();
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;

  while (BT.available()) {
    char c = BT.read();
    if (c != '\n' && c != '\r') btBuffer += c;
  }

  if (btBuffer.length() > 0) {
    char cmd = btBuffer.charAt(0);

    if (cmd == 'T') {
      int year = btBuffer.substring(1, 5).toInt();
      int month = btBuffer.substring(6, 8).toInt();
      int day = btBuffer.substring(9, 11).toInt();
      int hour = btBuffer.substring(12, 14).toInt();
      int minute = btBuffer.substring(15, 17).toInt();
      rtc.adjust(DateTime(year, month, day, hour, minute, 0));
    } 
    else {
      switch (cmd) {
        case '1': mode = 1; saveEEPROM(); break;
        case '2': mode = 2; saveEEPROM(); break;
        case '3': mode = 3; customMessage = btBuffer.substring(1); customMessage.trim(); customMessage.toUpperCase(); saveEEPROM(); break;
        case '4': mode = 4; saveEEPROM(); break;
        case 'L': effectMode = 0; saveEEPROM(); break;
        case 'R': effectMode = 1; saveEEPROM(); break;
        case 'U': effectMode = 2; saveEEPROM(); break;
        case 'D': effectMode = 3; saveEEPROM(); break;
        case 'S': effectMode = 4; saveEEPROM(); break;
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

// ===== EEPROM =====
void saveEEPROM() {
  EEPROM.write(ADDR_MODE, mode);
  EEPROM.write(ADDR_EFFECT, effectMode);
  for (int i = 0; i < MSG_MAX_LEN; i++) EEPROM.write(ADDR_MSG + i, (i < customMessage.length()) ? customMessage[i] : 0);
}

void loadEEPROM() {
  mode = EEPROM.read(ADDR_MODE);
  effectMode = EEPROM.read(ADDR_EFFECT);
  char msg[MSG_MAX_LEN];
  for (int i = 0; i < MSG_MAX_LEN; i++) msg[i] = EEPROM.read(ADDR_MSG + i);
  msg[MSG_MAX_LEN - 1] = '\0';
  customMessage = String(msg);
  customMessage.trim();
}

// ===== DISPLAY ENGINE =====
void displayText(String text) {
  textEffect_t effect = PA_SCROLL_LEFT;
  int speed = 40, pause = 0;

  switch (effectMode) {
    case 0: effect = PA_SCROLL_RIGHT; break;
    case 1: effect = PA_SCROLL_LEFT; break;
    case 2: effect = PA_SCROLL_UP; break;
    case 3: effect = PA_SCROLL_DOWN; break;
    case 4: effect = PA_PRINT; break;
  }

  matrix.setTextAlignment(PA_CENTER);
  matrix.displayText(text.c_str(), PA_CENTER, speed, pause, effect, effect);
  while (!matrix.displayAnimate());
}

// ===== SHOW FUNCTIONS =====
void showTime() {
  DateTime now = rtc.now();
  char buffer[6];
  sprintf(buffer, "%02d:%02d", now.hour(), now.minute());
  displayText(buffer);
}

void showDate() {
  DateTime now = rtc.now();
  char buffer[11];
  sprintf(buffer, "%02d/%02d/%02d", now.day(), now.month(), now.year() % 100);
  displayText(buffer);
}

// Hiển thị message dài theo hiệu ứng UP/DOWN; tự động tính kích thước, không dính chữ
void showMessage(String msg) {
  msg.trim();
  msg.toUpperCase();

  // Chọn hiệu ứng
  textEffect_t eff;
  if (effectMode == 2) eff = PA_SCROLL_UP;
  else if (effectMode == 3) eff = PA_SCROLL_DOWN;
  else {
    displayText(msg);
    return;
  }

  // Tính số cột hiển thị thực tế
  int totalCols = MAX_DEVICES * 8; // mỗi module FC16 = 8 cột
  // xấp xỉ: mỗi ký tự font mặc định ~6 cột (5 pixel + 1 space)
  const int approxCharWidth = 6;
  int maxCharsPerLine = totalCols / approxCharWidth;
  if (maxCharsPerLine < 1) maxCharsPerLine = 1;

  // padding spaces giữa các phần (đảm bảo 1-2 cột trống)
  const int padSpaces = 2;

  // thêm 1 space ở cuối chuỗi để dễ xử lý
  msg += " ";

  Serial.print("TotalCols="); Serial.print(totalCols);
  Serial.print(" maxCharsPerLine="); Serial.println(maxCharsPerLine);
  Serial.print("showMessage: '"); Serial.print(msg); Serial.println("'");

  int n = msg.length();
  int pos = 0;
  int partIndex = 0;

  while (pos < n) {
    // bỏ khoảng trắng đầu
    while (pos < n && msg.charAt(pos) == ' ') pos++;
    if (pos >= n) break;

    // end là vị trí lý tưởng cắt theo số ký tự
    int end = pos + maxCharsPerLine;
    if (end > n) end = n;

    // tìm lastSpace trong khoảng [pos, end-1]
    int lastSpace = -1;
    for (int i = pos; i < end; i++) {
      if (msg.charAt(i) == ' ') lastSpace = i;
    }

    String part;
    if (end >= n) {
      part = msg.substring(pos, n);
      pos = n;
    } else if (msg.charAt(end) == ' ') {
      part = msg.substring(pos, end);
      pos = end + 1;
    } else if (lastSpace != -1) {
      part = msg.substring(pos, lastSpace);
      pos = lastSpace + 1;
    } else {
      // không có khoảng trắng trong phạm vi => cắt thẳng
      part = msg.substring(pos, end);
      pos = end;
    }

    part.trim();

    // nếu sau này vẫn còn chữ, thêm padding space để bảo toàn cột trắng
    if (pos < n) {
      for (int s = 0; s < padSpaces; s++) part += " ";
    }

    Serial.print("part["); Serial.print(partIndex++); Serial.print("] = '");
    Serial.print(part); Serial.println("'");

    // Hiển thị phần bằng PA_LEFT để giữ cột trắng cuối
    matrix.displayClear();
    matrix.displayReset();
    matrix.displayText(part.c_str(), PA_LEFT, 60, 0, eff, eff);
    while (!matrix.displayAnimate()) { /* animate */ }
    delay(220);
  }
}




void showTemperature() {
  int analogValue = analogRead(LM35_PIN);
  float voltage = analogValue * (5.0 / 1023.0);
  int temperature = voltage * 100;
  char buffer[10];
  sprintf(buffer, "%d*C", temperature);
  displayText(buffer);
}