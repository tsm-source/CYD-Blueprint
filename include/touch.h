#pragma once

#include <Arduino.h>

void touchBegin();
// Returns true once per press with calibrated screen coordinates.
bool touchRead(int16_t &x, int16_t &y);
// Blocking five-point calibration; also stores the result in NVS.
void touchCalibrate();
