#define DECODE_NEC

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <IRremote.hpp>

#include "Wheels.h"
#include "SpeedSensor.h"

#define TRIG 9
#define ECHO 10
#define SERVO 3

#define IR_RECEIVE_PIN 2

LiquidCrystal_I2C lcd(0x27, 16, 2);
Wheels w;
Servo serwo;


const uint8_t IR_UP       = 0x18;
const uint8_t IR_DOWN     = 0x52;
const uint8_t IR_LEFT     = 0x08;
const uint8_t IR_RIGHT    = 0x5A;
const uint8_t IR_OK       = 0x1C;

const uint8_t IR_STAR     = 0x16;
const uint8_t IR_HASH     = 0x0D;

const uint8_t IR_0        = 0x19;
const uint8_t IR_1        = 0x45;
const uint8_t IR_2        = 0x46;
const uint8_t IR_3        = 0x47;
const uint8_t IR_4        = 0x44;
const uint8_t IR_5        = 0x40;
const uint8_t IR_6        = 0x43;
const uint8_t IR_7        = 0x07;
const uint8_t IR_8        = 0x15;
const uint8_t IR_9        = 0x09;

// Dodatkowe nazwy, jesli pilot ma podpisy CH/VOL zamiast cyfr/strzalek.
const uint8_t IR_CH_MINUS = 0x45;
const uint8_t IR_CH       = 0x46;
const uint8_t IR_CH_PLUS  = 0x47;
const uint8_t IR_PREV     = 0x44;
const uint8_t IR_NEXT     = 0x40;
const uint8_t IR_PLAY     = 0x43;
const uint8_t IR_VOL_MINUS = 0x07;
const uint8_t IR_VOL_PLUS  = 0x15;
const uint8_t IR_EQ        = 0x09;

// PIN STARTOWY

const char CORRECT_PIN[] = "1234";
char enteredPin[8];
byte enteredPinLength = 0;

// TRYBY PRACY

enum RobotMode {
  MODE_MANUAL,
  MODE_SPRING
};

RobotMode robotMode = MODE_MANUAL;

// PARAMETRY MODELU SPREZYNY

int targetDistance = 100;          // punkt rownowagi: 100 cm
const int DEAD_ZONE = 5;           // tolerancja +/- 5 cm

const int MIN_PWM = 90;            // minimalna moc, zeby silniki ruszyly
const int MAX_PWM = 220;           // maksymalna moc

const float KP = 2.0;              // "twardosc sprezyny"
const float KD = 5.0;              // tlumienie


// ZMIENNE POMIAROWE


unsigned int currentDist = 400;
unsigned int rawDist = 400;

float filteredDist = 400.0;
float previousFilteredDist = 400.0;
float distanceChange = 0.0;

bool firstMeasurement = true;

// -2 = obrot lewo, -1 = cofanie, 0 = stop, 1 = przod, 2 = obrot prawo
int driveDirection = 0;

int currentPWM = 0;
int manualSpeed = 150;

// IR — CALLBACK

volatile bool irDataReady = false;
volatile uint8_t lastIRCommand = 0;
volatile uint8_t lastIRFlags = 0;
volatile uint32_t lastIRRawData = 0;

void ReceiveCompleteCallbackHandler();


// TICKER


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

// DEKLARACJE FUNKCJI

void updateLCD();
void springControl();
void applyMotorCommand(int command);
unsigned int getSonarDistance();

void stopRobot();
void manualForward();
void manualBack();
void manualLeft();
void manualRight();

void handleRemoteCommand(uint8_t command);
bool getIRCommand(uint8_t &command, uint8_t &flags, uint32_t &rawData);
void waitForPIN();
void handlePINCommand(uint8_t command);
char commandToDigit(uint8_t command);
void printIRCode(uint8_t command, uint8_t flags, uint32_t rawData);


// TICKERY


Ticker lcdTicker(200, updateLCD);
Ticker controlTicker(100, springControl);


