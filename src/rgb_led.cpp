#include "rgb_led.h"

#if defined(BOARD_FREENOVE_FNK0114L)

#include "settings.h"
#include "ui.h"

namespace {

constexpr uint8_t RGB_LED_RED_PIN = 22;
constexpr uint8_t RGB_LED_GREEN_PIN = 16;
constexpr uint8_t RGB_LED_BLUE_PIN = 17;
constexpr uint8_t RGB_LED_RED_CHANNEL = 5;
constexpr uint8_t RGB_LED_GREEN_CHANNEL = 6;
constexpr uint8_t RGB_LED_BLUE_CHANNEL = 7;
constexpr uint32_t RGB_LED_PULSE_PERIOD_MS = 4000;
constexpr uint32_t RGB_LED_UPDATE_INTERVAL_MS = 30;

struct RgbColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};
constexpr RgbColor RGB_LED_COLORS[RGB_LED_COLOR_COUNT] = {
    {255, 36, 18}, {0, 190, 255}, {154, 62, 255}, {255, 132, 18},
    {36, 255, 92}};

uint8_t colorIndex = 0;
bool enabled = false;
bool pulse = false;
uint32_t lastUpdateAt = 0;

}  // namespace

const char *const RGB_LED_COLOR_LABELS[RGB_LED_COLOR_COUNT] = {
    "RED", "CYAN", "VIOLET", "AMBER", "GREEN"};

// The LED is common-anode, so the PWM duty is inverted.
void rgbLedSetOutput(uint8_t brightness) {
  const RgbColor color = RGB_LED_COLORS[colorIndex];
  ledcWrite(RGB_LED_RED_CHANNEL, 255 - color.red * brightness / 255);
  ledcWrite(RGB_LED_GREEN_CHANNEL, 255 - color.green * brightness / 255);
  ledcWrite(RGB_LED_BLUE_CHANNEL, 255 - color.blue * brightness / 255);
}

void rgbLedBegin() {
  enabled = preferences.getBool("rgbEnabled", false);
  pulse = preferences.getBool("rgbPulse", false);
  colorIndex = min<uint8_t>(preferences.getUChar("rgbColor", 0),
                            RGB_LED_COLOR_COUNT - 1);
  ledcSetup(RGB_LED_RED_CHANNEL, 5000, 8);
  ledcSetup(RGB_LED_GREEN_CHANNEL, 5000, 8);
  ledcSetup(RGB_LED_BLUE_CHANNEL, 5000, 8);
  ledcAttachPin(RGB_LED_RED_PIN, RGB_LED_RED_CHANNEL);
  ledcAttachPin(RGB_LED_GREEN_PIN, RGB_LED_GREEN_CHANNEL);
  ledcAttachPin(RGB_LED_BLUE_PIN, RGB_LED_BLUE_CHANNEL);
  rgbLedSetOutput(0);
}

void rgbLedLoop(uint32_t now) {
  if (now - lastUpdateAt < RGB_LED_UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdateAt = now;
  if (!enabled) {
    rgbLedSetOutput(0);
    return;
  }
  if (!pulse) {
    rgbLedSetOutput(72);
    return;
  }
  const uint32_t phase = now % RGB_LED_PULSE_PERIOD_MS;
  const uint32_t halfPeriod = RGB_LED_PULSE_PERIOD_MS / 2;
  const uint32_t distance =
      phase < halfPeriod ? phase : RGB_LED_PULSE_PERIOD_MS - phase;
  const uint32_t ramp = distance * 255 / halfPeriod;
  rgbLedSetOutput(
      static_cast<uint8_t>(16 + (ramp * ramp * 104) / (255 * 255)));
}

void rgbLedSetEnabled(bool value) {
  enabled = value;
  if (settingSaved(preferences.putBool("rgbEnabled", enabled),
                   "RGB LED setting")) {
    markUiDirty();
  }
}

void rgbLedSetPulse(bool value) {
  pulse = value;
  if (settingSaved(preferences.putBool("rgbPulse", pulse),
                   "RGB LED pulse setting")) {
    markUiDirty();
  }
}

void rgbLedSetColor(uint8_t index) {
  if (index >= RGB_LED_COLOR_COUNT || index == colorIndex) {
    return;
  }
  colorIndex = index;
  if (settingSaved(preferences.putUChar("rgbColor", colorIndex),
                   "RGB LED color")) {
    markUiDirty();
  }
}

bool rgbLedEnabled() { return enabled; }
bool rgbLedPulse() { return pulse; }
uint8_t rgbLedColorIndex() { return colorIndex; }

uint16_t rgbLedColor565(uint8_t index) {
  const RgbColor c = RGB_LED_COLORS[index];
  return ((c.red & 0xF8) << 8) | ((c.green & 0xFC) << 3) | (c.blue >> 3);
}

#endif
