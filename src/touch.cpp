#include "touch.h"

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>

#include "config.h"
#include "display.h"
#include "settings.h"

namespace {

constexpr uint8_t TOUCH_CALIBRATION_TARGET_COUNT = 5;
constexpr int16_t TOUCH_CALIBRATION_X[TOUCH_CALIBRATION_TARGET_COUNT] = {
    48, 271, 271, 48, DISPLAY_WIDTH / 2};
constexpr int16_t TOUCH_CALIBRATION_Y[TOUCH_CALIBRATION_TARGET_COUNT] = {
    36, 36, 203, 203, DISPLAY_HEIGHT / 2};
constexpr uint8_t TOUCH_CALIBRATION_VERSION = 1;

#if defined(BOARD_FREENOVE_FNK0114L)
// The FNK0114L touch controller is driven by TFT_eSPI on the display bus.
constexpr uint16_t FNK_TOUCH_MIN_Z = 350;
constexpr char CAL_PREFIX[] = "ft";
#else
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(33, 36);
constexpr char CAL_PREFIX[] = "touch";
#endif

struct TouchCalibration {
  float xRawX = 0;
  float xRawY = 0;
  float xOffset = 0;
  float yRawX = 0;
  float yRawY = 0;
  float yOffset = 0;
  bool valid = false;
};

TouchCalibration calibration{};
bool touchHeld = false;
uint32_t lastTouchAt = 0;

String calKey(const char *suffix) {
  return String(CAL_PREFIX) + suffix;
}

bool touchscreenTouched() {
#if defined(BOARD_FREENOVE_FNK0114L)
  return display.getTouchRawZ() > FNK_TOUCH_MIN_Z;
#else
  return touch.touched();
#endif
}

void readRawTouch(uint16_t &x, uint16_t &y) {
#if defined(BOARD_FREENOVE_FNK0114L)
  display.getTouchRaw(&x, &y);
#else
  const TS_Point point = touch.getPoint();
  x = point.x;
  y = point.y;
#endif
}

bool isCalibrationValid(const TouchCalibration &c) {
  const float determinant = c.xRawX * c.yRawY - c.xRawY * c.yRawX;
  return isfinite(c.xRawX) && isfinite(c.xRawY) && isfinite(c.xOffset) &&
         isfinite(c.yRawX) && isfinite(c.yRawY) && isfinite(c.yOffset) &&
         fabsf(determinant) > 1e-6f;
}

// Least-squares affine fit from raw touch readings to screen coordinates.
bool fitCalibration(const uint16_t *rawX, const uint16_t *rawY,
                    TouchCalibration &fit) {
  double meanX = 0;
  double meanY = 0;
  for (uint8_t i = 0; i < TOUCH_CALIBRATION_TARGET_COUNT; ++i) {
    meanX += rawX[i];
    meanY += rawY[i];
  }
  meanX /= TOUCH_CALIBRATION_TARGET_COUNT;
  meanY /= TOUCH_CALIBRATION_TARGET_COUNT;

  double xx = 0, xy = 0, yy = 0;
  double xTargetX = 0, yTargetX = 0, xTargetY = 0, yTargetY = 0;
  for (uint8_t i = 0; i < TOUCH_CALIBRATION_TARGET_COUNT; ++i) {
    const double dx = rawX[i] - meanX;
    const double dy = rawY[i] - meanY;
    const double targetX = TOUCH_CALIBRATION_X[i];
    const double targetY = TOUCH_CALIBRATION_Y[i];
    xx += dx * dx;
    xy += dx * dy;
    yy += dy * dy;
    xTargetX += dx * targetX;
    yTargetX += dy * targetX;
    xTargetY += dx * targetY;
    yTargetY += dy * targetY;
  }
  const double determinant = xx * yy - xy * xy;
  if (xx <= 0 || yy <= 0 || determinant <= 1e-6 * xx * yy) {
    return false;
  }

  fit.xRawX = (yy * xTargetX - xy * yTargetX) / determinant;
  fit.xRawY = (xx * yTargetX - xy * xTargetX) / determinant;
  fit.xOffset = DISPLAY_WIDTH / 2.0 - fit.xRawX * meanX - fit.xRawY * meanY;
  fit.yRawX = (yy * xTargetY - xy * yTargetY) / determinant;
  fit.yRawY = (xx * yTargetY - xy * xTargetY) / determinant;
  fit.yOffset = DISPLAY_HEIGHT / 2.0 - fit.yRawX * meanX - fit.yRawY * meanY;
  fit.valid = isfinite(fit.xRawX) && isfinite(fit.xRawY) &&
              isfinite(fit.xOffset) && isfinite(fit.yRawX) &&
              isfinite(fit.yRawY) && isfinite(fit.yOffset);
  return fit.valid;
}

void drawCalibrationTarget(uint8_t index) {
  display.fillScreen(backgroundColor);
  display.setTextDatum(TL_DATUM);
  display.setTextColor(primaryTextColor(), backgroundColor);
  display.drawString("Touch calibration", 24, 4, 2);
  display.drawString("Touch target " + String(index + 1) + "/" +
                         String(TOUCH_CALIBRATION_TARGET_COUNT),
                     24, 22, 2);
  const int16_t targetX = TOUCH_CALIBRATION_X[index];
  const int16_t targetY = TOUCH_CALIBRATION_Y[index];
  display.drawCircle(targetX, targetY, 13, TFT_CYAN);
  display.drawFastHLine(targetX - 18, targetY, 37, TFT_CYAN);
  display.drawFastVLine(targetX, targetY - 18, 37, TFT_CYAN);
}

void waitForTouchRelease() {
  while (touchscreenTouched()) {
    delay(10);
  }
  delay(120);
}

// Average a burst of raw readings for one calibration target.
void sampleTarget(uint16_t &outX, uint16_t &outY) {
  uint32_t sumX = 0;
  uint32_t sumY = 0;
  uint8_t samples = 0;
  while (samples == 0) {
    while (!touchscreenTouched()) {
      delay(10);
    }
    for (uint8_t sample = 0; sample < 8; ++sample) {
      if (touchscreenTouched()) {
        uint16_t rawX = 0;
        uint16_t rawY = 0;
        readRawTouch(rawX, rawY);
        sumX += rawX;
        sumY += rawY;
        ++samples;
      }
      delay(8);
    }
  }
  outX = sumX / samples;
  outY = sumY / samples;
}

void loadCalibration() {
  if (preferences.getUChar(calKey("Ver").c_str(), 0) !=
      TOUCH_CALIBRATION_VERSION) {
    return;
  }
  calibration.xRawX = preferences.getFloat(calKey("Xx").c_str(), 0);
  calibration.xRawY = preferences.getFloat(calKey("Xy").c_str(), 0);
  calibration.xOffset = preferences.getFloat(calKey("Xo").c_str(), 0);
  calibration.yRawX = preferences.getFloat(calKey("Yx").c_str(), 0);
  calibration.yRawY = preferences.getFloat(calKey("Yy").c_str(), 0);
  calibration.yOffset = preferences.getFloat(calKey("Yo").c_str(), 0);
  calibration.valid = isCalibrationValid(calibration);
}

}  // namespace

