#include <EEPROM.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);
    int index = 0;
  while (index < EEPROM.length()) {
    Serial.print(EEPROM.read(index));
    Serial.print("\t");
    index++;
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
