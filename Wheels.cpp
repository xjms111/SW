#include <Arduino.h>
#include "Wheels.h"

#define SET_MOVEMENT(side,f,b) digitalWrite(side[0], f); \
                               digitalWrite(side[1], b)

// parametry robota (DOSTOSUJ!)
#define PULSES_PER_ROTATION 20
#define WHEEL_DIAMETER 6.5   // cm

// zmienne enkoderów (z .ino)
extern volatile long encoderLeft;
extern volatile long encoderRight;

Wheels::Wheels() 
{ }

void Wheels::attachRight(int pF, int pB, int pS)
{
    pinMode(pF, OUTPUT);
    pinMode(pB, OUTPUT);
    pinMode(pS, OUTPUT);

    pinsRight[0] = pF;
    pinsRight[1] = pB;
    pinsRight[2] = pS;
}

void Wheels::attachLeft(int pF, int pB, int pS)
{
    pinMode(pF, OUTPUT);
    pinMode(pB, OUTPUT);
    pinMode(pS, OUTPUT);

    pinsLeft[0] = pF;
    pinsLeft[1] = pB;
    pinsLeft[2] = pS;
}

void Wheels::attach(int pRF, int pRB, int pRS, int pLF, int pLB, int pLS)
{
    attachRight(pRF, pRB, pRS);
    attachLeft(pLF, pLB, pLS);
}

void Wheels::setSpeedRight(uint8_t s)
{
    analogWrite(pinsRight[2], s);
}

void Wheels::setSpeedLeft(uint8_t s)
{
    analogWrite(pinsLeft[2], s);
}

void Wheels::setSpeed(uint8_t s)
{
    setSpeedLeft(s);
    setSpeedRight(s);
}

void Wheels::forwardLeft() 
{
    SET_MOVEMENT(pinsLeft, HIGH, LOW);
}

void Wheels::forwardRight() 
{
    SET_MOVEMENT(pinsRight, HIGH, LOW);
}

void Wheels::backLeft()
{
    SET_MOVEMENT(pinsLeft, LOW, HIGH);
}

void Wheels::backRight()
{
    SET_MOVEMENT(pinsRight, LOW, HIGH);
}

void Wheels::forward()
{
    forwardLeft();
    forwardRight();
}

void Wheels::back()
{
    backLeft();
    backRight();
}

void Wheels::stopLeft()
{
    SET_MOVEMENT(pinsLeft, LOW, LOW);
}

void Wheels::stopRight()
{
    SET_MOVEMENT(pinsRight, LOW, LOW);
}

void Wheels::stop()
{
    stopLeft();
    stopRight();
}

/*
 * NOWE METODY – na podstawie enkoderów
 */

void Wheels::goForward(int cm)
{
    float wheelCirc = 3.1416 * WHEEL_DIAMETER;
    float rotations = cm / wheelCirc;
    long targetPulses = rotations * PULSES_PER_ROTATION;

    encoderLeft = 0;
    encoderRight = 0;

    forward();

    while(encoderLeft < targetPulses && encoderRight < targetPulses)
    {
        // czekamy na impulsy
    }

    stop();
}

void Wheels::goBack(int cm)
{
    float wheelCirc = 3.1416 * WHEEL_DIAMETER;
    float rotations = cm / wheelCirc;
    long targetPulses = rotations * PULSES_PER_ROTATION;

    encoderLeft = 0;
    encoderRight = 0;

    back();

    while(encoderLeft < targetPulses && encoderRight < targetPulses)
    {
        // czekamy
    }

    stop();
}
