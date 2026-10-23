#pragma once

#include <Arduino.h>

#if defined(BOARD_FREENOVE_FNK0114L)
constexpr uint8_t RGB_LED_COLOR_COUNT = 5;
extern const char *const RGB_LED_COLOR_LABELS[RGB_LED_COLOR_COUNT];

void rgbLedBegin();
void rgbLedLoop(uint32_t now);
void rgbLedSetEnabled(bool enabled);
void rgbLedSetPulse(bool enabled);
void rgbLedSetColor(uint8_t index);
bool rgbLedEnabled();
bool rgbLedPulse();
uint8_t rgbLedColorIndex();
// RGB565 value of a palette entry for drawing swatches.
uint16_t rgbLedColor565(uint8_t index);
// Direct output (0-255) in the selected color, e.g. for app effects.
void rgbLedSetOutput(uint8_t brightness);
#endif
