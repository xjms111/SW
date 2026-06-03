#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "Wheels.h"
#include "Blinker.h"
#include "SpeedSensor.h"

// Upewnij się, że te piny są poprawne dla Twojego shielda!
#define TRIG 9
#define ECHO 10
#define SERVO 3

LiquidCrystal_I2C lcd(0x27, 16, 2); 
Wheels w;
Servo serwo;

int lastSpeedLeft = 0, lastSpeedRight = 0;

// Zmienne echoradaru
int radarAngle = 90;
int radarDirection = 15; 
unsigned int currentDist = 400;

class Ticker {
  private: 
    unsigned long period;
    unsigned long previousStart;
    void (*trigger)(void);
  public: 
    Ticker(unsigned long p, void(*fun)(void)) { period = p; trigger = fun; previousStart = 0; }
    bool check() {
      unsigned long timeNow = millis();
      if(timeNow - previousStart > period) { (*trigger)(); previousStart = timeNow; return true; }
      return false;
    }
};

void updateLCD();
void speedChange();
void scanRadar();

Ticker ticker(100, updateLCD);       // Odświeżanie ekranu co 100ms
Ticker speedTicker(32, speedChange);
Ticker radarTicker(50, scanRadar);    // Skanowanie otoczenia co 50ms

// Funkcja pomiaru odległości
unsigned int getSonarDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long tot = pulseIn(ECHO, HIGH, 25000); 
  if (tot == 0) return 400; 
  return tot / 58;
}

// Funkcja skanowania przestrzeni
void scanRadar() {
  // Fizyczny ruch serwa o kolejny krok
  serwo.write(radarAngle);
  currentDist = getSonarDistance();

  // REAKCJA ANTYKOLIZYJNA: 
  if (w.getMoving() && w.getForw() && radarAngle >= 70 && radarAngle <= 110 && currentDist < 25) {
    
    w.stop(); // Natychmiast zatrzymaj silniki
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("OBIEKT! SKANUJE");
    
    // PROCEDURA ROZEJRZENIA SIĘ
    serwo.write(30); delay(500); unsigned int leftSpace = getSonarDistance();
    serwo.write(150); delay(500); unsigned int rightSpace = getSonarDistance();
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WYBRANA DROGA:");
    
    // RĘCZNE WYMUSZENIE SKRĘTU CZASOWEGO (Niezależne od enkoderów)
    if(leftSpace > rightSpace) {
      lcd.setCursor(0, 1); lcd.print(">>> LEWO <<<");
      w.setSpeedLeft(160);  w.setSpeedRight(160);
      // Aby skręcić w lewo: lewe koła w tył, prawe koła w przód
      analogWrite(6, 0);   // Lewy przód STOP
      analogWrite(8, 160); // Lewy tył START
      analogWrite(7, 160); // Prawy przód START
      analogWrite(5, 0);   // Prawy tył STOP
    } else {
      lcd.setCursor(0, 1); lcd.print(">>> PRAWO <<<");
      w.setSpeedLeft(160);  w.setSpeedRight(160);
      // Aby skręcić w prawo: lewe koła w przód, prawe koła w tył
      analogWrite(6, 160); // Lewy przód START
      analogWrite(8, 0);   // Lewy tył STOP
      analogWrite(7, 0);   // Prawy przód STOP
      analogWrite(5, 160); // Prawy tył START
    }
    
    // Czas trwania fizycznego skrętu (600 ms). 
    // Jeśli robot skręca za mało, zwiększ do 800. Jeśli za dużo, zmniejsz do 400.
    delay(600); 
    
    w.stop(); // Zakończ manewr skręcania
    delay(100);
    
    // PO SKRĘCIE: Wymuszamy natychmiastowy powrót do jazdy i odblokowanie serwa
    serwo.write(90);
    radarAngle = 90;
    lcd.clear();
    
    // Ruszamy ponownie przed siebie za pomocą biblioteki
    w.setSpeed(140);
    w.goForward(999); 
    
    return; 
  }

  // Zwykły ruch wahadłowy serwa (wykonuje się zawsze, gdy droga jest czysta)
  radarAngle += radarDirection;
  if (radarAngle >= 160 || radarAngle <= 20) {
    radarDirection = -radarDirection; 
  }
}

// Wyświetlanie danych z sonaru w czasie rzeczywistym
void updateLCD() {
  if (w.getMoving() && w.getForw()) {
    lcd.setCursor(0, 0);
    lcd.print("Kat: "); lcd.print(radarAngle); lcd.print("deg   ");
    
    lcd.setCursor(0, 1);
    lcd.print("Dyst: ");
    if(currentDist == 400) {
      lcd.print("CLEAR   ");
    } else {
      lcd.print(currentDist); lcd.print("cm    ");
    }
  }
}

void speedChange() {
  int sl = 0, sr = 0;
  if (w.getMoving()) {
    sl = w.getSpeedLeft(); sr = w.getSpeedRight();
    if (!w.getForw()) { sl *= -1; sr *= -1; }
  } else if (w.getTurning()) {
    sl = 180; sr = 180;
    if (w.getLeft()) sl *= -1; else sr *= -1;
  }
  if(lastSpeedRight != sr || lastSpeedLeft != sl){
    lastSpeedLeft = sl; lastSpeedRight = sr;
  }
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  Serial.begin(9600);
  
  // Konfiguracja pinów silników w bibliotece:
  // pins: forward Left, backward Left, forward Right, backward Right, plus piny enable/speed (7 i 5)
  w.attach(11, 12, 6, 8, 7, 5);
  Blinker::begin(13); 
  SpeedSensor::begin();
  
  lcd.init();
  lcd.backlight();
  
  serwo.attach(SERVO);
  serwo.write(90); 
  delay(1000);
  
  // Start robota
  w.setSpeed(140);
  w.goForward(999); 
}

void loop() {
  ticker.check();
  speedTicker.check();
  radarTicker.check(); 

  // Te funkcje muszą być, ale nie będą już blokować powrotu do jazdy
  w.moveStep();
  w.turnStep();
}
