#include <ThreeWire.h>  
#include <RtcDS1302.h>

// --- KONFIGURACJA PINÓW RTC DS1302 ---
#define RTC_CLK_PIN 18
#define RTC_DAT_PIN 19
#define RTC_RST_PIN 21

ThreeWire myWire(RTC_DAT_PIN, RTC_CLK_PIN, RTC_RST_PIN);
RtcDS1302<ThreeWire> Rtc(myWire);

// --- KOMUNIKACJA UART ---
#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(9600);
  delay(100);
  
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("\n=================================");
  Serial.println("System logowania ESP32");
  Serial.println("=================================");

  // --- KONFIGURACJA RTC ---
  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);
  Rtc.SetIsRunning(true);
  
  // ZAWSZE ustaw czas kompilacji
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetDateTime(compiled);
  
  Serial.print("Czas ustawiony na: ");
  printDateTime(compiled);
  
  Serial.println("\nNasłuchuję danych z Arduino...");
  Serial.println("=================================\n");
}

void loop() {
  if (Serial2.available()) {
    String odebraneDane = Serial2.readStringUntil('\n');
    odebraneDane.trim();
    
    if (odebraneDane.length() > 0) {
      RtcDateTime now = Rtc.GetDateTime();
      printLog(now, odebraneDane);
    }
  }
}

void printLog(const RtcDateTime& dt, String info) {
  char datestring[20];
  
  snprintf_P(datestring, 
          countof(datestring),
          PSTR("%04u-%02u-%02u %02u:%02u:%02u"),
          dt.Year(),
          dt.Month(),
          dt.Day(),
          dt.Hour(),
          dt.Minute(),
          dt.Second());

  Serial.print("[LOG] ");
  Serial.print(datestring);
  Serial.print(" | ");
  Serial.println(info);
}

void printDateTime(const RtcDateTime& dt) {
  char datestring[20];
  
  snprintf_P(datestring, 
          countof(datestring),
          PSTR("%04u-%02u-%02u %02u:%02u:%02u"),
          dt.Year(),
          dt.Month(),
          dt.Day(),
          dt.Hour(),
          dt.Minute(),
          dt.Second());
  
  Serial.println(datestring);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