// POMIAR ODLEGLOSCI

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

// STEROWANIE SILNIKAMI

void stopRobot() {
  w.stop();
  driveDirection = 0;
  currentPWM = 0;
}

void manualForward() {
  robotMode = MODE_MANUAL;

  w.setSpeed(manualSpeed);
  w.forward();

  driveDirection = 1;
  currentPWM = manualSpeed;
}

void manualBack() {
  robotMode = MODE_MANUAL;

  w.setSpeed(manualSpeed);

  w.backLeft();
  w.backRight();

  driveDirection = -1;
  currentPWM = manualSpeed;
}

void manualLeft() {
  robotMode = MODE_MANUAL;

  w.setSpeed(manualSpeed);

  w.backLeft();
  w.forwardRight();

  driveDirection = -2;
  currentPWM = manualSpeed;
}

void manualRight() {
  robotMode = MODE_MANUAL;

  w.setSpeed(manualSpeed);

  w.forwardLeft();
  w.backRight();

  driveDirection = 2;
  currentPWM = manualSpeed;
}

// command > 0  -> jazda do przodu
// command < 0  -> cofanie
// command == 0 -> stop
void applyMotorCommand(int command) {

  if (command == 0) {
    stopRobot();
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
    w.backLeft();
    w.backRight();
    driveDirection = -1;
  }
}

// =====================================================
// MODEL SPREZYNY HOOKE'A
// =====================================================

void springControl() {

  if (robotMode != MODE_SPRING) {
    return;
  }

  serwo.write(90);

  rawDist = getSonarDistance();

  if (firstMeasurement) {
    filteredDist = rawDist;
    previousFilteredDist = filteredDist;
    firstMeasurement = false;
  }

  previousFilteredDist = filteredDist;

  // filtr wygladzajacy pomiary
  filteredDist = 0.7 * filteredDist + 0.3 * rawDist;

  currentDist = (unsigned int)filteredDist;

  distanceChange = filteredDist - previousFilteredDist;

  int error = currentDist - targetDistance;

  // minimum energii: autko okolo targetDistance od przeszkody
  if (abs(error) <= DEAD_ZONE && abs(distanceChange) < 1.5) {
    applyMotorCommand(0);
    return;
  }

  // Prawo Hooke'a + tlumienie.
  // error > 0: przeszkoda za daleko, jedz do przodu.
  // error < 0: przeszkoda za blisko, cofaj.
  // distanceChange > 0: przeszkoda sie oddala, gon ja.
  // distanceChange < 0: przeszkoda sie zbliza, zwalniaj/cofaj.
  float commandFloat = KP * error + KD * distanceChange;

  int command = (int)commandFloat;

  if (abs(command) < MIN_PWM && abs(error) <= DEAD_ZONE) {
    command = 0;
  }

  command = constrain(command, -MAX_PWM, MAX_PWM);

  applyMotorCommand(command);

  Serial.print("SPRING raw=");
  Serial.print(rawDist);

  Serial.print(" dist=");
  Serial.print(currentDist);

  Serial.print(" target=");
  Serial.print(targetDistance);

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

  if (robotMode == MODE_SPRING) {
    lcd.print("AUTO D:");
    lcd.print(currentDist);
    lcd.print(" T:");
    lcd.print(targetDistance);
    lcd.print(" ");
  } else {
    lcd.print("MANUAL SPD:");
    lcd.print(manualSpeed);
    lcd.print("    ");
  }

  lcd.setCursor(0, 1);

  if (driveDirection == 1) {
    lcd.print("PRZOD PWM:");
    lcd.print(currentPWM);
    lcd.print("   ");
  }
  else if (driveDirection == -1) {
    lcd.print("TYL   PWM:");
    lcd.print(currentPWM);
    lcd.print("   ");
  }
  else if (driveDirection == -2) {
    lcd.print("OBROT LEWO     ");
  }
  else if (driveDirection == 2) {
    lcd.print("OBROT PRAWO    ");
  }
  else {
    lcd.print("STOP            ");
  }
}

