#include <Arduino.h>
#include "Wheels.h"

#define SET_MOVEMENT(side,f,b) digitalWrite(side[0], f); \
                               digitalWrite(side[1], b)

// Liczba impulsów enkodera przypadająca na 1 cm.
// Ustal eksperymentalnie.
#define PULSES_PER_CM 5.0

// Funkcje i zmienne z pliku .ino
extern void aktualizujLCD(int pozostalo, int mocL, int mocP);
extern void startBeep(long period);
extern void stopBeep();

extern volatile int cnt0;   // prawy enkoder
extern volatile int cnt1;   // lewy enkoder

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

void Wheels::attach(int pRF, int pRB, int pRS,
                    int pLF, int pLB, int pLS)
{
    attachRight(pRF, pRB, pRS);
    attachLeft(pLF, pLB, pLS);
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
    // "bip-bip" podczas cofania
    long period = map(100, 0, 255, 800000, 200000);
    startBeep(period);

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
    stopBeep();
}

void Wheels::goForward(int cm)
{
    long target = cm * PULSES_PER_CM;

    // Zerowanie liczników
    cnt0 = 0;
    cnt1 = 0;

    forward();

    while (((cnt0 + cnt1) / 2.0) < target) {
        float pulsesDone = (cnt0 + cnt1) / 2.0;
        int pozostalo = (target - pulsesDone) / PULSES_PER_CM;
        if (pozostalo < 0) pozostalo = 0;

        aktualizujLCD(pozostalo, 255, 255);
    }

    stop();
    aktualizujLCD(0, 0, 0);
}

void Wheels::goBack(int cm)
{
    long target = cm * PULSES_PER_CM;

    // Zerowanie liczników
    cnt0 = 0;
    cnt1 = 0;

    back();

    while (((cnt0 + cnt1) / 2.0) < target) {
        float pulsesDone = (cnt0 + cnt1) / 2.0;
        int pozostalo = (target - pulsesDone) / PULSES_PER_CM;
        if (pozostalo < 0) pozostalo = 0;

        aktualizujLCD(pozostalo, -255, -255);
    }

    stop();
    aktualizujLCD(0, 0, 0);
}
