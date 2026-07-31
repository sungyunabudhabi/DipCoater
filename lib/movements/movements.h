#ifndef MOVEMENTS_H
#define MOVEMENTS_H

#include <Arduino.h>
#include <FastAccelStepper.h>

// ==========================================
// Global Stepper Handles
// (Allows main.cpp to inspect positions/status if needed)
// ==========================================
extern FastAccelStepper *horizstepper;
extern FastAccelStepper *vertstepper;

extern volatile bool abortMotion;

// ==========================================
// Core Setup & Motor Functions
// ==========================================
void initMotors();
void enablemotor();
void disablemotor();

// ==========================================
// Motion Control Functions
// ==========================================
void movehoriz(float mm);
void movetohoriz(float pos);
void movevert(float mm);
void movetovert(float pos);
void homehoriz();
void homevert();
void dryvert();
void withdraw(float mmpermin);

// ==========================================
// Utility & Safety Functions
// ==========================================
void wait(float sec);
void estop();
void endstop();
void turnoff();

#endif // DIPCOATER_H