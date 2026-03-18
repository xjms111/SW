#include "Wheels.h"

Wheels w;
volatile char cmd;

// zmienne enkoderów
volatile long encoderLeft = 0;
volatile long encoderRight = 0;

// ISR (przerwania)
void encoderLeftISR()
{
  encoderLeft++;
}

void encoderRightISR()
{
  encoderRight++;
}

void setup() {
  w.attach(7,8,5,12,11,10);

  Serial.begin(9600);

  // konfiguracja enkoderów
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(2), encoderLeftISR, RISING);
  attachInterrupt(digitalPinToInterrupt(3), encoderRightISR, RISING);

  Serial.println("Forward: WAD");
  Serial.println("Back: ZXC");
  Serial.println("Stop: S");
  Serial.println("Distance forward: F");
  Serial.println("Distance back: B");
}

void loop() {
  while(Serial.available())
  {
    cmd = Serial.read();
    switch(cmd)
    {
      case 'w': w.forward(); break;
      case 'x': w.back(); break;
      case 'a': w.forwardLeft(); break;
      case 'd': w.forwardRight(); break;
      case 'z': w.backLeft(); break;
      case 'c': w.backRight(); break;
      case 's': w.stop(); break;

      case '1': w.setSpeedLeft(75); break;
      case '2': w.setSpeedLeft(200); break;
      case '9': w.setSpeedRight(75); break;
      case '0': w.setSpeedRight(200); break;
      case '5': w.setSpeed(100); break;

      case 'f': w.goForward(50); break;
      case 'b': w.goBack(50); break;
    }
  }
}
