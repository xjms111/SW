#ifndef Wheels_h
#define Wheels_h

#include <Arduino.h>

class Wheels {
    public: 
        Wheels();

        void attachRight(int pinForward, int pinBack, int pinSpeed);
        void attachLeft(int pinForward, int pinBack, int pinSpeed);
        void attach(int pRF, int pRB, int pRS, int pLF, int pLB, int pLS);

        void forward();
        void forwardLeft();
        void forwardRight();
        void back();
        void backLeft();
        void backRight();
        void stop();
        void stopLeft();
        void stopRight();

        void setSpeed(uint8_t);
        void setSpeedRight(uint8_t);
        void setSpeedLeft(uint8_t);

        void goForward(int cm);
        void goBack(int cm);

    private: 
        int pinsRight[3];
        int pinsLeft[3];

        uint8_t speedLeft = 100;
        uint8_t speedRight = 100;
};

#endif
