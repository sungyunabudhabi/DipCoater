#include <Arduino.h>
#include <TMC2209.h>
#include <FastAccelStepper.h>
#include <esp_task_wdt.h>
#include "movements.h"

volatile bool abortMotion = false;

const uint8_t HORIZ_STEP_PIN = 14;
const uint8_t HORIZ_DIR_PIN = 27;
const uint8_t VERT_STEP_PIN = 26;
const uint8_t VERT_DIR_PIN = 25;
const uint8_t DISPENSE_STEP_PIN = 13;
const uint8_t DISPENSE_DIR_PIN = 12;
const uint8_t MICROSTEPS = 16;
const uint8_t HORIZ_DIAG_PIN = 33;
const uint8_t VERT_DIAG_PIN = 32;
//const uint8_t HORIZ_ENDSTOP_PIN = 33;
//const uint8_t VERT_ENDSTOP_PIN = 32;

TMC2209 horizmotor;
TMC2209 vertmotor;
TMC2209 dispensemotor;
FastAccelStepper *horizstepper = NULL;
FastAccelStepper *vertstepper = NULL;
FastAccelStepper *dispensestepper = NULL;
FastAccelStepperEngine engine = FastAccelStepperEngine();

int DEFAULT_SPEED = 32000;
int DEFAULT_ACCELERATION = 64000;

//steps = (200/2) * MICROSTEPS * mm;
//stepspersec = (200/2) * MICROSTEPS * mmpersec;
//stepspersec = (200/2) * MICROSTEPS * (mmpermin/60);

// Initialize the motors through TMC2209 and FastAccelStepper
void initMotors() {
    //pinMode(HORIZ_ENDSTOP_PIN, INPUT_PULLUP);
    //pinMode(VERT_ENDSTOP_PIN, INPUT_PULLUP);
    pinMode(HORIZ_DIAG_PIN, INPUT_PULLDOWN);
    pinMode(VERT_DIAG_PIN, INPUT_PULLDOWN);

    // Establish serial communication
    Serial2.begin(115200, SERIAL_8N1, 16, 17);
    delay(100);

    horizmotor.setup(Serial2, 115200, TMC2209::SERIAL_ADDRESS_1, 16, 17);       // assign the address
    vertmotor.setup(Serial2, 115200, TMC2209::SERIAL_ADDRESS_0, 16, 17);
    dispensemotor.setup(Serial2, 115200, TMC2209::SERIAL_ADDRESS_2, 16, 17);

    delay(100);

    if (horizmotor.isSetupAndCommunicating()) {
    Serial.println("Horiz motor setup and communication successful.");
    } else {
    Serial.println("Error: Failed to setup or communicate with horiz motor.");
    while(1); // Halt execution to prevent crashing
    }

    if (vertmotor.isSetupAndCommunicating()) {
    Serial.println("Vert motor setup and communication successful.");
    } else {
    Serial.println("Error: Failed to setup or communicate with vert motor.");
    while(1); // Halt execution to prevent crashing
    }


    // TMC2209 Library Initialization
    horizmotor.setMicrostepsPerStep(MICROSTEPS);                        // set microsteps per step
    horizmotor.setRunCurrent(80);                                       // set current cap at 80%
    horizmotor.disableStealthChop();                                    // enabling stealthchop causes motor to skip steps at higher speeds
    horizmotor.setStallGuardThreshold(60);                              // set stallguard threshold (sensitivity for detecting stalls)
    horizmotor.setCoolStepDurationThreshold(0xFFFFF);
    horizmotor.enableInverseMotorDirection();
    
    vertmotor.setMicrostepsPerStep(MICROSTEPS);
    vertmotor.setRunCurrent(80);
    vertmotor.disableStealthChop();
    vertmotor.setStallGuardThreshold(60);
    vertmotor.setCoolStepDurationThreshold(0xFFFFF);
    vertmotor.enableInverseMotorDirection();

    dispensemotor.setMicrostepsPerStep(MICROSTEPS);
    dispensemotor.setRunCurrent(80);

    // FastAccelStepper Initialization
    engine.init();

    horizstepper = engine.stepperConnectToPin(HORIZ_STEP_PIN);
    vertstepper = engine.stepperConnectToPin(VERT_STEP_PIN);
    dispensestepper = engine.stepperConnectToPin(DISPENSE_STEP_PIN);

    if (horizstepper == NULL || vertstepper == NULL || dispensestepper == NULL) {
    Serial.println("Error: Failed to initialize stepper! Check pin assignment.");
    while(1); // Halt execution to prevent crashing
}

    if (horizstepper)   {
        horizstepper->setDirectionPin(HORIZ_DIR_PIN);
        horizstepper->setAcceleration(DEFAULT_ACCELERATION);
        horizstepper->setCurrentPosition(0);  
    }

    if (vertstepper)    {
        vertstepper->setDirectionPin(VERT_DIR_PIN);
        vertstepper->setAcceleration(DEFAULT_ACCELERATION);
        vertstepper->setCurrentPosition(0);
    } 

if (dispensestepper)    {
        dispensestepper->setDirectionPin(DISPENSE_DIR_PIN);
        dispensestepper->setAcceleration(DEFAULT_ACCELERATION);
        dispensestepper->setCurrentPosition(0);
    }

    turnoff();
}

// Functions to enable/disable the motors
void enablemotor() {
    horizmotor.enable();
    vertmotor.enable();
}

void disablemotor() {
    horizmotor.disable();
    vertmotor.disable();
    dispensemotor.disable();
}

