#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  Serial.println("Clearing EEPROM...");

  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);  // or 0x00 if you want to reset all to zero
  }

  Serial.println("EEPROM cleared!");
}

void loop() {
}
