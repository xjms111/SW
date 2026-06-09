#include "Arduino.h"
#include "SpeedSensor.h"

#define INTINPUT0 A0
#define INTINPUT1 A1

static volatile int cnt0, cnt1;

void SpeedSensor::begin(){
  cnt0 = 0;
  cnt1 = 0;
  pinMode(INTINPUT0, INPUT);
  pinMode(INTINPUT1, INPUT);
  attachPCINT(digitalPinToPCINT(INTINPUT0), increment, CHANGE);
  attachPCINT(digitalPinToPCINT(INTINPUT1), increment, CHANGE);
  
}

void SpeedSensor::increment(){
  if( (PINC & (1 << PC0)) ) 
  cnt0++;

  if( (PINC & (1 << PC1)) )
  cnt1++;
}

void SpeedSensor::printCnt(){
  Serial.print(cnt0);
  Serial.print(" ");
  Serial.println(cnt1);
}

int SpeedSensor::getCnts(){
  return cnt1 + cnt0;
}

void SpeedSensor::reset(){
  cnt0=0;
  cnt1=0;
}