// =====================================================
// OBSLUGA PILOTA
// =====================================================

void handleRemoteCommand(uint8_t command) {

  // Każdy klawisz działa. Jeśli Twój pilot ma inne kody,
  // popraw stale IR_* na początku programu.

  switch (command) {

    case IR_UP:
      manualForward();
      break;

    case IR_DOWN:
      manualBack();
      break;

    case IR_LEFT:
      manualLeft();
      break;

    case IR_RIGHT:
      manualRight();
      break;

    case IR_OK:
      robotMode = MODE_MANUAL;
      stopRobot();
      break;

    case IR_STAR:
      // Tryb automatyczny — model sprezyny.
      robotMode = MODE_SPRING;
      firstMeasurement = true;
      serwo.write(90);
      lcd.clear();
      break;

    case IR_HASH:
      // Tryb reczny.
      robotMode = MODE_MANUAL;
      stopRobot();
      lcd.clear();
      break;

    case IR_0:
      stopRobot();
      break;

    case IR_1:
      manualSpeed = 90;
      break;

    case IR_2:
      manualSpeed = 110;
      break;

    case IR_3:
      manualSpeed = 130;
      break;

    case IR_4:
      manualSpeed = 150;
      break;

    case IR_5:
      manualSpeed = 170;
      break;

    case IR_6:
      manualSpeed = 190;
      break;

    case IR_7:
      manualSpeed = 210;
      break;

    case IR_8:
      manualSpeed = 230;
      break;

    case IR_9:
      manualSpeed = 250;
      break;

    case IR_CH_MINUS:
      targetDistance -= 5;
      if (targetDistance < 30) targetDistance = 30;
      break;

    case IR_CH:
      targetDistance = 100;
      break;

    case IR_CH_PLUS:
      targetDistance += 5;
      if (targetDistance > 250) targetDistance = 250;
      break;

    case IR_VOL_MINUS:
      manualSpeed -= 10;
      if (manualSpeed < MIN_PWM) manualSpeed = MIN_PWM;
      break;

    case IR_VOL_PLUS:
      manualSpeed += 10;
      if (manualSpeed > 250) manualSpeed = 250;
      break;

    case IR_PLAY:
      if (robotMode == MODE_SPRING) {
        robotMode = MODE_MANUAL;
        stopRobot();
      } else {
        robotMode = MODE_SPRING;
        firstMeasurement = true;
        serwo.write(90);
      }
      lcd.clear();
      break;

    case IR_PREV:
      manualLeft();
      break;

    case IR_NEXT:
      manualRight();
      break;

    case IR_EQ:
      stopRobot();
      break;

    default:
      // Nieznany klawisz — nic nie robimy poza wydrukiem kodu.
      break;
  }
}

bool getIRCommand(uint8_t &command, uint8_t &flags, uint32_t &rawData) {

  if (!irDataReady) {
    return false;
  }

  noInterrupts();

  command = lastIRCommand;
  flags = lastIRFlags;
  rawData = lastIRRawData;
  irDataReady = false;

  interrupts();

  return true;
}

void printIRCode(uint8_t command, uint8_t flags, uint32_t rawData) {

  Serial.print("IR command: 0x");
  Serial.print(command, HEX);

  Serial.print(" raw: 0x");
  Serial.print(rawData, HEX);

  Serial.print(" flags: 0x");
  Serial.println(flags, HEX);
}

// =====================================================
// PIN STARTOWY
// =====================================================

char commandToDigit(uint8_t command) {

  if (command == IR_0) return '0';
  if (command == IR_1) return '1';
  if (command == IR_2) return '2';
  if (command == IR_3) return '3';
  if (command == IR_4) return '4';
  if (command == IR_5) return '5';
  if (command == IR_6) return '6';
  if (command == IR_7) return '7';
  if (command == IR_8) return '8';
  if (command == IR_9) return '9';

  return '\0';
}

