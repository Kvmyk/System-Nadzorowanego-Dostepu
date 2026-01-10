#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
//#include <Servo.h>

// --- KONFIGURACJA EKRANU ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- KONFIGURACJA PINÓW ---
const int SERVO = 6;  // PD6
const int BUZZER = 3;        // PD3

// RFID
const int RST_PIN = 9;
const int SS_PIN = 10;

MFRC522 rfid(SS_PIN, RST_PIN);

// --- KOMUNIKACJA Z ESP32 ---
SoftwareSerial espSerial(2, 4);

// --- SERWO ---
//Servo myServo;

// --- FUNKCJE DŹWIĘKOWE I EKRANOWE ---

void pokazKomunikat(String linia1, String linia2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Kontrola dostepu"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE); 
  
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(linia1);
  
  if (linia2 != "") {
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println(linia2);
  }
  display.display();
}

void dzwiekSukces() {
  tone(BUZZER, 659); delay(100);
  tone(BUZZER, 784); delay(100);
  noTone(BUZZER);
  PORTD |= (1 << PD3); // Buzzer HIGH
}

void dzwiekBlad() {
  tone(BUZZER, 200); delay(250);
  noTone(BUZZER);
  PORTD |= (1 << PD3); // Buzzer HIGH
}


void otworzDrzwi() {
  // Symulacja otwarcia drzwi za pomocą diody (PWM)
  // Powoli zwiększaj jasność diody (symulacja otwierania)
  for (int brightness = 0; brightness <= 255; brightness += 5) {
    analogWrite(SERVO, brightness);
    delay(30); // Zmienia jasność co 30ms
  }
  
  delay(5000); // Czekaj 5 sekund z otwartymi drzwiami
  
  // Powoli zmniejszaj jasność diody (symulacja zamykania)
  for (int brightness = 255; brightness >= 0; brightness -= 5) {
    analogWrite(SERVO, brightness);
    delay(30);
  }
  // Kod dla faktycznego serwa 
  /*
  myServo.attach(6);
  myServo.write(90);     // Otwórz na 90°
  delay(5000);
  myServo.write(0);      // Zamknij na 0°
  myServo.detach();
  */
}
void setup() {
  // Konfiguracja pinów jako OUTPUT przez rejestry
  //PD5 (pin 5), PD3 (pin 3)
  DDRD |= (1 << DDD5) | (1 << DDD3);
  
  // Ustaw wszystkie na HIGH (wyłączone)
  PORTD |= (1 << PD5) | (1 << PD3);

  Serial.begin(9600);     
  delay(100);
  
  espSerial.begin(9600);  
  delay(100);
  
  SPI.begin();
  rfid.PCD_Init();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Błąd SSD1306!"));
    for(;;); 
  }
  
  Serial.println(F("System gotowy!"));
  pokazKomunikat("PRZYLOZ", "KARTE...");
}

void loop() {
  // Czekaj na nową kartę
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Pobierz ID
  String content = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  String idKarty = content.substring(1);

  Serial.print(F("Karta: "));
  Serial.println(idKarty);

  // --- LOGIKA DOSTĘPU ---
  if (idKarty == "1B 5B 83 32") {
    
    // Wyślij do ESP32
    espSerial.print("SUKCES|");
    espSerial.println(idKarty);
    Serial.println(F("→ Wysłano SUKCES do ESP32"));
    
    // Obsługa lokalna
    pokazKomunikat("DOSTEP", "PRZYZNANY");
    display.invertDisplay(true); 
    
    // LED ZIELONY ON (LOW) przez rejestr
    PORTD &= ~(1 << PD5);
    
    dzwiekSukces();
    
    otworzDrzwi();

    delay(2000); 
    
    display.invertDisplay(false);
    
    // LED ZIELONY OFF (HIGH) przez rejestr
    PORTD |= (1 << PD5);
  }
  else {
    // Wyślij do ESP32
    espSerial.print("ODMOWA|");
    espSerial.println(idKarty);
    Serial.println(F("→ Wysłano ODMOWA do ESP32"));
    
    // Obsługa lokalna
    pokazKomunikat("ZAKAZ", "DOSTEPU!");
 
    dzwiekBlad();
    
    delay(2000);
    
  }

  // Powrót do stanu czuwania
  pokazKomunikat("PRZYLOZ", "KARTE...");
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
