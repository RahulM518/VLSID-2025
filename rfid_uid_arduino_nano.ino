#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN  10
#define OUTPUT_PIN_2 2
#define OUTPUT_PIN_3 3
#define OUTPUT_PIN_4 4

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Define card IDs
byte ID1[] = {0xF3, 0xC3, 0x7D, 0x35};
byte ID2[] = {0x93, 0x77, 0x09, 0x35};
byte ID3[] = {0x63, 0x4B, 0x54, 0xF5}; // New card
byte ID4[] = {0x33, 0x28, 0xFD, 0xF6}; // New card
byte ID5[] = {0x23, 0x95, 0x42, 0xF7}; // New card
byte ID6[] = {0xE3, 0x8C, 0xF5, 0xF6}; // New card

void setup() {
  Serial.begin(9600);
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println(F("Scan a card to see its UID..."));

  // Set up pins
  pinMode(OUTPUT_PIN_2, OUTPUT);
  pinMode(OUTPUT_PIN_3, OUTPUT);
  pinMode(OUTPUT_PIN_4, OUTPUT);

  // Ensure all output pins are low at startup
  digitalWrite(OUTPUT_PIN_2, LOW);
  digitalWrite(OUTPUT_PIN_3, LOW);
  digitalWrite(OUTPUT_PIN_4, LOW);
}

void loop() {
  // Check for a new card
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select the card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Print the UID
  Serial.print(F("Card UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Default: make pin 2 high
  digitalWrite(OUTPUT_PIN_2, HIGH);

  // Check UID against predefined IDs and set pins accordingly
  if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size, ID1, sizeof(ID1))) {
    Serial.println(F("ID1 recognized."));
    digitalWrite(OUTPUT_PIN_3, HIGH); // Activate pin 3 for ID1
  } else if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size, ID2, sizeof(ID2))) {
    Serial.println(F("ID2 recognized."));
    digitalWrite(OUTPUT_PIN_4, HIGH); // Activate pin 4 for ID2
  } else {
    Serial.println(F("Unknown card."));
  }
  // Keep the pins HIGH for 2 seconds
  delay(2000);
 

  if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size, ID3, sizeof(ID3))) {
    Serial.println(F("ID3 recognized."));
    digitalWrite(OUTPUT_PIN_2, HIGH);  // Binary: 010
  } else if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size, ID4, sizeof(ID4))) {
    Serial.println(F("ID4 recognized."));
    digitalWrite(OUTPUT_PIN_2, HIGH);  
  } else if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size, ID5, sizeof(ID5))) {
    Serial.println(F("ID5 recognized."));
    digitalWrite(OUTPUT_PIN_2, HIGH);  // Binary: 011
  } else if (checkUID(mfrc522.uid.uidByte, mfrc522.uid.size, ID6, sizeof(ID6))) {
    Serial.println(F("ID6 recognized."));
    digitalWrite(OUTPUT_PIN_2, HIGH); // Binary: 111
  }else{
    Serial.println(F("Unknown checkpoint."));
  } 

  delay(2000);
   // Reset all pins to LOW
  digitalWrite(OUTPUT_PIN_2, LOW);
  digitalWrite(OUTPUT_PIN_3, LOW);
  digitalWrite(OUTPUT_PIN_4, LOW);
  // Halt the card to stop further processing
  mfrc522.PICC_HaltA();
}

// Function to check if the scanned UID matches a predefined UID
bool checkUID(byte *scannedUID, byte scannedSize, byte *knownUID, byte knownSize) {
  if (scannedSize != knownSize) {
    return false;
  }
  for (byte i = 0; i < scannedSize; i++) {
    if (scannedUID[i] != knownUID[i]) {
      return false;
    }
  }
  return true;
}
