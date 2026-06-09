#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#include "Wheels.h"
#include "SpeedSensor.h"

#define TRIG 9
#define ECHO 10
#define SERVO 3

LiquidCrystal_I2C lcd(0x27, 16, 2);
Wheels w;
Servo serwo;

// =====================================================
// PARAMETRY MODELU SPREZYNY
// =====================================================

const int TARGET_DISTANCE = 100;   // punkt rownowagi: 100 cm
const int DEAD_ZONE = 5;           // tolerancja +/- 5 cm

const int MIN_PWM = 90;            // minimalna moc, zeby silniki ruszyly
const int MAX_PWM = 210;           // maksymalna moc

const float KP = 2.0;              // "twardosc sprezyny"
const float KD = 5.0;              // tlumienie, ogranicza oscylacje

// =====================================================
// ZMIENNE POMIAROWE
// =====================================================

unsigned int currentDist = 400;
unsigned int rawDist = 400;

float filteredDist = 400.0;
float previousFilteredDist = 400.0;
float distanceChange = 0.0;

bool firstMeasurement = true;

// -1 = cofanie, 0 = stop, 1 = jazda do przodu
int driveDirection = 0;
int currentPWM = 0;

// =====================================================
// TICKER
// =====================================================

class Ticker {
  private:
    unsigned long period;
    unsigned long previousStart;
    void (*trigger)(void);

  public:
    Ticker(unsigned long p, void (*fun)(void)) {
      period = p;
      trigger = fun;
      previousStart = 0;
    }

    bool check() {
      unsigned long timeNow = millis();

      if (timeNow - previousStart > period) {
        (*trigger)();
        previousStart = timeNow;
        return true;
      }

      return false;
    }
};

// =====================================================
// DEKLARACJE FUNKCJI
// =====================================================

void updateLCD();
void springControl();
void applyMotorCommand(int command);
unsigned int getSonarDistance();

// =====================================================
// TICKERY
// =====================================================

Ticker lcdTicker(200, updateLCD);
Ticker controlTicker(100, springControl);

// =====================================================
// POMIAR ODLEGLOSCI
// =====================================================

unsigned int getSonarDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  unsigned long tot = pulseIn(ECHO, HIGH, 25000);

  if (tot == 0) {
    return 400;
  }

  return tot / 58;
}

// =====================================================
// STEROWANIE SILNIKAMI
// command > 0  -> jazda do przodu
// command < 0  -> cofanie
// command == 0 -> stop
// =====================================================

void applyMotorCommand(int command) {

  if (command == 0) {
    w.stop();

    driveDirection = 0;
    currentPWM = 0;

    return;
  }

  int pwm = abs(command);

  pwm = constrain(pwm, MIN_PWM, MAX_PWM);

  w.setSpeed(pwm);

  currentPWM = pwm;

  if (command > 0) {

    w.forward();

    driveDirection = 1;

  } else {

    // Nie uzywamy w.back(), bo tam wlacza sie Blinker na TimerOne.
    // Servo tez korzysta z timera, wiec bezpieczniej sterowac kolami bez buzzera.
    w.backLeft();
    w.backRight();

    driveDirection = -1;
  }
}

// =====================================================
// MODEL SPREZYNY HOOKE'A
// =====================================================

void springControl() {

  serwo.write(90);

  rawDist = getSonarDistance();

  if (firstMeasurement) {
    filteredDist = rawDist;
    previousFilteredDist = filteredDist;
    firstMeasurement = false;
  }

  previousFilteredDist = filteredDist;

  // Prosty filtr, zeby pojedyncze bledne pomiary nie szarpaly autem
  filteredDist = 0.7 * filteredDist + 0.3 * rawDist;

  currentDist = (unsigned int)filteredDist;

  distanceChange = filteredDist - previousFilteredDist;

  int error = currentDist - TARGET_DISTANCE;

  // Punkt minimum energii: autko jest okolo 100 cm od przeszkody
  if (abs(error) <= DEAD_ZONE && abs(distanceChange) < 1.5) {
    applyMotorCommand(0);
    return;
  }

  // Prawo Hooke'a + tlumienie:
  // error > 0: przeszkoda daleko, jedz do przodu
  // error < 0: przeszkoda blisko, cofaj
  // distanceChange > 0: przeszkoda sie oddala, gon ja
  // distanceChange < 0: przeszkoda sie zbliza, zwalniaj/cofaj
  float commandFloat = KP * error + KD * distanceChange;

  int command = (int)commandFloat;

  // Przy bardzo malych komendach nie szarpiemy silnikami
  if (abs(command) < MIN_PWM && abs(error) <= DEAD_ZONE) {
    command = 0;
  }

  command = constrain(command, -MAX_PWM, MAX_PWM);

  applyMotorCommand(command);

  Serial.print("raw=");
  Serial.print(rawDist);

  Serial.print(" filtered=");
  Serial.print(currentDist);

  Serial.print(" error=");
  Serial.print(error);

  Serial.print(" d=");
  Serial.print(distanceChange);

  Serial.print(" pwm=");
  Serial.print(currentPWM);

  Serial.print(" dir=");
  Serial.println(driveDirection);
}

// =====================================================
// LCD
// =====================================================

void updateLCD() {

  lcd.setCursor(0, 0);
  lcd.print("D:");
  lcd.print(currentDist);
  lcd.print("cm ");

  lcd.print("Cel:");
  lcd.print(TARGET_DISTANCE);
  lcd.print(" ");

  lcd.setCursor(0, 1);

  if (driveDirection > 0) {
    lcd.print("Jade  PWM:");
    lcd.print(currentPWM);
    lcd.print("   ");
  }
  else if (driveDirection < 0) {
    lcd.print("Cofam PWM:");
    lcd.print(currentPWM);
    lcd.print("   ");
  }
  else {
    lcd.print("STOP  MIN ENERG ");
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  Serial.begin(9600);

  w.attach(11, 12, 6, 8, 7, 5);

  SpeedSensor::begin();

  lcd.init();
  lcd.backlight();

  serwo.attach(SERVO);
  serwo.write(90);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Model sprezyny");
  lcd.setCursor(0, 1);
  lcd.print("Cel: 100 cm");

  delay(1500);

  lcd.clear();

  w.stop();

  Serial.println("Start modelu sprezyny Hooke'a");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  controlTicker.check();
  lcdTicker.check();
}
