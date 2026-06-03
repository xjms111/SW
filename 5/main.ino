#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "Wheels.h"
#include "Blinker.h"
#include "SpeedSensor.h"

// Przypisanie pinów zgodnie z Twoim shieldem (zieloną płytką)
#define TRIG 9
#define ECHO 10
#define SERVO 3

LiquidCrystal_I2C lcd(0x27, 16, 2);
Wheels w;
Servo serwo;

int lastSpeedLeft = 0, lastSpeedRight = 0;
int lastDist = 0, lastMode = -1; 
unsigned long lastMove = 0;
char cmd;

// Obsługa radaru
int radarAngle = 90;
int radarDirection = 15; // Zwiększony krok dla żywszego omiatania
bool obstacleDetected = false;
unsigned int currentDist = 400;

// Klasa Ticker do wielozadaniowości
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

Ticker ticker(100, updateLCD);     // Odświeżanie ekranu co 100ms
Ticker speedTicker(32, speedChange);
Ticker radarTicker(60, scanRadar);  // Skanowanie otoczenia co 60ms

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

void scanRadar() {
  if (obstacleDetected) return; // Jeśli trwa procedura omijania, zawieś wahadło

  serwo.write(radarAngle);
  currentDist = getSonarDistance();

  // Wysyłanie danych na Serial Plotter
  Serial.print("Angle:"); Serial.print(radarAngle); Serial.print(",");
  Serial.print("Distance:"); Serial.println(currentDist);

  // REAKCJA ANTYKOLIZYJNA: Robot jedzie w przód i widzi przeszkodę < 25cm na wprost (70-110 stopni)
  if (w.getMoving() && w.getForw() && radarAngle >= 70 && radarAngle <= 110 && currentDist < 25) {
    w.stop(); 
    obstacleDetected = true;
    
    // PUNKT: Podejmowanie decyzji (Skanowanie stron)
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("OBIEKT! SKANUJĘ");
    
    serwo.write(30); delay(500); unsigned int leftSpace = getSonarDistance();
    serwo.write(150); delay(500); unsigned int rightSpace = getSonarDistance();
    serwo.write(90); delay(200); // Powrót sonaru na wprost
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WYBRANA DROGA:");
    
    // PUNKT: Realizacja manewru ucieczki na podstawie decyzji
    if(leftSpace > rightSpace) {
      lcd.setCursor(0, 1); lcd.print(">>> LEWO <<<");
      w.setSpeed(160);
      w.turnLeft(60); // Fizyczny skręt w lewo o bezpieczny kąt
    } else {
      lcd.setCursor(0, 1); lcd.print(">>> PRAWO <<<");
      w.setSpeed(160);
      w.turnRight(60); // Fizyczny skręt w prawo
    }
    
    lastMove = millis(); // Zapamiętaj czas rozpoczęcia skrętu
    return;
  }

  // Ruch wahadłowy radaru
  radarAngle += radarDirection;
  if (radarAngle >= 160 || radarAngle <= 20) {
    radarDirection = -radarDirection; 
  }
}

// PUNKT: Wyświetlanie danych z sonaru (Kąt i odległość) na LCD w czasie rzeczywistym
void updateLCD() {
  if (obstacleDetected) return; // Podczas alarmu nie nadpisuj ekranu decyzji

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
    // Wyświetlanie prędkości na skrajach ekranu, jeśli jest taka potrzeba
    lastSpeedLeft = sl; lastSpeedRight = sr;
  }
}

void setup() {
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  Serial.begin(9600);
  
  // Konfiguracja kół
  w.attach(11, 12, 6, 8, 7, 5);
  Blinker::begin(13); 
  SpeedSensor::begin();
  
  lcd.init();
  lcd.backlight();
  
  serwo.attach(SERVO);
  serwo.write(90); 
  
  delay(1000);
  
  // URUCHOMIENIE AUTKA: Automatycznie rusza przed siebie po włączeniu zasilania
  w.setSpeed(140);
  w.goForward(999); // Komenda dalekiej jazdy, sterowanej teraz przez sztuczną inteligencję sonaru
}

void loop() {
  ticker.check();
  speedTicker.check();
  radarTicker.check(); 

  w.moveStep();
  w.turnStep();

  // PUNKT: Kontynuowanie jazdy po zakończeniu manewru omijania
  if (obstacleDetected && !w.getTurning() && (millis() - lastMove > 1500)) {
    obstacleDetected = false; 
    lcd.clear();
    w.setSpeed(140);
    w.goForward(999); // Skręt wykonany, droga czysta -> jedź dalej przed siebie!
  }
}
