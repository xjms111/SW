#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include "Wheels.h"
#include "Blinker.h"
#include "SpeedSensor.h"

// Konfiguracja pinów sonaru i serwa (Bezpieczne piny)
#define TRIG 9
#define ECHO 10
#define SERVO 3

// Inicjalizacja obiektów globalnych
LiquidCrystal_I2C lcd(0x27, 16, 2);
Wheels w;
Servo serwo;

// Zmienne globalne dla obsługi ekranu i algorytmu
int lastSpeedLeft = 0, lastSpeedRight = 0;
int lastDist = 0, lastMode = -1; 
unsigned long lastMove = 0;
unsigned long interval = 4000; 
int fr = 0; 
char cmd;

// Zmienne obsługi radaru (Serwo + Sonar) bez delay()
int radarAngle = 90;
int radarDirection = 10; // Krok zmiany kąta (+10 lub -10)
bool obstacleDetected = false;

// Definicje własnych znaków (Strzałki do animacji)
uint8_t arrowUp[8] = {0b00100, 0b01110, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
uint8_t arrowDown[8] = {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111, 0b01110, 0b00100};
uint8_t arrowLeft[8] = {0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010, 0b00000};
uint8_t arrowRight[8] = {0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000, 0b00000};
uint8_t current[8];
uint8_t frame[8];

// Klasa Ticker (Programowy stoper)
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

// Deklaracje zapowiedzi funkcji
void an();
void speedChange();
void animation();
void scanRadar();

// Rejestracja stoperów
Ticker ticker(32, an);
Ticker speedTicker(32, speedChange);
Ticker animationTicker(150, animation);
Ticker radarTicker(80, scanRadar); // Zmiana kąta radaru co 80ms

// Funkcje pomocnicze animacji matrycy
void shiftUp(uint8_t *src, uint8_t *dst) { for(int i=0; i<7; i++) dst[i] = src[i+1]; dst[7] = src[0]; }
void shiftDown(uint8_t *src, uint8_t *dst) { dst[0] = src[7]; for(int i=1; i<8; i++) dst[i] = src[i-1]; }
void startLineAnimation(bool up) { memcpy(current, up ? arrowUp : arrowDown, 8); }

// Funkcja pomiaru odległości sonaru
unsigned int getSonarDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long tot = pulseIn(ECHO, HIGH, 25000); // Timeout 25ms
  if (tot == 0) return 400; 
  return tot / 58;
}

// Funkcja asynchronicznego skanowania (Wywoływana przez radarTicker co 80ms)
void scanRadar() {
  serwo.write(radarAngle);
  unsigned int currentDist = getSonarDistance();

  // Wysłanie danych do Narzędzia -> Kreślarka (Plotter)
  Serial.print("Angle:"); Serial.print(radarAngle); Serial.print(",");
  Serial.print("Distance:"); Serial.println(currentDist);

  // Jeśli robot jedzie do przodu i wykryje przeszkodę bliżej niż 20cm na wprost (70-110 deg)
  if (w.getMoving() && w.getForw() && radarAngle >= 70 && radarAngle <= 110 && currentDist < 20) {
    w.stop(); // Natychmiastowe zatrzymanie silników i uciszenie beepera
    obstacleDetected = true;
    
    // Skanowanie decyzji (Sekwencja szybka sprzętowo)
    serwo.write(30); delay(400); unsigned int leftSpace = getSonarDistance();
    serwo.write(150); delay(400); unsigned int rightSpace = getSonarDistance();
    serwo.write(90); // Powrót
    
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("OBIEKT! HAMOWANIE");
    lcd.setCursor(0, 1);
    if(leftSpace > rightSpace) lcd.print("Droga: LEWO     ");
    else lcd.print("Droga: PRAWO    ");
    
    lastMove = millis() + 2000; // Zablokuj automat na 2 sekundy, aby odczytać LCD
    return;
  }

  // Aktualizacja kąta na następny cykl (wahadło 20 <-> 160 stopni)
  radarAngle += radarDirection;
  if (radarAngle >= 160 || radarAngle <= 20) {
    radarDirection = -radarDirection; // Odwrócenie kierunku obrotu serwa
  }
}

