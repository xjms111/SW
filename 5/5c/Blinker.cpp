#include "Blinker.h"
#include <TimerOne.h>

int Blinker::_pin = 13;

void Blinker::begin(int pin) {
  _pin = pin;
  pinMode(_pin, OUTPUT);
  Timer1.initialize();
}

void Blinker::start(long int period) {
  Timer1.detachInterrupt();
  Timer1.attachInterrupt(doBeep, period);
}

void Blinker::doBeep() {
  digitalWrite(_pin, digitalRead(_pin) ^ 1);
}

void Blinker::stop() {
  Timer1.detachInterrupt();
  digitalWrite(_pin, LOW);
}
