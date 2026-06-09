#ifndef SpeedSensor_H
#define SpeedSensor_H

#include "Arduino.h"
#include "PinChangeInterrupt.h"

class SpeedSensor {
  public:
  static void begin();
  static void printCnt();
  static int getCnts();
  static void reset();
  private:
  static void increment();
};

#endif
