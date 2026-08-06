#ifndef DISPENSER_H
#define DISPENSER_H

#include <Arduino.h>
#include <ESP32Servo.h>

extern Servo servo;

void dispense (int numsub);
void homeservo();
void refill();

#endif