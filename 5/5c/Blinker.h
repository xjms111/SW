#ifndef BLINKER_H
#define BLINKER_H

#include <Arduino.h>

class Blinker {
public:
  static void begin(int pin);
  static void start(long int period);
  static void stop();
private:
  static int _pin;
  static void doBeep();
};

#endif
