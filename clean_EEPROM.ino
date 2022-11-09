#include <EEPROM.h>

void setup() {
  int i = 0;
  while (i < EEPROM.length()) {
    EEPROM[i] = 0x00;
    i++;
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
