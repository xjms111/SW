#include <LiquidCrystal_I2C.h>
#include "TimerOne.h"
#include "Wheels.h"

// Inicjalizacja LCD (adres 0x3f, 16 kolumn, 2 wiersze)
LiquidCrystal_I2C lcd(0x3f, 16, 2);

#define BEEPER 13
long int intPeriod = 500000;

Wheels w;
volatile char cmd;

// Funkcja odpowiedzialna za rysowanie interfejsu (dystans + deska rozdzielcza)
void aktualizujLCD(int pozostalo, int mocL, int mocP) {
  // --- 1. LINIA: Pozostały dystans ---
  lcd.setCursor(0, 0);
  lcd.print("Zostalo: ");
  char buf[9];
  sprintf(buf, "%4dcm ", pozostalo); // Formatowanie do 4 znaków, żeby usunąć "śmieci"
  lcd.print(buf);

  // --- 2. LINIA: Deska rozdzielcza ---
  lcd.setCursor(0, 1);
  
  // Lewy silnik
  char bufLewy[5];
  sprintf(bufLewy, "%4d", mocL);
  lcd.print(bufLewy);
  
  // Animacja na środku
  int sredniaMoc = (mocL + mocP) / 2;
  static int pozycjaAnimacji = 4;
  static unsigned long ostatniCzasAnimacji = 0;
  
  if (millis() - ostatniCzasAnimacji > 150) {
    if (sredniaMoc > 0) {
      pozycjaAnimacji++;
      if (pozycjaAnimacji > 11) pozycjaAnimacji = 4;
    } else if (sredniaMoc < 0) {
      pozycjaAnimacji--;
      if (pozycjaAnimacji < 4) pozycjaAnimacji = 11;
    }
    ostatniCzasAnimacji = millis();
  }
  
  // Rysowanie znaków animacji
  for (int i = 4; i <= 11; i++) {
    lcd.setCursor(i, 1);
    if (sredniaMoc == 0) {
      lcd.print((i == 7 || i == 8) ? '-' : ' '); // Silniki stoją
    } else if (i == pozycjaAnimacji) {
      lcd.print(sredniaMoc > 0 ? '>' : '<');     // Kierunek jazdy
    } else {
      lcd.print(' ');
    }
  }
  
  // Prawy silnik
  char bufPrawy[5];
  sprintf(bufPrawy, "%4d", mocP);
  lcd.setCursor(12, 1);
  lcd.print(bufPrawy);
}

void TimerUpdate() {
  Timer1.detachInterrupt();
  Timer1.attachInterrupt(doBeep, intPeriod);
}

// zmienia wartość pinu BEEPER
void doBeep() {
  digitalWrite(BEEPER, digitalRead(BEEPER) ^ 1);
}

void setup() {
  pinMode(BEEPER, OUTPUT);
  Timer1.initialize();
  TimerUpdate();

  
  lcd.init();
  lcd.backlight();
  aktualizujLCD(0, 0, 0); // Stan początkowy po włączeniu
  
  w.attach(7,12, 3,9,11,5);
  
  Serial.begin(9600);
  Serial.println("Forward: WAD");
  Serial.println("Back: ZXC");
  Serial.println("Stop: S");
}

void loop() {
  while(Serial.available())
  {
    cmd = Serial.read();
    switch(cmd)
    {
      case 'w': w.forward(); break;
      case 'x': w.back(); break;
      case 'a': w.forwardLeft(); break;
      case 'd': w.forwardRight(); break;
      case 'z': w.backLeft(); break;
      case 'c': w.backRight(); break;
      case 's': w.stop(); aktualizujLCD(0, 0, 0); break; 
      case '1': w.setSpeedLeft(75); break;
      case '2': w.setSpeedLeft(200); break;
      case '9': w.setSpeedRight(75); break;
      case '0': w.setSpeedRight(200); break;
      case '5': w.setSpeed(100); break;

      case 'f': delay(500); w.goForward(100); break;
      case 'b': delay(500); w.goBack(100); TimerUpdate(); Serial.println(intPeriod); Serial.println(intPeriod);break;
    }
  }
}
