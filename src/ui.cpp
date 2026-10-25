#include "ui.h"

#include <WiFi.h>

#include "app.h"
#include "audio_player.h"
#include "colors.h"
#include "config.h"
#include "display.h"
#include "network.h"
#include "rgb_led.h"
#include "touch.h"

namespace {

enum class KeyboardTarget : uint8_t { Ssid, Password };

constexpr uint8_t TEST_STREAM_ROW = 4;

Screen screen = Screen::Home;
Screen keyboardReturnScreen = Screen::Wifi;
KeyboardTarget keyboardTarget = KeyboardTarget::Ssid;
bool uiDirty = true;
String statusMessage = "Set up Wi-Fi";

// Credentials are edited here and only stored when CONNECT is pressed.
String editSsid;
String editPassword;
bool showPassword = false;
String keyboardValue;
bool keyboardUppercase = false;
uint8_t keyboardPage = 0;

// ---- Keyboard geometry -----------------------------------------------------

String keyboardRow(uint8_t row) {
  if (keyboardPage == 1) {
    const String symbolRows[] = {"1234567890", "!@#$%^&*()", "-_./:;?+="};
    return symbolRows[row];
  }
  if (keyboardPage == 2) {
    const String symbolRows[] = {"[]{}<>~", "`\\|,", "'\""};
    return symbolRows[row];
  }
  const String letterRows[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
  return letterRows[row];
}

constexpr int16_t KEY_GAP = 2;
constexpr int16_t KEY_HEIGHT = 34;
constexpr int16_t KEYBOARD_CONTROL_Y = 177;
constexpr int16_t KEYBOARD_CONTROL_HEIGHT = 28;
constexpr int16_t KEYBOARD_ACTION_Y = 211;
constexpr int16_t KEYBOARD_ACTION_HEIGHT = 25;
constexpr int16_t SHOW_BUTTON_WIDTH = 70;

int16_t keyboardRowY(uint8_t row) {
  return 65 + row * 37;
}

int16_t keyboardKeyWidth(const String &keys) {
  const int16_t availableWidth = UI_WIDTH - (keys.length() - 1) * KEY_GAP;
  return min<int16_t>(48, availableWidth / keys.length());
}

int16_t keyboardRowLeft(const String &keys) {
  const int16_t rowWidth =
      keys.length() * keyboardKeyWidth(keys) + (keys.length() - 1) * KEY_GAP;
  return UI_LEFT + (UI_WIDTH - rowWidth) / 2;
}

void keyboardControlBounds(uint8_t index, int16_t &x, int16_t &width) {
  constexpr int16_t controlWidths[] = {64, 64, 76, 64};
  constexpr int16_t controlsWidth = 64 + 6 + 64 + 6 + 76 + 6 + 64;
  x = UI_LEFT + (UI_WIDTH - controlsWidth) / 2;
  for (uint8_t i = 0; i < index; ++i) {
    x += controlWidths[i] + 6;
  }
  width = controlWidths[index];
}

uint16_t maximumKeyboardLength() {
  return keyboardTarget == KeyboardTarget::Ssid ? 32 : 63;
}

String passwordForDisplay(const String &password) {
  if (showPassword) {
    return password;
  }
  String masked;
  masked.reserve(password.length());
  for (size_t i = 0; i < password.length(); ++i) {
    masked += '*';
  }
  return masked;
}

// ---- Drawing ---------------------------------------------------------------

void drawHome() {
  displayBeginFrame();
  appDrawHome(*renderTarget);
  drawButton(OPTIONS_BUTTON_X, OPTIONS_BUTTON_Y, OPTIONS_BUTTON_WIDTH,
             OPTIONS_BUTTON_HEIGHT, "OPTIONS", PANEL_LIGHT, TFT_WHITE, 2);
}

void drawSettings() {
  drawPanelHeader("OPTIONS");
  drawMenuBackButton();
  const int16_t rowStep = SETTINGS_ITEM_HEIGHT + SETTINGS_ITEM_GAP;
  const char *labels[SETTINGS_ITEM_COUNT] = {
      "AUDIO", "DISPLAY & TOUCH", "WI-FI",
#if defined(BOARD_FREENOVE_FNK0114L)
      "RGB LED"
#endif
  };
  for (uint8_t row = 0; row < SETTINGS_ITEM_COUNT; ++row) {
    drawButton(UI_LEFT, SETTINGS_ITEM_START_Y + row * rowStep, UI_WIDTH,
               SETTINGS_ITEM_HEIGHT, labels[row], PANEL_LIGHT, TFT_WHITE, 2,
               false);
  }
  drawText(String(BUILD_PROFILE_NAME), UI_LEFT, 228, 1, TEXT_MUTED);
}

void drawAudioSettings() {
  TFT_eSPI &gfx = *renderTarget;
  drawPanelHeader("AUDIO");
  drawMenuBackButton();
  const String labels[] = {
      String("AUDIO BOOST +6 dB: ") + (audioBoostEnabled() ? "ON" : "OFF"),
      String("DYNAMIC COMPRESSION: ") +
          (audioCompressorEnabled() ? "ON" : "OFF"),
      String("GPIO4 AMP: ") + (audioAmpEnabled() ? "ON" : "OFF"),
      String("NOISE SHAPING: ") + (audioNoiseShapingEnabled() ? "ON" : "OFF"),
      audioIsPlaying() ? "STOP TEST STREAM" : "PLAY TEST STREAM"};
  constexpr int16_t TOGGLE_START_Y = 37;
  constexpr int16_t TOGGLE_STEP = 32;
  constexpr int16_t TOGGLE_HEIGHT = 29;
  for (uint8_t row = 0; row < 5; ++row) {
    drawButton(UI_LEFT, TOGGLE_START_Y + row * TOGGLE_STEP, UI_WIDTH,
               TOGGLE_HEIGHT, labels[row],
               row == TEST_STREAM_ROW && audioIsPlaying() ? 0x2147
                                                          : PANEL_LIGHT,
               TFT_WHITE, 2, false);
  }
  const uint8_t volume = audioVolume();
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(TEXT_MUTED, backgroundColor);
  gfx.drawString("VOLUME", UI_LEFT, 194, 2);
  gfx.setTextDatum(TR_DATUM);
  gfx.drawString(String(volume * 100 / MAX_VOLUME) + "%", UI_LEFT + UI_WIDTH,
                 194, 2);
  gfx.fillRoundRect(UI_LEFT, 220, UI_WIDTH, 10, 5,
                    BUTTON_COLORS[buttonColorIndex]);
  const int16_t sliderWidth =
      static_cast<int16_t>((UI_WIDTH - 4) * volume / MAX_VOLUME);
  gfx.fillRoundRect(UI_LEFT + 2, 222, sliderWidth, 6, 3, ACCENT);
  gfx.fillCircle(UI_LEFT + 2 + sliderWidth, 225, 8, TFT_WHITE);
}

constexpr int16_t SWATCH_WIDTH = 50;
constexpr int16_t SWATCH_HEIGHT = 24;
constexpr int16_t SWATCH_GAP = 8;
constexpr int16_t SWATCH_ROW_STEP = 29;
constexpr int16_t BACKGROUND_SWATCH_Y = 53;
constexpr int16_t BUTTON_SWATCH_Y = 128;

int16_t swatchX(uint8_t column) {
  const int16_t paletteWidth =
      COLORS_PER_ROW * SWATCH_WIDTH + (COLORS_PER_ROW - 1) * SWATCH_GAP;
  return UI_LEFT + (UI_WIDTH - paletteWidth) / 2 +
         column * (SWATCH_WIDTH + SWATCH_GAP);
}

void drawSwatches(const uint16_t *colors, uint8_t count, int16_t top,
                  uint8_t selected) {
  TFT_eSPI &gfx = *renderTarget;
  for (uint8_t index = 0; index < count; ++index) {
    const int16_t x = swatchX(index % COLORS_PER_ROW);
    const int16_t y = top + (index / COLORS_PER_ROW) * SWATCH_ROW_STEP;
    gfx.fillRoundRect(x, y, SWATCH_WIDTH, SWATCH_HEIGHT, 4, colors[index]);
    gfx.drawRoundRect(x - 1, y - 1, SWATCH_WIDTH + 2, SWATCH_HEIGHT + 2, 5,
                      index == selected ? primaryTextColor() : 0x4208);
  }
}

// Returns the swatch index under the touch, or -1.
int8_t swatchAt(int16_t x, int16_t y, int16_t top, uint8_t count) {
  for (uint8_t index = 0; index < count; ++index) {
    const int16_t sy = top + (index / COLORS_PER_ROW) * SWATCH_ROW_STEP;
    if (pointInRect(x, y, swatchX(index % COLORS_PER_ROW) - 2, sy - 2,
                    SWATCH_WIDTH + 4, 28)) {
      return index;
    }
  }
  return -1;
}

void drawDisplaySettings() {
  TFT_eSPI &gfx = *renderTarget;
  drawPanelHeader("DISPLAY & TOUCH");
  drawMenuBackButton();
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(primaryTextColor(), backgroundColor);
  gfx.drawString("BACKGROUND", UI_LEFT, 35, 2);
  drawSwatches(THEME_COLORS, THEME_COLOR_COUNT, BACKGROUND_SWATCH_Y,
               themeColorIndex);
  gfx.drawString("BUTTONS", UI_LEFT, 110, 2);
  drawSwatches(BUTTON_COLORS, BUTTON_COLOR_COUNT, BUTTON_SWATCH_Y,
               buttonColorIndex);
  drawButton(UI_LEFT, 194, UI_WIDTH, 36, "RECALIBRATE TOUCHSCREEN",
             PANEL_LIGHT, TFT_WHITE, 2, false);
}

#if defined(BOARD_FREENOVE_FNK0114L)
constexpr int16_t LED_COLOR_WIDTH = 52;
constexpr int16_t LED_COLOR_GAP = 7;
constexpr int16_t LED_COLOR_HEIGHT = 38;
constexpr int16_t LED_COLOR_Y = 150;

int16_t ledColorX(uint8_t index) {
  const int16_t colorsWidth = RGB_LED_COLOR_COUNT * LED_COLOR_WIDTH +
                              (RGB_LED_COLOR_COUNT - 1) * LED_COLOR_GAP;
  return UI_LEFT + (UI_WIDTH - colorsWidth) / 2 +
         index * (LED_COLOR_WIDTH + LED_COLOR_GAP);
}

void drawRgbLedSettings() {
  TFT_eSPI &gfx = *renderTarget;
  drawPanelHeader("RGB LED OPTIONS");
  drawMenuBackButton();
  drawButton(UI_LEFT, 48, UI_WIDTH, 32,
             String("LED: ") + (rgbLedEnabled() ? "ON" : "OFF"), PANEL_LIGHT,
             TFT_WHITE, 2);
  drawButton(UI_LEFT, 88, UI_WIDTH, 32,
             String("SLOW PULSE: ") + (rgbLedPulse() ? "ON" : "OFF"),
             PANEL_LIGHT, TFT_WHITE, 2);
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(primaryTextColor(), backgroundColor);
  gfx.drawString("COLOR", UI_LEFT, 132, 1);
  for (uint8_t index = 0; index < RGB_LED_COLOR_COUNT; ++index) {
    const int16_t x = ledColorX(index);
    const uint16_t color565 = rgbLedColor565(index);
    gfx.fillRoundRect(x, LED_COLOR_Y, LED_COLOR_WIDTH, LED_COLOR_HEIGHT, 5,
                      color565);
    gfx.drawRoundRect(x - 1, LED_COLOR_Y - 1, LED_COLOR_WIDTH + 2,
                      LED_COLOR_HEIGHT + 2, 6,
                      index == rgbLedColorIndex() ? TFT_WHITE : 0x4208);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_WHITE, color565);
    gfx.drawString(RGB_LED_COLOR_LABELS[index], x + LED_COLOR_WIDTH / 2,
                   LED_COLOR_Y + LED_COLOR_HEIGHT / 2, 1);
  }
}
#endif

void drawWifiSettings() {
  TFT_eSPI &gfx = *renderTarget;
  drawPanelHeader("WI-FI SETTINGS");
  drawMenuBackButton();
  const int16_t labelLeft = UI_LEFT + 6;
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(TEXT_MUTED, backgroundColor);
  gfx.drawString("NETWORK NAME (SSID)", labelLeft, 37, 2);
  drawButton(UI_LEFT, 55, UI_WIDTH, 39,
             fittedText(editSsid.isEmpty() ? "(Enter SSID)" : editSsid,
                        UI_WIDTH - 16, 2),
             PANEL, TFT_WHITE, 2, false);
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(TEXT_MUTED, backgroundColor);
  gfx.drawString("PASSWORD", labelLeft, 100, 2);
  String passwordPreview = passwordForDisplay(editPassword);
  if (editPassword.isEmpty()) {
    passwordPreview = "(Enter password)";
  }
  drawButton(UI_LEFT, 118, UI_WIDTH, 39,
             fittedText(passwordPreview, UI_WIDTH - 16, 2), PANEL, TFT_WHITE,
             2, false);
  drawButton(UI_LEFT, 157, UI_WIDTH, 34,
             String("SHOW PASSWORD: ") + (showPassword ? "ON" : "OFF"),
             showPassword ? 0x2147 : PANEL, TFT_WHITE, 2, false);
  drawButton(UI_LEFT, 198, UI_WIDTH, 32, "CONNECT",
             wifiConnecting() ? 0x3186 : ACCENT, TFT_WHITE, 2, false);
  drawText(fittedText(wifiStatusText(), UI_WIDTH, 1), UI_LEFT, 231, 1,
           wifiConnected() ? SUCCESS : TEXT_MUTED);
}

void drawKeyboard() {
  drawPanelHeader(keyboardTarget == KeyboardTarget::Ssid ? "ENTER SSID"
                                                         : "ENTER PASSWORD");
  const bool editingPassword = keyboardTarget == KeyboardTarget::Password;
  const String visibleValue =
      editingPassword ? passwordForDisplay(keyboardValue) : keyboardValue;
  const int16_t valueWidth =
      editingPassword && !showPassword ? UI_WIDTH - 78 : UI_WIDTH;
  drawText(fittedText(visibleValue, valueWidth, 2), UI_LEFT, 38, 2,
           TFT_WHITE);
  if (editingPassword) {
    drawButton(UI_LEFT + UI_WIDTH - SHOW_BUTTON_WIDTH, 32, SHOW_BUTTON_WIDTH,
               30, showPassword ? "HIDE" : "SHOW", PANEL_LIGHT, TFT_WHITE, 1,
               false);
  }

  for (uint8_t row = 0; row < 3; ++row) {
    const String keys = keyboardRow(row);
    const int16_t keyWidth = keyboardKeyWidth(keys);
    const int16_t rowLeft = keyboardRowLeft(keys);
    for (uint8_t column = 0; column < keys.length(); ++column) {
      String label(keys[column]);
      if (keyboardPage == 0 && keyboardUppercase) {
        label.toUpperCase();
      }
      drawButton(rowLeft + column * (keyWidth + KEY_GAP), keyboardRowY(row),
                 keyWidth, KEY_HEIGHT, label, PANEL_LIGHT, TFT_WHITE, 2,
                 false);
    }
  }

  const char *controlLabels[] = {
      keyboardPage == 0 && !keyboardUppercase ? "abc" : "ABC",
      keyboardPage == 0 ? "?123" : keyboardPage == 1 ? "SYM2" : "SYM1",
      "SPACE", "<-"};
  for (uint8_t i = 0; i < 4; ++i) {
    int16_t x = 0;
    int16_t width = 0;
    keyboardControlBounds(i, x, width);
    drawButton(x, KEYBOARD_CONTROL_Y, width, KEYBOARD_CONTROL_HEIGHT,
               controlLabels[i], PANEL_LIGHT, TFT_WHITE, 2, false);
  }
  const int16_t actionWidth = (UI_WIDTH - 8) / 2;
  drawButton(UI_LEFT, KEYBOARD_ACTION_Y, actionWidth, KEYBOARD_ACTION_HEIGHT,
             "CANCEL", PANEL_LIGHT, TFT_WHITE, 1, false);
  drawButton(UI_LEFT + actionWidth + 8, KEYBOARD_ACTION_Y, actionWidth,
             KEYBOARD_ACTION_HEIGHT, "DONE", ACCENT, TFT_WHITE, 1, false);
}

void renderScreen() {
  uiDirty = false;
  if (screen == Screen::Home) {
    drawHome();
  } else {
    displayBeginFrame();
    switch (screen) {
      case Screen::Settings:
        drawSettings();
        break;
      case Screen::AudioSettings:
        drawAudioSettings();
        break;
      case Screen::DisplaySettings:
        drawDisplaySettings();
        break;
#if defined(BOARD_FREENOVE_FNK0114L)
      case Screen::RgbLedSettings:
        drawRgbLedSettings();
        break;
#endif
      case Screen::Wifi:
        drawWifiSettings();
        break;
      case Screen::Keyboard:
        drawKeyboard();
        break;
      default:
        break;
    }
  }
  displayEndFrame();
  if (screen == Screen::Home && animationCanvasReady()) {
    // The frame just overwrote the animation region; redraw it immediately.
    const uint32_t now = millis();
    appDrawAnimation(animationCanvas(), now);
    animationCanvasPush(now);
  }
}

// ---- Touch -----------------------------------------------------------------

void adjustVolumeFromTouch(int16_t x) {
  const int16_t relativeX = constrain(x - UI_LEFT - 2, 0, UI_WIDTH - 4);
  audioSetVolume(static_cast<uint8_t>(
      (relativeX * MAX_VOLUME + (UI_WIDTH - 4) / 2) / (UI_WIDTH - 4)));
}

void toggleTestStream() {
  if (audioIsPlaying()) {
    audioStop();
  } else {
    audioPlayUrl(AUDIO_TEST_STREAM_URL);
  }
}

void beginKeyboard(KeyboardTarget target) {
  keyboardTarget = target;
  keyboardReturnScreen = screen;
  keyboardValue = target == KeyboardTarget::Ssid ? editSsid : editPassword;
  keyboardPage = 0;
  keyboardUppercase = false;
  screen = Screen::Keyboard;
}

void finishKeyboard(bool accept) {
  if (accept) {
    (keyboardTarget == KeyboardTarget::Ssid ? editSsid : editPassword) =
        keyboardValue;
  }
  screen = keyboardReturnScreen;
}

void appendKeyboardCharacter(char value) {
  if (keyboardValue.length() < maximumKeyboardLength()) {
    keyboardValue += value;
  }
}

void handleKeyboardTouch(int16_t x, int16_t y) {
  if (keyboardTarget == KeyboardTarget::Password &&
      pointInRect(x, y, UI_LEFT + UI_WIDTH - SHOW_BUTTON_WIDTH - 8, 26,
                  SHOW_BUTTON_WIDTH + 16, 44)) {
    showPassword = !showPassword;
    return;
  }

  for (uint8_t row = 0; row < 3; ++row) {
    const String keys = keyboardRow(row);
    const int16_t keyWidth = keyboardKeyWidth(keys);
    const int16_t rowLeft = keyboardRowLeft(keys);
    for (uint8_t column = 0; column < keys.length(); ++column) {
      if (pointInRect(x, y, rowLeft + column * (keyWidth + KEY_GAP),
                      keyboardRowY(row), keyWidth, KEY_HEIGHT)) {
        char value = keys[column];
        if (keyboardPage == 0 && keyboardUppercase) {
          value = static_cast<char>(toupper(value));
        }
        appendKeyboardCharacter(value);
        return;
      }
    }
  }

  for (uint8_t i = 0; i < 4; ++i) {
    int16_t controlX = 0;
    int16_t controlWidth = 0;
    keyboardControlBounds(i, controlX, controlWidth);
    if (!pointInRect(x, y, controlX, KEYBOARD_CONTROL_Y, controlWidth,
                     KEYBOARD_CONTROL_HEIGHT)) {
      continue;
    }
    switch (i) {
      case 0:
        if (keyboardPage == 0) {
          keyboardUppercase = !keyboardUppercase;
        } else {
          keyboardPage = 0;
        }
        break;
      case 1:
        keyboardPage = keyboardPage == 0 ? 1 : keyboardPage == 1 ? 2 : 1;
        break;
      case 2:
        appendKeyboardCharacter(' ');
        break;
      case 3:
        if (!keyboardValue.isEmpty()) {
          keyboardValue.remove(keyboardValue.length() - 1);
        }
        break;
    }
    return;
  }

  const int16_t actionWidth = (UI_WIDTH - 8) / 2;
  if (pointInRect(x, y, UI_LEFT, KEYBOARD_ACTION_Y, actionWidth,
                  KEYBOARD_ACTION_HEIGHT)) {
    finishKeyboard(false);
  } else if (pointInRect(x, y, UI_LEFT + actionWidth + 8, KEYBOARD_ACTION_Y,
                         actionWidth, KEYBOARD_ACTION_HEIGHT)) {
    finishKeyboard(true);
  }
}

void handleSettingsTouch(int16_t x, int16_t y) {
  if (pointOnMenuBackButton(x, y)) {
    screen = Screen::Home;
    return;
  }
  const int16_t rowStep = SETTINGS_ITEM_HEIGHT + SETTINGS_ITEM_GAP;
  if (x < UI_LEFT || x >= UI_LEFT + UI_WIDTH || y < SETTINGS_ITEM_START_Y) {
    return;
  }
  const uint8_t row =
      static_cast<uint8_t>((y - SETTINGS_ITEM_START_Y) / rowStep);
  const int16_t rowY = SETTINGS_ITEM_START_Y + row * rowStep;
  if (row >= SETTINGS_ITEM_COUNT ||
      !pointInRect(x, y, UI_LEFT, rowY, UI_WIDTH, SETTINGS_ITEM_HEIGHT)) {
    return;
  }
  switch (row) {
    case 0:
      screen = Screen::AudioSettings;
      break;
    case 1:
      screen = Screen::DisplaySettings;
      break;
    case 2:
      editSsid = wifiSsid();
      editPassword = wifiPassword();
      screen = Screen::Wifi;
      break;
#if defined(BOARD_FREENOVE_FNK0114L)
    case 3:
      screen = Screen::RgbLedSettings;
      break;
#endif
  }
}

void handleTouch(int16_t x, int16_t y) {
  switch (screen) {
    case Screen::Home:
      if (pointInRect(x, y, OPTIONS_BUTTON_X, OPTIONS_BUTTON_Y,
                      OPTIONS_BUTTON_WIDTH, OPTIONS_BUTTON_HEIGHT)) {
        screen = Screen::Settings;
      } else {
        // App touches must not repaint the whole screen (flicker).
        appHandleTouch(x, y);
        return;
      }
      break;
    case Screen::Settings:
      handleSettingsTouch(x, y);
      break;
    case Screen::AudioSettings:
      if (pointOnMenuBackButton(x, y)) {
        screen = Screen::Settings;
      } else if (pointInRect(x, y, UI_LEFT, 37, UI_WIDTH, 29)) {
        audioSetBoost(!audioBoostEnabled());
      } else if (pointInRect(x, y, UI_LEFT, 69, UI_WIDTH, 29)) {
        audioSetCompressor(!audioCompressorEnabled());
      } else if (pointInRect(x, y, UI_LEFT, 101, UI_WIDTH, 29)) {
        audioSetAmp(!audioAmpEnabled());
      } else if (pointInRect(x, y, UI_LEFT, 133, UI_WIDTH, 29)) {
        audioSetNoiseShaping(!audioNoiseShapingEnabled());
      } else if (pointInRect(x, y, UI_LEFT, 165, UI_WIDTH, 29)) {
        toggleTestStream();
      } else if (pointInRect(x, y, UI_LEFT, 194, UI_WIDTH, 46)) {
        adjustVolumeFromTouch(x);
      }
      break;
    case Screen::DisplaySettings: {
      if (pointOnMenuBackButton(x, y)) {
        screen = Screen::Settings;
      } else if (pointInRect(x, y, UI_LEFT, 51, UI_WIDTH, 56)) {
        const int8_t index =
            swatchAt(x, y, BACKGROUND_SWATCH_Y, THEME_COLOR_COUNT);
        if (index >= 0) {
          setThemeColor(index);
        }
      } else if (pointInRect(x, y, UI_LEFT, 126, UI_WIDTH, 57)) {
        const int8_t index =
            swatchAt(x, y, BUTTON_SWATCH_Y, BUTTON_COLOR_COUNT);
        if (index >= 0) {
          setButtonColor(index);
        }
      } else if (pointInRect(x, y, UI_LEFT, 194, UI_WIDTH, 36)) {
        touchCalibrate();
      }
      break;
    }
#if defined(BOARD_FREENOVE_FNK0114L)
    case Screen::RgbLedSettings:
      if (pointOnMenuBackButton(x, y)) {
        screen = Screen::Settings;
      } else if (pointInRect(x, y, UI_LEFT, 48, UI_WIDTH, 32)) {
        rgbLedSetEnabled(!rgbLedEnabled());
      } else if (pointInRect(x, y, UI_LEFT, 88, UI_WIDTH, 32)) {
        rgbLedSetPulse(!rgbLedPulse());
      } else if (pointInRect(x, y, UI_LEFT, 149, UI_WIDTH, 40)) {
        for (uint8_t index = 0; index < RGB_LED_COLOR_COUNT; ++index) {
          if (pointInRect(x, y, ledColorX(index) - 1, 149,
                          LED_COLOR_WIDTH + 2, 40)) {
            rgbLedSetColor(index);
            break;
          }
        }
      }
      break;
#endif
    case Screen::Wifi:
      if (pointOnMenuBackButton(x, y)) {
        screen = Screen::Settings;
      } else if (pointInRect(x, y, UI_LEFT, 55, UI_WIDTH, 39)) {
        beginKeyboard(KeyboardTarget::Ssid);
      } else if (pointInRect(x, y, UI_LEFT, 118, UI_WIDTH, 39)) {
        beginKeyboard(KeyboardTarget::Password);
      } else if (pointInRect(x, y, UI_LEFT, 157, UI_WIDTH, 38)) {
        showPassword = !showPassword;
      } else if (pointInRect(x, y, UI_LEFT, 198, UI_WIDTH, 32)) {
        wifiConnect(editSsid, editPassword);
      }
      break;
    case Screen::Keyboard:
      handleKeyboardTouch(x, y);
      break;
  }
  markUiDirty();
}

}  // namespace

void uiBegin() {
  markUiDirty();
}

void uiLoop(uint32_t now) {
  int16_t x = 0;
  int16_t y = 0;
  if (touchRead(x, y)) {
    handleTouch(x, y);
  }

  if (uiDirty) {
    renderScreen();
  } else if (screen == Screen::Home && animationCanvasDue(now)) {
    appDrawAnimation(animationCanvas(), now);
    animationCanvasPush(now);
  }
}

void markUiDirty() {
  uiDirty = true;
}

void setStatus(const String &message) {
  statusMessage = message;
  Serial.println(message);
  markUiDirty();
}

const String &statusText() {
  return statusMessage;
}

Screen currentScreen() {
  return screen;
}
