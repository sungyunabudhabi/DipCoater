#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <Arduino.h>

// Global position variables (defined in operations.cpp)
extern float beaker1position;
extern float beaker2position;
extern float beaker3position;
extern float beaker4position;
extern float dipDistance;

// Dipping Process Function Prototypes
void dipSolution1(float diptime, float drytime, float mmpermin);
void dipSolution2(float diptime, float drytime);
void dipSolution3(float diptime, float drytime);
void dipSolution4(float diptime, float drytime);

#endif // OPERATIONS_H