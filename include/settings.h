#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Shared NVS storage. Station-style bulk data (URLs, lists) belongs here and
// not in RAM: read it on demand into a stack buffer instead of keeping copies.
extern Preferences preferences;

// Log and report a failed write; returns true when `written` is non-zero.
bool settingSaved(size_t written, const char *what);
