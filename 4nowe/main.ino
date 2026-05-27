#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Wheels.h"
#include "Blinker.h"
#include "SpeedSensor.h"

byte LCDAddress = 0x27;
LiquidCrystal_I2C lcd(LCDAddress, 16, 2);
Wheels w;
volatile char cmd;

// Zmienne do automatycznej sekwencji ruchu (Z kodu kolegi)
unsigned long lastMove = 0;
unsigned long interval = 4000; // Czas (w ms) na wykonanie jednego kroku (np. 4 sekundy)
int fr = 0;

// ==========================================
// KLASA TICKER
// ==========================================
class Ticker {
  private: 
    unsigned long period;
    unsigned long previousStart;
    void (*trigger)(void);
  public: 
    Ticker(unsigned long p, void(*fun)(void)) {
      period = p; trigger = fun; previousStart = 0;
    }
    bool check() {
      unsigned long timeNow = millis();
      if(timeNow - previousStart > period) {
        (*trigger)(); previousStart = timeNow; return true;
      }
      return false;
    }
};

// ==========================================
// GRAFIKA PIXEL-ART I ZMIENNE LCD
// ==========================================
int lastSpeedLeft = 0;
int lastSpeedRight = 0;
int lastDist = 0;
int lastMode = -1; // -1=brak, 0=stop, 1=jazda, 2=skret

uint8_t arrowUp[8] = { 0b00100, 0b01110, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
uint8_t arrowDown[8] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111, 0b01110, 0b00100 };
uint8_t arrowLeft[8] = { 0b00010, 0b00100, 0b01000, 0b11111, 0b01000, 0b00100, 0b00010, 0b00000 };
uint8_t arrowRight[8] = { 0b01000, 0b00100, 0b00010, 0b11111, 0b00010, 0b00100, 0b01000, 0b00000 };

uint8_t frame[8];
uint8_t current[8];

// ==========================================
// TICKER 1: Wyświetlanie statusu / odliczania dystansu
// ==========================================
void an() {
  if (w.getMoving()) {
    int tmp = w.distance - w.distanceTravelled;
    if(lastDist != tmp || lastMode != 1){
      lcd.setCursor(0, 0);
      lcd.print("Dystans:    cm  ");
      lcd.setCursor(9, 0);
      lcd.print(tmp);
      lastDist = tmp; lastMode = 1;
    }
  } else if (w.getTurning()) {
    if (lastMode != 2) {
      lcd.setCursor(0, 0);
      lcd.print("Status: OBROT    ");
      lastMode = 2;
    }
  } else {
    if (lastMode != 0) {
      lcd.setCursor(0, 0);
      lcd.print("Status: STOP     ");
      lastMode = 0;
    }
  }
}

// ==========================================
// TICKER 2: Dynamiczne znaki prędkości (Zadanie 2)
// ==========================================
void speedChange() {
  int sl = 0;
  int sr = 0;

  if (w.getMoving()) {
    sl = w.getSpeedLeft();
    sr = w.getSpeedRight();
    if (!w.getForw()) { sl *= -1; sr *= -1; } // Bieg wsteczny: oba minus
  } 
  else if (w.getTurning()) {
    sl = 180; sr = 180; // turnStep na sztywno wymusza prędkość 180
    if (w.getLeft()) {
      sl *= -1; // Skręt w lewo: lewe koło cofa (-), prawe jedzie w przód (+)
    } else {
      sr *= -1; // Skręt w prawo: lewe koło w przód (+), prawe cofa (-)
    }
  }

  if(lastSpeedRight != sr || lastSpeedLeft != sl){
    lcd.setCursor(0, 1); lcd.print("    ");
    lcd.setCursor(0, 1); lcd.print(sl);
    
    lcd.setCursor(12, 1); lcd.print("    ");
    lcd.setCursor(12, 1); lcd.print(sr);
    
    lastSpeedLeft = sl; lastSpeedRight = sr;
  }
}

// ==========================================
// TICKER 3: Animacja strzałek środkowych
// ==========================================
void shiftUp(uint8_t *src, uint8_t *dest) {
  for (int i = 0; i < 7; i++) dest[i] = src[i + 1];
  dest[7] = src[0]; 
}
void shiftDown(uint8_t *src, uint8_t *dest){
  for (int i = 7; i > 0; i--) dest[i] = src[i - 1];
  dest[0] = src[7];
}

void animation(){
  lcd.setCursor(7, 1);
  
  if(w.getMoving()){
    lcd.createChar(0, current);
    lcd.write(byte(0));
    lcd.write(byte(0));
    if (w.getForw()) {
      shiftUp(current, frame); memcpy(current, frame, 8);
    } else {
      shiftDown(current, frame); memcpy(current, frame, 8);
    }
  } 
  else if(w.getTurning()){
    // Migające strzałki boczne dla skrętu
    static bool flash = false;
    flash = !flash;
    if (flash) {
      lcd.createChar(1, w.getLeft() ? arrowLeft : arrowRight);
      lcd.write(byte(1)); lcd.write(byte(1));
    } else {
      lcd.print("  ");
    }
  } 
  else {
    lcd.print("--"); // Zatrzymany
  }
}

Ticker ticker(32, an);
Ticker speedTicker(32, speedChange);
Ticker animationTicker(150, animation);

// Helper do startu animacji liniowej
void startLineAnimation(bool forwardDirection) {
  if (forwardDirection) {
    lcd.createChar(0, arrowUp); memcpy(current, arrowUp, 8);
  } else {
    lcd.createChar(0, arrowDown); memcpy(current, arrowDown, 8);
  }
}

// ==========================================
// SETUP I LOOP
// ==========================================
void setup() {
  w.attach(12, 11, 6, 7, 8, 5); 
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  
  Blinker::begin(13);
  SpeedSensor::begin();

  w.setSpeed(150);
  unsigned long startTimeout = 1000; // 10 sekund na odpięcie kabla i ucieczkę
  lastMove = millis() + startTimeout;
  Serial.println("Robot gotowy. Uruchamiam automatyczną sekwencję...");
}

void loop() {
  // 1. Wielozadaniowe tickery działające w tle
  ticker.check();
  w.moveStep();
  w.turnStep();
  speedTicker.check();
  animationTicker.check();

  // 2. Automatyczna sekwencja z kodu kolegi (wywoływana co 'interval' milisekund)
  if(millis() - lastMove > interval){
    Serial.println("Uruchomienie kolejnego manewru automatycznego");
    switch(fr){
      case 0:
        Serial.println("Skręt w lewo o 90 stopni");
        w.turnLeft(90);
        break;
      case 2:
        Serial.println("Skręt w prawo o 90 stopni");
        w.turnRight(90);
        break;
      case 1:
        Serial.println("Cofanie - Szybkość 130");
        w.setSpeed(130);
        w.goBack(50);
        startLineAnimation(false);
        break;
      case 3:
        Serial.println("Cofanie - Szybkość 250");
        w.setSpeed(250);
        w.goBack(50);
        startLineAnimation(false);
        break;
    }
    fr = (fr + 1) % 4;
    lastMove = millis();
  }

  // 3. Opcjonalne sterowanie z klawiatury (nadpisuje automatykę, jeśli klikniesz)
  if (Serial.available()) {
    cmd = Serial.read();
    switch (cmd) {
      case 'w': w.forward(); startLineAnimation(true); break;
      case 'x': w.back(); startLineAnimation(false); break;
      case 's': w.stop(); break;
    }
  }
}