// Functions to move the motors
void movehoriz(float mm) {
    horizstepper->setSpeedInHz(DEFAULT_SPEED);
    horizstepper->move((int32_t)(mm * 200.0f * (float)MICROSTEPS / 2.0f));
    while (horizstepper->isRunning()) {
        if (abortMotion) {
            horizstepper->forceStopAndNewPosition(horizstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void movetohoriz(float pos) {
    horizstepper->setSpeedInHz(DEFAULT_SPEED);
    horizstepper->moveTo((int32_t)(pos * 200.0f * (float)MICROSTEPS / 2.0f));
    while (horizstepper->isRunning()) {
        if (abortMotion) {
            horizstepper->forceStopAndNewPosition(horizstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void movevert(float mm)    {
    vertstepper->setSpeedInHz(DEFAULT_SPEED);                   // set speed in Hz
    vertstepper->move((int32_t)(mm * 200.0f * (float)MICROSTEPS / 2.0f));
    while (vertstepper->isRunning()) {
        if (abortMotion) {
            vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void movetovert(float pos) {
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    vertstepper->moveTo((int32_t)(pos * 200.0f * (float)MICROSTEPS / 2.0f));
    while (vertstepper->isRunning()) {
        if (abortMotion) {
            vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Function to move the dispensing motor
void movedispense() {
    dispensemotor.enable();
    dispensestepper->setSpeedInHz(3200);
    // 1.5 rotations per the old calculations - change as needed
    // x rotations * 200 steps per rotation * 16 microsteps per step
    dispensestepper->move((int32_t)(1.5f * 200.0f * (float)MICROSTEPS));
    while (dispensestepper->isRunning()) {
        if (abortMotion) {
            dispensestepper->forceStopAndNewPosition(dispensestepper->getCurrentPosition());
            dispensemotor.disable();
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    dispensestepper->move((int32_t)(-1.5f * 200.0f * (float)MICROSTEPS));
    while (dispensestepper->isRunning()) {
        if (abortMotion) {
            dispensestepper->forceStopAndNewPosition(dispensestepper->getCurrentPosition());
            dispensemotor.disable();
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    dispensemotor.disable();
}

// Functions to home the motors to 0
void homehoriz()    {
    horizstepper->setSpeedInHz(DEFAULT_SPEED);
    horizstepper->moveTo(0);
    while (horizstepper->isRunning()) {
        if (abortMotion) {
            horizstepper->forceStopAndNewPosition(horizstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void homevert() {
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    vertstepper->moveTo(0);
    while (vertstepper->isRunning()) {
        if (abortMotion) {
            vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Function to move substrate just above the beaker
void dryvert() {
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    vertstepper->moveTo(-200 * 200 * MICROSTEPS / 2);       // Move 200mm from the top. Adjust as needed.
    while (vertstepper->isRunning()) {
        if (abortMotion) {
            vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Function to withdraw the substrate from solution at a given rate
void withdraw(float mmpermin) {
    vertstepper->setAcceleration(1000000);          // This large number makes it almost instant
    vertstepper->setSpeedInHz((int32_t)(mmpermin/60.0f * 200.0f * (float)MICROSTEPS / 2.0f));    // set speed in Hz
    vertstepper->move(15 * 200 * MICROSTEPS / 2);                       // move 15mm
    while (vertstepper->isRunning() == true) {
        if (abortMotion) {
            vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vertstepper->setAcceleration(DEFAULT_ACCELERATION); 
}

void wait(float sec) {
    vTaskDelay(sec * 1000 / portTICK_PERIOD_MS);
}

void estop() {
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    horizstepper->setSpeedInHz(DEFAULT_SPEED);

    vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
    horizstepper->forceStopAndNewPosition(horizstepper->getCurrentPosition());

    // hardcoded dryvert(); the function does not work because of the abortMotion flag.
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    vertstepper->moveTo(-150 * 200 * MICROSTEPS / 2);   // target = -240000
    while (vertstepper->isRunning() == true) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    turnoff();
    disablemotor();

    abortMotion = false;
}

// Function to utilize the DIAG pins and stop the motors when triggered
void endstop() {
    if (horizstepper->isRunning() && digitalRead(HORIZ_DIAG_PIN) == HIGH) {
        //Serial.println("HORIZ_DIAG triggered");
        horizstepper->forceStopAndNewPosition(0);
    }
    if (vertstepper->isRunning() && digitalRead(VERT_DIAG_PIN) == HIGH) {
        //Serial.println("VERT_DIAG triggered");
        vertstepper->forceStopAndNewPosition(0);
    }
}

// Function that returns the motors to their default positions
void turnoff()  {
    // set speed
    //horizstepper->setSpeedInHz(DEFAULT_SPEED);          // default speed: 40mm/s
    //vertstepper->setSpeedInHz(DEFAULT_SPEED);

    // home motors
    //homehoriz();
    //homevert();

    // sensorless homing
    horizstepper->setSpeedInHz(4000);
    horizmotor.enable();
    horizmotor.enableStealthChop();
    horizstepper->runBackward();

    vertstepper->setSpeedInHz(4000);
    vertmotor.enable();
    vertmotor.enableStealthChop();
    vertstepper->runForward();

        while (horizstepper->isRunning() == true || vertstepper->isRunning() == true) {
        endstop();
        vTaskDelay(10 / portTICK_PERIOD_MS);          // Must call esp_task_wdt_reset to watchdog to prevent watchdog reboot
    }

    horizmotor.disableStealthChop();
    horizmotor.disable();

    vertmotor.disableStealthChop();
    vertmotor.disable();
}