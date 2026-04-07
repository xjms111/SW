#include <Arduino.h>
#include "Wheels.h"

#define SET_MOVEMENT(side,f,b) digitalWrite( side[0], f);\
                               digitalWrite( side[1], b)

#define TIME_PER_CM 100                          

extern void aktualizujLCD(int pozostalo, int mocL, int mocP);

Wheels::Wheels() 
{ }

void Wheels::attachRight(int pF, int pB, int pS)
{
    pinMode(pF, OUTPUT);
    pinMode(pB, OUTPUT);
    pinMode(pS, OUTPUT);
    this->pinsRight[0] = pF;
    this->pinsRight[1] = pB;
    this->pinsRight[2] = pS;
}

void Wheels::attachLeft(int pF, int pB, int pS)
{
    pinMode(pF, OUTPUT);
    pinMode(pB, OUTPUT);
    pinMode(pS, OUTPUT);
    this->pinsLeft[0] = pF;
    this->pinsLeft[1] = pB;
    this->pinsLeft[2] = pS;
}

void Wheels::setSpeedRight(uint8_t s)
{
    analogWrite(this->pinsRight[2], s);
}

void Wheels::setSpeedLeft(uint8_t s)
{
    analogWrite(this->pinsLeft[2], s);
}

void Wheels::setSpeed(uint8_t s)
{
    setSpeedLeft(s);
    setSpeedRight(s);
}

void Wheels::attach(int pRF, int pRB, int pRS, int pLF, int pLB, int pLS)
{
    this->attachRight(pRF, pRB, pRS);
    this->attachLeft(pLF, pLB, pLS);
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
    this->forwardLeft();
    this->forwardRight();
}

void Wheels::back()
{
    this->backLeft();
    this->backRight();
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
    this->stopLeft();
    this->stopRight();
}


void Wheels::goForward(int cm)
{
    unsigned long czasJazdy = cm * TIME_PER_CM;
    unsigned long startCzasu = millis();

    this->forward(); // Ruszamy silnikami
    
    // Zamiast delay() wchodzimy w pętlę, która stale aktualizuje ekran
    while (millis() - startCzasu < czasJazdy) {
        // Wyliczamy, ile cm już przejechaliśmy
        int przejechaneCM = (millis() - startCzasu) / TIME_PER_CM;
        int zostaloCM = cm - przejechaneCM;
        
        // Aktualizujemy LCD (255 na silnikach uruchomi animację do przodu ">")
        aktualizujLCD(zostaloCM, 255, 255);
    }
    
    this->stop();
    aktualizujLCD(0, 0, 0); // Po zatrzymaniu pokazujemy 0 i zatrzymujemy animację
}

void Wheels::goBack(int cm)
{
    unsigned long czasJazdy = cm * TIME_PER_CM;
    unsigned long startCzasu = millis();

    this->back(); 
    
    while (millis() - startCzasu < czasJazdy) {
        int przejechaneCM = (millis() - startCzasu) / TIME_PER_CM;
        int zostaloCM = cm - przejechaneCM;
        
        // Aktualizujemy LCD (-255 uruchomi animację do tyłu "<")
        aktualizujLCD(zostaloCM, -255, -255);
    }
    
    this->stop();
    aktualizujLCD(0, 0, 0);
}
