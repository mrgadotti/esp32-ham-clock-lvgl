#pragma once
#include <Arduino.h>

bool touchCalLoadSaved();  // applies the NVS "tcal" blob if present; false when missing
void touchCalRun(void (*onDone)(bool saved));  // blocking 3-point calibration UI; saves NVS "tcal"
