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
#define MSG_MAX_LEN 38

// ===== CONTROL VARIABLES =====
int mode = 1;
int effectMode = 0;
char customMessage[MSG_MAX_LEN + 1] = "HELLO FPT";
char btBuffer[64] = "";

// ========== SETUP ==========
void setup() {
  BT.begin(9600);
  if (!rtc.begin()) while (1);

  // GIỜ VIỆT NAM (GMT+7)
  if (!rtc.isrunning()) {
    char d[12], t[9];
    strcpy_P(d, __DATE__);
    strcpy_P(t, __TIME__);
    char m[4]; int dd, yy, hh, mm, ss;
    sscanf(d, "%3s %d %d", m, &dd, &yy);
    sscanf(t, "%d:%d:%d", &hh, &mm, &ss);
    const char PROGMEM months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int mo = (strstr_P(months, m) - months) / 3 + 1;
    rtc.adjust(DateTime(yy, mo, dd, hh, mm, ss) + TimeSpan(25200));
  }

  matrix.begin();
  matrix.setIntensity(5);
  matrix.displayClear();
  matrix.setFont(nullptr);
  loadEEPROM();
}

// ========== MAIN LOOP ==========
void loop() {
  int temp = (int)(analogRead(LM35_PIN) * 0.4888);

  // BLUETOOTH: ĐÃ SỬA – XỬ LÝ \r VÀ \n
  while (BT.available()) {
    char c = BT.read();
    if (c != '\n' && c != '\r' && strlen(btBuffer) < 60) {
      strncat(btBuffer, &c, 1);
    }
    if (c == '\n' || c == '\r') {  // XỬ LÝ KHI GẶP \r HOẶC \n
      processCommand();
      btBuffer[0] = '\0';
      break;  // THOÁT VÒNG LẶP ĐỂ TRÁNH ĐỌC DƯ
    }
  }

  switch (mode) {
    case 1: showTime(); break;
    case 2: showDate(); break;
    case 3: showMessage(customMessage); break;
    case 4: showTemperature(temp); break;
  }
}

// ===== XỬ LÝ LỆNH =====
void processCommand() {
  if (strlen(btBuffer) == 0) return;

  if (btBuffer[0] == 'T' && strlen(btBuffer) >= 17) {
    int y = atoi(btBuffer + 1);
    int mo = atoi(btBuffer + 6);
    int d = atoi(btBuffer + 9);
    int h = atoi(btBuffer + 12);
    int mi = atoi(btBuffer + 15);
    rtc.adjust(DateTime(y, mo, d, h, mi, 0));
  } else {
    switch (btBuffer[0]) {
      case '1': mode = 1; saveEEPROM(); break;
      case '2': mode = 2; saveEEPROM(); break;
      case '3':
        mode = 3;
        strncpy(customMessage, btBuffer + 1, MSG_MAX_LEN);
        customMessage[MSG_MAX_LEN] = '\0';
        my_strupr(customMessage);
        trim(customMessage);
        saveEEPROM();
        break;
      case '4': mode = 4; saveEEPROM(); break;
      case 'L': effectMode = 0; saveEEPROM(); break;
      case 'R': effectMode = 1; saveEEPROM(); break;
      case 'U': effectMode = 2; saveEEPROM(); break;
      case 'D': effectMode = 3; saveEEPROM(); break;
      case 'S': effectMode = 4; saveEEPROM(); break;
    }
  }
}

// ===== EEPROM =====
void saveEEPROM() {
  EEPROM.update(ADDR_MODE, mode);
  EEPROM.update(ADDR_EFFECT, effectMode);
  for (int i = 0; i < MSG_MAX_LEN; i++)
    EEPROM.update(ADDR_MSG + i, (i < strlen(customMessage)) ? customMessage[i] : 0);
}

void loadEEPROM() {
  mode = EEPROM.read(ADDR_MODE);
  effectMode = EEPROM.read(ADDR_EFFECT);
  if (mode < 1 || mode > 4) mode = 1;
  if (effectMode < 0 || effectMode > 4) effectMode = 0;

  for (int i = 0; i < MSG_MAX_LEN; i++) {
    customMessage[i] = EEPROM.read(ADDR_MSG + i);
    if (!customMessage[i]) break;
  }
  customMessage[MSG_MAX_LEN] = '\0';
  if (!customMessage[0]) strcpy(customMessage, "HELLO FPT");
}

// ===== HIỂN THỊ =====
void displayText(const char* text) {
  textEffect_t effect = PA_SCROLL_LEFT;
  int speed = 40, pause = 0;
  switch (effectMode) {
    case 0: effect = PA_SCROLL_RIGHT; break;
    case 1: effect = PA_SCROLL_LEFT; break;
    case 2: effect = PA_SCROLL_UP; break;
    case 3: effect = PA_SCROLL_DOWN; break;
    case 4: effect = PA_PRINT; pause = 2000; break;
  }
  matrix.displayClear();
  matrix.displayReset();
  matrix.setTextAlignment(PA_CENTER);
  matrix.displayText(text, PA_CENTER, speed, pause, effect, effect);
  if (effectMode == 4) {
    matrix.displayAnimate();
    delay(2000);
    matrix.displayClear();
  } else {
    while (!matrix.displayAnimate());
  }
}

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

void showTemperature(int temp) {
  char buffer[10];
  sprintf(buffer, "%d*C", temp);
  displayText(buffer);
}

// ===== SHOW MESSAGE (UP/DOWN CĂN GIỮA) =====
void showMessage(char* msg) {
  my_strupr(msg);
  trim(msg);
  if (effectMode != 2 && effectMode != 3) {
    displayText(msg);
    return;
  }
  textEffect_t eff = (effectMode == 2) ? PA_SCROLL_UP : PA_SCROLL_DOWN;
  int totalCols = MAX_DEVICES * 8;
  int maxChars = totalCols / 6;
  if (maxChars < 1) maxChars = 1;

  char full[64];
  strcpy(full, msg);
  strcat(full, " ");
  int len = strlen(full), pos = 0;

  while (pos < len) {
    while (pos < len && full[pos] == ' ') pos++;
    if (pos >= len) break;

    int end = pos + maxChars;
    if (end > len) end = len;

    int lastSpace = -1;
    for (int i = pos; i < end; i++) {
      if (full[i] == ' ') lastSpace = i;
    }

    char part[20] = {0};
    if (end >= len) {
      strncpy(part, full + pos, len - pos);
      pos = len;
    } else if (lastSpace != -1) {
      strncpy(part, full + pos, lastSpace - pos);
      pos = lastSpace + 1;
    } else {
      strncpy(part, full + pos, end - pos);
      pos = end;
    }

    trim(part);
    if (pos < len) strcat(part, "  ");

    matrix.displayClear();
    matrix.displayReset();
    matrix.setTextAlignment(PA_CENTER);
    matrix.displayText(part, PA_CENTER, 55, 400, eff, eff);

    unsigned long start = millis();
    while (!matrix.displayAnimate() && millis() - start < 3000);
    delay(300);
  }
}

// ===== HỖ TRỢ =====
void trim(char* s) {
  char* p = s;
  while (*p == ' ') p++;
  if (p != s) {
    char* q = s;
    while (*p) *q++ = *p++;
    *q = '\0';
  }
  p = s + strlen(s) - 1;
  while (p >= s && *p == ' ') *p-- = '\0';
}

void my_strupr(char* s) {
  while (*s) {
    *s = toupper(*s);
    s++;
  }
}