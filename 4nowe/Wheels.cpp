#include <Arduino.h>
#include "HardwareSerial.h"
#include "Wheels.h"
#include "Blinker.h"
#include "SpeedSensor.h"

#define SET_MOVEMENT(side,f,b) digitalWrite( side[0], f);\
                               digitalWrite( side[1], b)

Wheels::Wheels() { distanceTravelled = 0; }
bool Wheels::getForw() { return this->forw; }

void Wheels::attachRight(int pF, int pB, int pS) {
    pinMode(pF, OUTPUT); pinMode(pB, OUTPUT); pinMode(pS, OUTPUT);
    this->pinsRight[0] = pF; this->pinsRight[1] = pB; this->pinsRight[2] = pS;
}

void Wheels::attachLeft(int pF, int pB, int pS) {
    pinMode(pF, OUTPUT); pinMode(pB, OUTPUT); pinMode(pS, OUTPUT);
    this->pinsLeft[0] = pF; this->pinsLeft[1] = pB; this->pinsLeft[2] = pS;
}

void Wheels::setSpeedRight(uint8_t s) { analogWrite(this->pinsRight[2], s); speedRight = s; }
int Wheels::getSpeedRight() { return this->speedRight; }
void Wheels::setSpeedLeft(uint8_t s) { analogWrite(this->pinsLeft[2], s); speedLeft = s; }
int Wheels::getSpeedLeft() { return this->speedLeft; }
void Wheels::setSpeed(uint8_t s) { setSpeedLeft(s); setSpeedRight(s); }

void Wheels::attach(int pRF, int pRB, int pRS, int pLF, int pLB, int pLS) {
    this->attachRight(pRF, pRB, pRS); this->attachLeft(pLF, pLB, pLS);
}

void Wheels::forwardLeft() { SET_MOVEMENT(pinsLeft, HIGH, LOW); }
void Wheels::forwardRight() { SET_MOVEMENT(pinsRight, HIGH, LOW); }
void Wheels::backLeft() { SET_MOVEMENT(pinsLeft, LOW, HIGH); }
void Wheels::backRight() { SET_MOVEMENT(pinsRight, LOW, HIGH); }
void Wheels::forward() { this->forwardLeft(); this->forwardRight(); }

void Wheels::back() {
    this->backLeft(); this->backRight();
    long int tmp = this->getPeriod();
    Blinker::start(tmp);
}

void Wheels::stopLeft() { SET_MOVEMENT(pinsLeft, LOW, LOW); }
void Wheels::stopRight() { SET_MOVEMENT(pinsRight, LOW, LOW); }
void Wheels::stop() { this->stopLeft(); this->stopRight(); Blinker::stop(); }

void Wheels::moveStep() {
    if(!moving) return;
    if(startRequest){
        if(forw) this->forward(); else this->back();
        startRequest = false;
        startTime = millis();
    }
    if(SpeedSensor::getCnts() >= distance * 4) {
        moving = false; distanceTravelled = 0; distance = 0;
        this->stop(); SpeedSensor::reset();
        dur = (millis() - startTime);
    } else {  
        distanceTravelled = SpeedSensor::getCnts() / 4; 
    }
}

void Wheels::turnStep(){
    if(!turning) return;
    if(turnRequest){
        this->setSpeed(180);
        if(left) { forwardRight(); backLeft(); } else { forwardLeft(); backRight(); }
        turnRequest = false;
    }
    if(SpeedSensor::getCnts() >= degrees * 0.77){
        turning = false;
        this->stop();
        SpeedSensor::reset();
    }
}

void Wheels::goForward(int cm) { moving = true; startRequest = true; forw = true; distance = cm; }
void Wheels::goBack(int cm) { moving = true; startRequest = true; forw = false; distance = cm; }
bool Wheels::getMoving() { return this->moving; }


bool Wheels::getTurning() { return this->turning; }
bool Wheels::getLeft() { return this->left; }

long int Wheels::getPeriod() {
    long int f_min = 300000, f_max = 1000000;
    return f_max - (f_max - f_min) * (this->speedLeft / 255.0);
}

void Wheels::turnLeft(int deg) { turning = true; turnRequest = true; left = true; degrees = deg; }
void Wheels::turnRight(int deg) { turning = true; turnRequest = true; left = false; degrees = deg; }
