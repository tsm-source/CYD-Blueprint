#pragma once

#include <Arduino.h>

enum class Screen : uint8_t {
  Home,
  Settings,
  AudioSettings,
  DisplaySettings,
  RgbLedSettings,
  Wifi,
  Keyboard
};

void uiBegin();
void uiLoop(uint32_t now);
void markUiDirty();
void setStatus(const String &message);
const String &statusText();
Screen currentScreen();
