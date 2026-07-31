#include <Arduino.h>
#include "movements.h"
#include "operations.h"
#include "webinterface.h"

// Dedicate a core for WIFI processing, and another for motor control and sensors.

//#define WIFI_CORE 0     // Core 0 is called the PRO_CPU; usually reserved for WIFI processing
//#define TASK_CORE 1     // Core 1 is called the APP_CPU; used for task processing

void setup() {
  Serial.begin(115200);

  delay(1000);

  initMotors();
  setupwifi();
  setupWebServer();
  startMotorTask();

  Serial.println("Setup complete");

  

  /*Serial.println("Initializing Dip Coater...");
  initMotors();
  
  enablemotor();

  turnoff();

  dipSolution1(1, 1, 600);

  turnoff();

  Serial.println("Disabling Motors...");
  disablemotor();
  */
}

void loop() {
  // Main code loop
  handleWebServer();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
