#include <Arduino.h>
#include <ESP32Servo.h>
#include "dispenser.h"
#include "movements.h"
#include <FastAccelStepper.h>
#include <TMC2209.h>

#define SERVO_PIN 21

// Declare the ESP32Servo object
Servo servo;

// Angles that the servo will move to, to align with the positions of all 3 motors
const int angle[3] = {55, 90, 135};

// Function that dispenses the solution from the syringe pump depending on the number of substrates being dipped
void dispense (int numsub) {
    ESP32PWM::allocateTimer(0);
    servo.setPeriodHertz(50); // Standard 50Hz PWM
    servo.attach(SERVO_PIN, 500, 2400);
    servo.write(0); // Make sure servo starts from the default position
    delay(1000);
    for (int i = 0; i < numsub; i++) {
        servo.write(angle[i]);
        delay(1000); // Wait for the servo to reach the position
        if (abortMotion) return;
        movedispense(1.5f);
        if (abortMotion) return;
        delay(1000); // Wait for the dispenser to complete its movement
        if (abortMotion) return;
    }
    servo.write(0); // Move to the default position
    delay(1000); // Wait for the servo to reach the default position
    servo.detach(); // Detach the servo from the pin
}

void refill() {
    Serial.println("refill() running");
    enablemotor();              // For some unknown reason, without this enablemotor(), refill() does not work.
                                // The software registers the steps to move the motor, meaning that the ESP32 sends 
                                // step pulses to the motor, but the motor just does not move.
    movedispense(-75.0f);       // Press Emergency Stop if it goes too far.
    disablemotor();
}

// This function was declared to call from other .cpp files.
void homeservo() {
    ESP32PWM::allocateTimer(0);
    servo.setPeriodHertz(50); // Standard 50Hz PWM
    servo.attach(SERVO_PIN, 500, 2400);
    servo.write(0);
    delay(1000);
    servo.detach();
}