#include <LiquidCrystal_I2C.h>
#include "Wheels.h"
#include "TimerOne.h"

#define BEEPER 13

// LCD
LiquidCrystal_I2C lcd(0x3f, 16, 2);

Wheels w;
volatile char cmd;

// ====== BEEPER ======
long int intPeriod = 500000;

void doBeep() {
  digitalWrite(BEEPER, digitalRead(BEEPER) ^ 1);
}

void startBeep(long period) {
  Timer1.attachInterrupt(doBeep, period);
}

void stopBeep() {
  Timer1.detachInterrupt();
  digitalWrite(BEEPER, LOW);
}

// ====== LCD ======
void aktualizujLCD(int pozostalo, int mocL, int mocP) {
  lcd.setCursor(0, 0);
  lcd.print("Zostalo: ");
  char buf[9];
  sprintf(buf, "%4dcm ", pozostalo);
  lcd.print(buf);

  lcd.setCursor(0, 1);

  char bufLewy[5];
  sprintf(bufLewy, "%4d", mocL);
  lcd.print(bufLewy);

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

  for (int i = 4; i <= 11; i++) {
    lcd.setCursor(i, 1);
    if (sredniaMoc == 0) {
      lcd.print((i == 7 || i == 8) ? '-' : ' ');
    } else if (i == pozycjaAnimacji) {
      lcd.print(sredniaMoc > 0 ? '>' : '<');
    } else {
      lcd.print(' ');
    }
  }

  char bufPrawy[5];
  sprintf(bufPrawy, "%4d", mocP);
  lcd.setCursor(12, 1);
  lcd.print(bufPrawy);
}

void setup() {
  lcd.init();
  lcd.backlight();
  aktualizujLCD(0, 0, 0);

  w.attach(7,12,3,9,11,5);

  pinMode(BEEPER, OUTPUT);
  Timer1.initialize();

  Serial.begin(9600);
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
      case 's': w.stop(); stopBeep(); aktualizujLCD(0,0,0); break;

      case '1': w.setSpeedLeft(75); break;
      case '2': w.setSpeedLeft(200); break;
      case '9': w.setSpeedRight(75); break;
      case '0': w.setSpeedRight(200); break;
      case '5': w.setSpeed(100); break;

      case 'f': w.goForward(10); break;
      case 'b': w.goBack(10); break;
    }
  }
}
