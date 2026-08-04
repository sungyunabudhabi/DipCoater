#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <WiFi.h>
#include <WebServer.h>

extern WebServer server;

void setupwifi();
void setupWebServer();
void handleWebServer();   // call every loop() iteration
void startMotorTask();    // call once from setup()
void startDispenserTask(); // call once from setup()

#endif