void handlePINCommand(uint8_t command) {

  char digit = commandToDigit(command);

  if (digit != '\0') {

    if (enteredPinLength < sizeof(enteredPin) - 1) {
      enteredPin[enteredPinLength] = digit;
      enteredPinLength++;
      enteredPin[enteredPinLength] = '\0';
    }

    lcd.setCursor(0, 1);
    lcd.print("PIN: ");

    for (byte i = 0; i < enteredPinLength; i++) {
      lcd.print("*");
    }

    lcd.print("        ");
  }

  else if (command == IR_STAR) {
    // Kasowanie wpisanego PIN-u.
    enteredPinLength = 0;
    enteredPin[0] = '\0';

    lcd.setCursor(0, 1);
    lcd.print("PIN wyczyszcz. ");
  }

  else if (command == IR_OK) {
    // ENTER / OK.
    if (strcmp(enteredPin, CORRECT_PIN) == 0) {

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PIN OK");
      lcd.setCursor(0, 1);
      lcd.print("Odblokowano");

      delay(1000);

    } else {

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ZLY PIN");
      lcd.setCursor(0, 1);
      lcd.print("Sprobuj znowu");

      enteredPinLength = 0;
      enteredPin[0] = '\0';

      delay(1000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Podaj PIN:");
    }
  }
}

void waitForPIN() {

  enteredPinLength = 0;
  enteredPin[0] = '\0';

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Podaj PIN:");
  lcd.setCursor(0, 1);
  lcd.print("OK=ENTER *=CLR");

  Serial.println("Czekam na PIN. Kody IR beda wypisywane ponizej.");

  while (strcmp(enteredPin, CORRECT_PIN) != 0) {

    uint8_t command;
    uint8_t flags;
    uint32_t rawData;

    if (getIRCommand(command, flags, rawData)) {

      printIRCode(command, flags, rawData);

      // Ignorujemy powtorzenia podczas wpisywania PIN-u,
      // zeby przytrzymany klawisz nie wpisal kilku cyfr.
      if (flags != 0) {
        continue;
      }

      handlePINCommand(command);
    }
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

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  IrReceiver.registerReceiveCompleteCallback(ReceiveCompleteCallbackHandler);

  stopRobot();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IR + robot");
  lcd.setCursor(0, 1);
  lcd.print("Pin IR: 2");

  delay(1000);

  waitForPIN();

  robotMode = MODE_MANUAL;
  stopRobot();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tryb reczny");
  lcd.setCursor(0, 1);
  lcd.print("*=AUTO #=MAN");

  Serial.println("Robot odblokowany.");
  Serial.println("Sterowanie:");
  Serial.println("UP/DOWN/LEFT/RIGHT - ruch");
  Serial.println("OK/0 - stop");
  Serial.println("* - tryb sprezyny AUTO");
  Serial.println("# - tryb reczny");
  Serial.println("1..9 - predkosc");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  uint8_t command;
  uint8_t flags;
  uint32_t rawData;

  if (getIRCommand(command, flags, rawData)) {

    printIRCode(command, flags, rawData);

    // Powtorzenia ignorujemy. Robot i tak kontynuuje ostatnia komende.
    if (flags == 0) {
      handleRemoteCommand(command);
    }
  }

  controlTicker.check();
  lcdTicker.check();
}

// =====================================================
// IR CALLBACK
// =====================================================

#if defined(ESP32) || defined(ESP8266)
IRAM_ATTR
#endif
void ReceiveCompleteCallbackHandler() {

  IrReceiver.decode();

  lastIRCommand = IrReceiver.decodedIRData.command;
  lastIRFlags = IrReceiver.decodedIRData.flags;
  lastIRRawData = IrReceiver.decodedIRData.decodedRawData;

  irDataReady = true;

  IrReceiver.resume();
}
