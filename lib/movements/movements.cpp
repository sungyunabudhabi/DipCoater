#include <Arduino.h>
#include <TMC2209.h>
#include <FastAccelStepper.h>
#include <esp_task_wdt.h>
#include <ESP32Servo.h>
#include "movements.h"
#include "dispenser.h"

volatile bool abortMotion = false;

// Declare pin numbers from the ESP32 GPIO pins
const uint8_t HORIZ_STEP_PIN = 14;
const uint8_t HORIZ_DIR_PIN = 27;
const uint8_t VERT_STEP_PIN = 26;
const uint8_t VERT_DIR_PIN = 25;
const uint8_t DISPENSE_STEP_PIN = 13;
const uint8_t DISPENSE_DIR_PIN = 12;
const uint8_t HORIZ_DIAG_PIN = 33;
const uint8_t VERT_DIAG_PIN = 32;
int MICROSTEPS = 16;
//const uint8_t HORIZ_ENDSTOP_PIN = 33;
//const uint8_t VERT_ENDSTOP_PIN = 32;

// Declare motor and stepper objects.
// The TMC2209 objects essentially control the motor using UART communication. This includes:
// Microstepping, enable/disable, current limits, and sensorless homing.
TMC2209 horizmotor;
TMC2209 vertmotor;
TMC2209 dispensemotor;

// The FastAccelStepper object control the movements of the motors with step pulses. Its primary functions include:
// Setting speed and acceleration (in Hz), returning motor status, moving a vertain distance or to a certain position.
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

    vertmotor.setup(Serial2, 115200, TMC2209::SERIAL_ADDRESS_0, 16, 17);       // assign the address
    horizmotor.setup(Serial2, 115200, TMC2209::SERIAL_ADDRESS_1, 16, 17);
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

    // This test fails. I suspect that the UART connection is improperly wired, since it still takes UART commands
    // and moves the motor as intended. Will test the PCB version to verify
    if (dispensemotor.isSetupAndCommunicating()) {
    Serial.println("Dispense motor setup and communication successful.");
    } else {
    Serial.println("Error: Failed to setup or communicate with dispense motor.");
    }

    // TMC2209 Library Initialization
    horizmotor.setMicrostepsPerStep(MICROSTEPS);                        // set microsteps per step
    horizmotor.setRunCurrent(80);                                       // set current cap at 80%
    horizmotor.disableStealthChop();                                    // enabling stealthchop causes motor to skip steps at higher speeds (around 5000Hz and above)
    horizmotor.setStallGuardThreshold(60);                              // set stallguard threshold (sensitivity for detecting stalls)
    horizmotor.setCoolStepDurationThreshold(0xFFFFF);
    horizmotor.enableInverseMotorDirection();                           // set right axis as positive
    
    vertmotor.setMicrostepsPerStep(MICROSTEPS);
    vertmotor.setRunCurrent(80);
    vertmotor.disableStealthChop();
    vertmotor.setStallGuardThreshold(60);
    vertmotor.setCoolStepDurationThreshold(0xFFFFF);
    vertmotor.enableInverseMotorDirection();                            // set axis pointing up as positive

    dispensemotor.setMicrostepsPerStep(MICROSTEPS);
    dispensemotor.setRunCurrent(80);

    // FastAccelStepper Initialization
    engine.init();

    // Initialize stepper objects
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

    // Return motors to their default position as part of initMotors() routine.
    homeservo();
    turnoff();
}

// Functions to enable/disable the motors
void enablemotor() {
    horizmotor.enable();
    vertmotor.enable();
    dispensemotor.enable();
}

void disablemotor() {
    horizmotor.disable();
    vertmotor.disable();
    dispensemotor.disable();
}

// Functions to move the motors
// move() function moves the motor the specified number of steps
// moveTo() function moves the motor to the step position from 0.
void movehoriz(float mm) {
    horizstepper->setSpeedInHz(DEFAULT_SPEED);          // Notice the -> instead of . since these are pointers
    horizstepper->move((int32_t)(mm * 200.0f * (float)MICROSTEPS / 2.0f));
    while (horizstepper->isRunning()) {
        if (abortMotion) {
            horizstepper->forceStopAndNewPosition(horizstepper->getCurrentPosition());
            return;   // bail out of this function immediately
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);            // This 10ms delay is critical. 
                                                        // There is a watchdog in the ESP32 software that repeatedly reboots the chip indefinitely when
                                                        // a task takes up the computing core for more than 2-3 seconds.
                                                        // This delay lets the watchdog know that the task is not stuck in a loop forever.
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

// Function to move the dispensing motor (in rotations, positive is dispensing and negative is refilling)
void movedispense(float rot) {
    dispensemotor.enable();
    if (dispensemotor.isSetupAndCommunicating()) {
        Serial.println("Dispense motor setup and communication successful.");
    } else {
        Serial.println("Error: Failed to setup or communicate with dispense motor.");
    }
    dispensestepper->setSpeedInHz(3200);
    Serial.printf("movedispense: rot=%.2f, pos_before=%ld, speed=%lu\n",
                  rot, dispensestepper->getCurrentPosition(), dispensestepper->getSpeedInMilliHz());
    dispensestepper->move((int32_t)(rot * 200.0f * (float)MICROSTEPS));
    while (dispensestepper->isRunning()) {
        if (abortMotion) {
            dispensestepper->forceStopAndNewPosition(dispensestepper->getCurrentPosition());
            dispensemotor.disable();
            Serial.println("movedispense: aborted");
            return;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    Serial.printf("movedispense: pos_after=%ld\n", dispensestepper->getCurrentPosition());
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
    vertstepper->moveTo(-190 * 200 * MICROSTEPS / 2);       // Move to 190mm from the top. Adjust as needed.
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
    vertstepper->setAcceleration(1000000);          // This large number makes speed almost constant, instead of having a gradual increase in speed.
    vertstepper->setSpeedInHz((int32_t)(mmpermin/60.0f * 200.0f * (float)MICROSTEPS / 2.0f));    // set speed in Hz
    vertstepper->move(15 * 200 * MICROSTEPS / 2);                       // move 15mm. Adjust as needed.
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

// Emergency stop logic
void estop() {
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    horizstepper->setSpeedInHz(DEFAULT_SPEED);

    vertstepper->forceStopAndNewPosition(vertstepper->getCurrentPosition());
    horizstepper->forceStopAndNewPosition(horizstepper->getCurrentPosition());

    homeservo();
    // Move vertmotor up first, to prevent the gripper from accidentally tipping over the beakers.
    vertstepper->setSpeedInHz(DEFAULT_SPEED);
    if (vertstepper->getCurrentPosition() < -50 * 200 * MICROSTEPS / 2) {
        vertstepper->moveTo(-50 * 200 * MICROSTEPS / 2);
        while (vertstepper->isRunning() == true) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
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
    // Sensorless homing
    // Sensorless homing requires StealthChop to be enabled to function. Do not increase the speed above 4000
    // because the motor will skip steps during StealthChop and eventually get damaged.
    horizstepper->setSpeedInHz(4000);
    horizmotor.enable();
    horizmotor.enableStealthChop();
    // Run backwards indefinitely until the sensorless homing is triggered.
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