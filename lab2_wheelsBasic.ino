#include "Wheels.h"
#include <LiquidCrystal_I2C.h>
#include "Blinker.h"
#include "SpeedSensor.h"

byte LCDAddress = 0x27;

LiquidCrystal_I2C lcd(LCDAddress, 16, 2);

class Ticker {
  private: 
    unsigned long period;
    unsigned long previousStart;
    void (*trigger)(void);
  public: 
    Ticker(unsigned long p, void(*fun)(void)) {
      period = p;
      trigger = fun;
      previousStart = 0;
    }
    bool check() {
      unsigned long timeNow = millis();
      if(timeNow - previousStart > period) {
        // call function
        (*trigger)();
        previousStart = timeNow;
        return true;
      }
      return false;
    }
};

Wheels w;

volatile char cmd;
unsigned long lastMove = 0;
const unsigned long interval = 5000;
const unsigned long inter = 5000;
int lap = 0;
unsigned long lm = 0;
unsigned int fr = 0;
int lastSpeedLeft=0;
int lastSpeedRight=0;
int lastDist=0;

void an() {
  int tmp = w.distance-w.distanceTravelled;
    //SpeedSensor::printCnt();
    if(lastDist!=tmp){
      lcd.setCursor(5,0);
      lcd.print("   ");
      lcd.setCursor(0,0);
      lcd.print("dist:");
      lcd.print(tmp);
      lastDist=tmp;
      
    }
}

Ticker ticker(32, an);

void speedChange() {
  int sl = w.getSpeedLeft();
  int sr = w.getSpeedRight();
  if (!w.getForw()){
    sl *= -1;
    sr *= -1;
  }
  if(lastSpeedRight!= sr || lastSpeedLeft != sl){
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print(sl);
    lcd.setCursor(12, 1);
    lcd.print("    ");
    lcd.setCursor(12, 1);
    lcd.print(sr);
    lastSpeedLeft = sl;
    lastSpeedRight = sr;
  }
}

Ticker speedTicker(32, speedChange);

uint8_t arrowUp[8] =
{
    0b00100,
    0b01110,
    0b11111,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100
};

uint8_t arrowDown[8] =
{
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b11111,
    0b01110,
    0b00100
};

uint8_t frame[8];
uint8_t current[8];

void startAnimation() {
  if (w.getForw()) {
    lcd.createChar(0, arrowUp);
    memcpy(current, arrowUp, 8);
  } else {
    lcd.createChar(0, arrowDown);
    memcpy(current, arrowDown, 8);
  }
}

void animation(){
  if(w.getMoving()){
    lcd.createChar(0, current);
    lcd.setCursor(8, 1);
    lcd.write(byte(0));
    if (w.getForw()){
      shiftUp(current, frame);
      memcpy(current, frame, 8);
    } else {
      shiftDown(current, frame);
      memcpy(current, frame, 8);
    }
  } else {
    lcd.setCursor(8, 1);
    lcd.print(" ");
  }
}

Ticker animationTicker(200, animation);

void shiftUp(uint8_t *src, uint8_t *dest) {
  for (int i = 0; i < 7; i++) {
    dest[i] = src[i + 1];
  }
  dest[7] = src[0]; // zawinięcie (rotacja)
}

void shiftDown(uint8_t *src, uint8_t *dest){
  for (int i = 7; i>0; i--){
    dest[i] = src[i-1];
  }
  dest[0] = src[7];
}

void moveForward(int cm){
  w.goForward(cm);
  startAnimation();
}

void moveBack(int cm){
  w.goBack(cm);
  startAnimation();
}

void setup() {
  // put your setup code here, to run once:
  w.attach(7,8,5,12,11,6);
  
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  Blinker::begin(13);
  SpeedSensor::begin();
}

void loop() {
  ticker.check();
  w.moveStep();
  w.turnStep();
  speedTicker.check();
  animationTicker.check();
  if(millis()-lastMove>interval){
    Serial.println("obrot");
    switch(fr){
      case 0:
        Serial.println("lewo");
        w.turnLeft(90);
        break;
      case 2:
        w.turnRight(90);
        break;
      case 1:
        w.setSpeed(130);
        w.goBack(50);
        break;
      case 3:
        w.setSpeed(250);
        w.goBack(50);
        break;
      default:
        break;
    }
    fr = (fr+1)%4;
    lastMove = millis();
  }
  /*if(millis()-lm>inter){
    if(w.dur>0){
      float speed = 50/(w.dur/1000.0);
      Serial.println(speed);
    }
    if(lap<15){
      w.setSpeed(120+lap*10);
      w.goForward(50);
    }
    lap++;
    lm = millis();
  }*/
}
