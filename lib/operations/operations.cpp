#include <Arduino.h>
#include "operations.h"
#include "movements.h"

// These are the default positions (in mm) of the beakers and the end position of the dipping arm,
// relative to the initial positions of the two motors.
float beaker1position = 50.0;
float beaker2position = 100.0;
float beaker3position = 150.0;
float beaker4position = 200.0;
float dipDistance = 252.0;          // Adjust after making the overhang. default: 253.0
float solution2mmpermin = 100.0;
float solution3mmpermin = 100.0;

void dipSolution1(float diptime, float drytime, float mmpermin) {
    movetohoriz(beaker1position);
    if (abortMotion) return;
    movetovert(-dipDistance);
    if (abortMotion) return;
    wait(diptime);
    if (abortMotion) return;
    withdraw(mmpermin);
    if (abortMotion) return;
    dryvert();
    if (abortMotion) return;
    wait(drytime);
    if (abortMotion) return;
}

void dipSolution2(float diptime, float drytime) {
    movetohoriz(beaker2position);
    if (abortMotion) return;
    movetovert(-dipDistance);
    if (abortMotion) return;
    wait(diptime);
    if (abortMotion) return;
    withdraw(solution2mmpermin);
    if (abortMotion) return;
    dryvert();
    if (abortMotion) return;
    wait(drytime);
    if (abortMotion) return;
}

void dipSolution3(float diptime, float drytime) {
    movetohoriz(beaker3position);
    if (abortMotion) return;
    movetovert(-dipDistance);
    if (abortMotion) return;
    wait(diptime);
    if (abortMotion) return;
    withdraw(solution3mmpermin);
    if (abortMotion) return;
    dryvert();
    if (abortMotion) return;
    wait(drytime);
    if (abortMotion) return;
}

void dipSolution4(float diptime, float drytime) {
    movetohoriz(beaker4position);
    if (abortMotion) return;
    movetovert(-dipDistance);
    if (abortMotion) return;
    wait(diptime);
    if (abortMotion) return;
    dryvert();
    if (abortMotion) return;
    wait(drytime);
    if (abortMotion) return;
}