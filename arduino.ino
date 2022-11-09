#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Constants.h>
#include <MFRC522Debug.h>
#include <rgb_lcd.h>
#include <Servo.h>
#include <EEPROM.h>
#include <string.h>

Servo myservo;
rgb_lcd lcd;

MFRC522DriverPinSimple ss_pin(10);
MFRC522DriverSPI driver{ss_pin};
MFRC522 reader{driver};

const int switchPin = 4;
bool deletemode = false;
const int deletebuttonPin = 3;
const int deleteledPin = 2;

int pos = 0;

boolean match = false;          // initialize card match to false
byte storedCard[4];    // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM

MFRC522Constants::Uid uid;

void setup() {
  lcd.begin(16, 2);
  myservo.attach(5);
  myservo.write(10);
  Serial.begin(115200);
  while (!Serial);
  reader.PCD_Init();
  Serial.print(F("Reader: "));
  MFRC522Debug::PCD_DumpVersionToSerial(reader, Serial);
  pinMode(switchPin, INPUT_PULLUP);
  delay(5000);
  Serial.print(F("Start"));
  pinMode(deletebuttonPin, INPUT);
  pinMode(deleteledPin, OUTPUT);
  digitalWrite(deleteledPin, deletemode);
  attachInterrupt(deletebuttonPin, delete_mode, FALLING);
}

void loop() {
  if (deletemode) {
    uid = read_card();
    deleteID(uid.uidByte);
    delay(5000);
  } else if (digitalRead(switchPin) == HIGH) read_mode();
  else write_mode();
}

void delete_mode() {
  deletemode = !deletemode;
  Serial.print(F("Delete Mode"));
  digitalWrite(deleteledPin, deletemode);
}

void read_mode() {
  uid = read_card();
  if (findID(uid.uidByte)) open_the_door();
}

void write_mode() {
  uid = read_card();
  writeID(uid.uidByte);
  delay(5000);
}

MFRC522Constants::Uid read_card() {
  if (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial()) {
    Serial.print(F("Card UID:"));
    MFRC522Debug::PrintUID(Serial, reader.uid);
    lcd.setCursor(0, 0);
    MFRC522Debug::PrintUID(lcd, reader.uid);

    Serial.println();

    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = reader.PICC_GetType(reader.uid.sak);
    Serial.println(MFRC522Debug::PICC_GetTypeName(piccType));
    lcd.setCursor(0, 1);
    lcd.print(MFRC522Debug::PICC_GetTypeName(piccType));

    reader.PICC_HaltA();
    reader.PCD_StopCrypto1();
    return reader.uid;
  } else {
    MFRC522Constants::Uid nul;
    return nul;
  }
}

void open_the_door() {
  myservo.write(70);
  delay(5000);
  myservo.write(10);
  delay(1000);
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * 4 ) + 2;     // Figure out starting position
  for ( int i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    int start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( int j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else {
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    int num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    int slot;       // Figure out the slot number of the card
    int start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    int looping;    // The number of times the loop repeats
    int j;
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( int k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if (a[0])       // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( int k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
  int count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
  return -1;
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  int count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}
