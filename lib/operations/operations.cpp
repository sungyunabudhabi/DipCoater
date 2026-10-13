#include <Arduino.h>
#include "operations.h"
#include "movements.h"

float beaker1position = 50.0;
float beaker2position = 100.0;
float beaker3position = 150.0;
float beaker4position = 200.0;
float dipDistance = 252.0;          // Adjust after making the overhang. To the bottom of the beaker: 253.0
                                    // 1. adjusted from 253 to 252mm, since grippers tend to be submerged in solution.

void dipSolution1(float diptime, float drytime, float mmpermin) {
    movetohoriz(beaker1position);
    if (abortMotion) return;
    movetovert(-dipDistance);
    if (abortMotion) return;
    wait(diptime);
    if (abortMotion) return;
    withdraw(mmpermin);
    if (abortMotion) return;
    dryvert();                 // Adjust the position once distances are calibrated.
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
    withdraw(100);              // Set speed at 100mm/min. Default speed might be too fast
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
    withdraw(100);              // Set speed at 100mm/min. Default speed might be too fast
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