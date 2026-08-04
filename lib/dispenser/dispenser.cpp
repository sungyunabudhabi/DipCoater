#include <Arduino.h>
#include <ESP32Servo.h>
#include "dispenser.h"
#include "movements.h"
#include <FastAccelStepper.h>
#include <TMC2209.h>

#define SERVO_PIN 21

Servo servo;

const int angle[3] = {70, 110, 180};

void dispense (int numsub) {
    ESP32PWM::allocateTimer(0);
    servo.setPeriodHertz(50); // Standard 50Hz PWM
    servo.attach(SERVO_PIN, 500, 2400);
    servo.write(0);
    delay(1000);
    for (int i = 0; i < numsub; i++) {
        servo.write(angle[i]);
        delay(1000); // Wait for the servo to reach the position
        if (abortMotion) return;
        movedispense();
        if (abortMotion) return;
        delay(1000); // Wait for the dispenser to complete its movement
        if (abortMotion) return;
    }
    servo.write(0); // Move to the default position
    delay(1000); // Wait for the servo to reach the default position
    servo.detach(); // Detach the servo from the pin
}