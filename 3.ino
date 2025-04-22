#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RESET_PIN 9
#define SELECT_PIN 10

MFRC522 rfidReader(SELECT_PIN, RESET_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust I2C address if needed

unsigned long startMillis;
int startHours = 12, startMinutes = 55, startSeconds = 50;

String lastCardUID = "";

String getCurrentTime() {
  unsigned long elapsedSeconds = (millis() - startMillis) / 1000;
  int currentHours = startHours + (elapsedSeconds / 3600);
  int currentMinutes = startMinutes + ((elapsedSeconds % 3600) / 60);
  int currentSeconds = startSeconds + (elapsedSeconds % 60);

  if (currentSeconds >= 60) { currentMinutes += currentSeconds / 60; currentSeconds %= 60; }
  if (currentMinutes >= 60) { currentHours += currentMinutes / 60; currentMinutes %= 60; }
  if (currentHours >= 24) { currentHours %= 24; }

  char formattedTime[9];
  sprintf(formattedTime, "%02d:%02d:%02d", currentHours, currentMinutes, currentSeconds);
  return String(formattedTime);
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfidReader.PCD_Init();
  lcd.init();
  lcd.backlight();

  startMillis = millis();
  lcd.setCursor(0, 0);
  lcd.print("Scan your Card");
}

void loop() {
  if (!rfidReader.PICC_IsNewCardPresent() || !rfidReader.PICC_ReadCardSerial()) return;

  String cardNumber = "";
  for (byte i = 0; i < rfidReader.uid.size; i++) {
    cardNumber += String(rfidReader.uid.uidByte[i], HEX);
  }

  cardNumber.toLowerCase();
  lastCardUID = cardNumber;
  Serial.println(cardNumber);  // Send UID to Python

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card Scanned");
  lcd.setCursor(0, 1);
  lcd.print("Wait");

  unsigned long startTime = millis();
  while (!Serial.available()) {
    if (millis() - startTime > 7000) {  // 7 seconds timeout
      lcd.clear();
      lcd.print("Timeout");
      delay(2000);
      lcd.clear();
      lcd.print("Scan your Card");
      return;
    }
  }

  String response = Serial.readStringUntil('\n');
  response.trim();

  lcd.clear();
  if (response == "FACE_OK") {
    lcd.print("Access Granted");
  } else if (response == "FACE_FAIL") {
    lcd.print("Face Mismatch");
  } else if (response == "UNAUTH") {
    lcd.print("Unauthorized");
  } else {
    lcd.print("Unknown Resp");
  }

  delay(2000);
  lcd.clear();
  lcd.print("Scan your Card");

  rfidReader.PICC_HaltA();
  rfidReader.PCD_StopCrypto1();
}
