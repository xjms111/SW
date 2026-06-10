#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "Wheels.h"
#include "Blinker.h"
#include "SpeedSensor.h"


#define TRIG 9
#define ECHO 10
#define SERVO 2

LiquidCrystal_I2C lcd(0x27, 16, 2);
Wheels w;
Servo serwo;

int lastSpeedLeft = 0, lastSpeedRight = 0;
unsigned long lastMove = 0;

// Zmienne echoradaru
int radarAngle = 90;
int radarDirection = 15; 
bool obstacleDetected = false;
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
Ticker radarTicker(50, scanRadar);    // Skanowanie otoczenia co 60ms

// Funkcja pomiaru odległości
unsigned int getSonarDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long tot = pulseIn(ECHO, HIGH, 12000); 
  if (tot == 0) return 400; 
  return tot / 58;
}

// Funkcja skanowania przestrzeni
void scanRadar() {

  // normalne skanowanie otoczenia
  serwo.write(radarAngle);

  currentDist = getSonarDistance();

  Serial.print("Angle:");
  Serial.print(radarAngle);
  Serial.print(",");

  Serial.print("Distance:");
  Serial.println(currentDist);

  // wykrywanie przeszkód tylko wtedy,
  // gdy robot jedzie do przodu i nie wykonuje skrętu
  if (!w.getTurning()) {

    if (w.getMoving() &&
        w.getForw() &&
        radarAngle >= 70 &&
        radarAngle <= 110 &&
        currentDist < 30) {

      w.stop();

      obstacleDetected = true;
      serwo.attach(SERVO);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("OBIEKT! SKANUJE");

      // pomiar lewej strony
      serwo.write(30);
      delay(500);
      unsigned int leftSpace = getSonarDistance();

      // pomiar prawej strony
      serwo.write(150);
      delay(500);
      unsigned int rightSpace = getSonarDistance();

      // powrot na srodek
      serwo.write(90);
      delay(200);

      serwo.detach();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WYBRANA DROGA:");

      if (leftSpace > rightSpace) {

        lcd.setCursor(0, 1);
        lcd.print(">>> LEWO <<<");

        w.setSpeed(160);
        w.turnLeft(60);

      } else {

        lcd.setCursor(0, 1);
        lcd.print(">>> PRAWO <<<");

        w.setSpeed(160);
        w.turnRight(60);
      }
    
      delay(150);
      return;
    }
  }

  // ruch wahadlowy sonaru
  radarAngle += radarDirection;

  if (radarAngle >= 160 || radarAngle <= 20) {
    radarDirection = -radarDirection;
  }
}

// Wyświetlanie danych z sonaru w czasie rzeczywistym
void updateLCD() {
  if (obstacleDetected) return; // Podczas alarmu i skręcania nie nadpisuj ekranu decyzji

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
  
  w.attach(13, 12, 6, 7, 8, 5);
  Blinker::begin(13); 
  SpeedSensor::begin();
  
  lcd.init();
  lcd.backlight();
  
  serwo.attach(SERVO);
  serwo.write(90); 
  delay(1000);
  
  // Pierwsze uruchomienie robota - jedzie przed siebie
  w.setSpeed(140);
  w.goForward(999); 
}

void loop() {
  // Wywołania stoperów
  ticker.check();
  speedTicker.check();
  radarTicker.check(); 

  // Strażnicy pracy silników (zliczanie enkoderów)
  w.moveStep();
  w.turnStep();


  if (obstacleDetected && !w.getTurning()) {

    lcd.clear();

    obstacleDetected = false;

    w.setSpeed(140);
    w.goForward(999);
  }
}