void touchBegin() {
#if !defined(BOARD_FREENOVE_FNK0114L)
  touchSPI.begin(25, 39, 32, 33);
  if (!touch.begin(touchSPI)) {
    Serial.println("ERROR: XPT2046 touch controller initialization failed.");
  }
  touch.setRotation(1);
#endif
  loadCalibration();
  if (calibration.valid) {
    Serial.println("Touchscreen calibration loaded.");
  } else {
    touchCalibrate();
  }
}

void touchCalibrate() {
  for (;;) {
    uint16_t measuredX[TOUCH_CALIBRATION_TARGET_COUNT]{};
    uint16_t measuredY[TOUCH_CALIBRATION_TARGET_COUNT]{};
    for (uint8_t target = 0; target < TOUCH_CALIBRATION_TARGET_COUNT;
         ++target) {
      drawCalibrationTarget(target);
      waitForTouchRelease();
      sampleTarget(measuredX[target], measuredY[target]);
      waitForTouchRelease();
    }
    if (fitCalibration(measuredX, measuredY, calibration)) {
      break;
    }
    display.fillScreen(TFT_RED);
    display.setTextColor(TFT_WHITE, TFT_RED);
    display.drawCentreString("Calibration failed", DISPLAY_WIDTH / 2,
                             DISPLAY_HEIGHT / 2, 2);
    Serial.println("ERROR: touchscreen calibration range is invalid.");
    delay(1500);
  }

  const bool saved =
      preferences.putFloat(calKey("Xx").c_str(), calibration.xRawX) > 0 &&
      preferences.putFloat(calKey("Xy").c_str(), calibration.xRawY) > 0 &&
      preferences.putFloat(calKey("Xo").c_str(), calibration.xOffset) > 0 &&
      preferences.putFloat(calKey("Yx").c_str(), calibration.yRawX) > 0 &&
      preferences.putFloat(calKey("Yy").c_str(), calibration.yRawY) > 0 &&
      preferences.putFloat(calKey("Yo").c_str(), calibration.yOffset) > 0 &&
      preferences.putUChar(calKey("Ver").c_str(),
                           TOUCH_CALIBRATION_VERSION) > 0;
  if (!saved) {
    Serial.println("ERROR: could not save touchscreen calibration.");
  }
}

bool touchRead(int16_t &x, int16_t &y) {
  if (!touchscreenTouched()) {
    touchHeld = false;
    return false;
  }
  if (touchHeld) {
    return false;
  }
  touchHeld = true;

  const uint32_t now = millis();
  if (now - lastTouchAt < TOUCH_DEBOUNCE_MS) {
    return false;
  }
  lastTouchAt = now;

  uint32_t sumX = 0;
  uint32_t sumY = 0;
  uint8_t samples = 0;
#if defined(BOARD_FREENOVE_FNK0114L)
  constexpr uint8_t MAX_SAMPLES = 1;
#else
  constexpr uint8_t MAX_SAMPLES = 3;
#endif
  for (uint8_t i = 0; i < MAX_SAMPLES && touchscreenTouched(); ++i) {
    uint16_t rawX = 0;
    uint16_t rawY = 0;
    readRawTouch(rawX, rawY);
    sumX += rawX;
    sumY += rawY;
    ++samples;
    if (MAX_SAMPLES > 1) {
      delayMicroseconds(1200);
    }
  }
  if (samples == 0) {
    return false;
  }
  const float rawX = sumX / static_cast<float>(samples);
  const float rawY = sumY / static_cast<float>(samples);
  x = constrain(lroundf(calibration.xRawX * rawX + calibration.xRawY * rawY +
                        calibration.xOffset),
                0, DISPLAY_WIDTH - 1);
  y = constrain(lroundf(calibration.yRawX * rawX + calibration.yRawY * rawY +
                        calibration.yOffset),
                0, DISPLAY_HEIGHT - 1);
  return true;
}