// Odświeżanie górnej linii LCD
void an() {
  if (obstacleDetected) {
    if (millis() - lastMove > 0) obstacleDetected = false; // Po czasie wyczyść stan alarmu
    return;
  }
  
  if (w.getMoving()) {
    int tmp = w.distance - w.distanceTravelled;
    if(lastDist != tmp || lastMode != 1){
      lcd.setCursor(0, 0); lcd.print("Dystans:    cm  ");
      lcd.setCursor(9, 0); lcd.print(tmp);
      lastDist = tmp; lastMode = 1;
    }
  } else if (w.getTurning()) {
    if (lastMode != 2) { lcd.setCursor(0, 0); lcd.print("Status: OBROT    "); lastMode = 2; }
  } else {
    if (lastMode != 0) { lcd.setCursor(0, 0); lcd.print("Status: STOP     "); lastMode = 0; }
  }
}

// Aktualizacja prędkości w rogach LCD
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
    lcd.setCursor(0, 1); lcd.print("    "); lcd.setCursor(0, 1); lcd.print(sl);
    lcd.setCursor(12, 1); lcd.print("    "); lcd.setCursor(12, 1); lcd.print(sr);
    lastSpeedLeft = sl; lastSpeedRight = sr;
  }
}

// Dynamiczne strzałki / Kierunkowskazy na środku LCD
void animation() {
  if (obstacleDetected) return;
  lcd.setCursor(7, 1);
  if(w.getMoving()){
    lcd.createChar(0, current);
    lcd.write(byte(0)); lcd.write(byte(0));
    if (w.getForw()) { shiftUp(current, frame); memcpy(current, frame, 8); } 
    else { shiftDown(current, frame); memcpy(current, frame, 8); }
  } else if(w.getTurning()){
    static bool flash = false;
    flash = !flash;
    if (flash) {
      lcd.createChar(1, w.getLeft() ? arrowLeft : arrowRight);
      lcd.write(byte(1)); lcd.write(byte(1));
    } else { lcd.print("  "); }
  } else { lcd.print("--"); }
}

void setup() {
  // Piny Sonaru
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  Serial.begin(9600);
  
  // Inicjalizacja kół (Piny: R_forward, R_back, R_speed, L_forward, L_back, L_speed)
  w.attach(12, 11, 6, 7, 8, 5);
  Blinker::begin(13); // Głośnik na pinie 13
  SpeedSensor::begin();
  
  lcd.init();
  lcd.backlight();
  
  serwo.attach(SERVO);
  serwo.write(90); // Ustawienie radaru na wprost
  
  lastMove = millis() + 1000; // Pierwsza sekunda bezpiecznego postoju
}

void loop() {
  // Wywołanie asynchronicznych stoperów sprzętu i interfejsu
  ticker.check();
  speedTicker.check();
  animationTicker.check();
  radarTicker.check(); // Silnik radaru działa bezustannie w tle

  // Strażnicy asynchronicznych pomiarów odległości kół i kątów
  w.moveStep();
  w.turnStep();

  // Automat sekwencyjny (taniec robota) - działa tylko gdy brak awarii sonaru
  if(!obstacleDetected && (millis() - lastMove > interval)){
    switch(fr){
      case 0: w.turnLeft(90); break;
      case 1: w.setSpeed(130); w.goBack(50); startLineAnimation(false); break;
      case 2: w.turnRight(90); break;
      case 3: w.setSpeed(250); w.goBack(50); startLineAnimation(false); break;
    }
    fr = (fr + 1) % 4;
    lastMove = millis();
  }

  // Obsługa komend awaryjnych z Serial Monitora
  if (Serial.available()) {
    cmd = Serial.read();
    switch (cmd) {
      case 'w': w.setSpeed(150); w.forward(); startLineAnimation(true); break;
      case 'x': w.setSpeed(150); w.back(); startLineAnimation(false); break;
      case 's': w.stop(); break;
    }
  }
}
