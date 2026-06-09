#ifndef Wheels_h
#define Wheels_h

#include <Arduino.h>

class Wheels {
    public: 
        Wheels();
        int distanceTravelled;
        int distance;
        
        void attachRight(int pinForward, int pinBack, int pinSpeed);
        void attachLeft(int pinForward, int pinBack, int pinSpeed);
        void attach(int pinRightForward, int pinRightBack, int pinRightSpeed,
                    int pinLeftForward, int pinLeftBack, int pinLeftSpeed);
        
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
        int getSpeedLeft();
        int getSpeedRight();
        void setSpeedRight(uint8_t);
        void setSpeedLeft(uint8_t);
        
        void moveStep();
        void goForward(int cm);
        void goBack(int cm);
        bool getForw();
        bool getMoving();
        
        // Nowe metody potrzebne do obsługi skręcania na LCD
        bool getTurning();
        bool getLeft();
        
        void turnRight(int degrees);
        void turnLeft(int degrees);
        void turnStep();
        
        unsigned long dur = 0;

    private: 
        int pinsRight[3];
        int pinsLeft[3];
        bool startRequest;
        bool turnRequest;
        bool left;
        bool forw;
        bool moving;
        bool turning;
        int degrees = 0;
        int speedLeft = 0;
        int speedRight = 0;
        unsigned long startTime;
        long int getPeriod();
};

#endif
