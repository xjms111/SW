#include <Arduino.h>
#include "Wheels.h"

#define SET_MOVEMENT(side,f,b) digitalWrite(side[0], f); \
                               digitalWrite(side[1], b)

#define TIME_PER_CM 100

extern void aktualizujLCD(int pozostalo, int mocL, int mocP);
extern void startBeep(long period);
extern void stopBeep();

Wheels::Wheels() {}

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
    speedRight = s;
    analogWrite(pinsRight[2], s);
}

void Wheels::setSpeedLeft(uint8_t s)
{
    speedLeft = s;
    analogWrite(pinsLeft[2], s);
}

void Wheels::setSpeed(uint8_t s)
{
    setSpeedLeft(s);
    setSpeedRight(s);
}

void Wheels::forwardLeft() { SET_MOVEMENT(pinsLeft, HIGH, LOW); }
void Wheels::forwardRight() { SET_MOVEMENT(pinsRight, HIGH, LOW); }
void Wheels::backLeft() { SET_MOVEMENT(pinsLeft, LOW, HIGH); }
void Wheels::backRight() { SET_MOVEMENT(pinsRight, LOW, HIGH); }

void Wheels::forward() { forwardLeft(); forwardRight(); }
void Wheels::back() { backLeft(); backRight(); }

void Wheels::stopLeft() { SET_MOVEMENT(pinsLeft, LOW, LOW); }
void Wheels::stopRight() { SET_MOVEMENT(pinsRight, LOW, LOW); }
void Wheels::stop() { stopLeft(); stopRight(); }

void Wheels::goForward(int cm)
{
    unsigned long czasJazdy = cm * TIME_PER_CM;
    unsigned long start = millis();

    forward();

    while (millis() - start < czasJazdy) {
        int przejechane = (millis() - start) / TIME_PER_CM;
        aktualizujLCD(cm - przejechane, speedLeft, speedRight);
    }

    stop();
    aktualizujLCD(0,0,0);
}

void Wheels::goBack(int cm)
{
    unsigned long czasJazdy = cm * TIME_PER_CM;
    unsigned long start = millis();

    // 🔥 częstotliwość zależna od prędkości
    long okres = map(speedLeft, 0, 255, 800000, 200000);
    startBeep(okres);

    back();

    while (millis() - start < czasJazdy) {
        int przejechane = (millis() - start) / TIME_PER_CM;
        aktualizujLCD(cm - przejechane, -speedLeft, -speedRight);
    }

    stop();
    stopBeep();
    aktualizujLCD(0,0,0);
}
