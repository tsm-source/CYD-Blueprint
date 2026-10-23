#include <Arduino.h>

#include "app.h"
#include "audio_player.h"
#include "config.h"
#include "display.h"
#include "network.h"
#include "rgb_led.h"
#include "settings.h"
#include "touch.h"
#include "ui.h"

void setup() {
  Serial.begin(115200);
  Serial.printf("Booting CYD Blueprint. Display: %s; profile: %s.\n",
                USER_SETUP_INFO, BUILD_PROFILE_NAME);

  displayBegin();
  if (!preferences.begin(PREFS_NAMESPACE, false)) {
    Serial.println("ERROR: could not open preferences storage.");
  }
  displayLoadSettings();
  display.fillScreen(backgroundColor);

  touchBegin();
  audioBegin();
#if defined(BOARD_FREENOVE_FNK0114L)
  rgbLedBegin();
#endif
  networkBegin();
  uiBegin();
  appSetup();
}

void loop() {
  audioLoop();
  networkLoop();
  const uint32_t now = millis();
#if defined(BOARD_FREENOVE_FNK0114L)
  rgbLedLoop(now);
#endif
  appLoop(now);
  uiLoop(now);
  delay(1);
}
