#include "display.h"

#include "config.h"
#include "settings.h"
#include "ui.h"

TFT_eSPI display;
TFT_eSPI *renderTarget = &display;
uint16_t backgroundColor = COLOR_DARK_GREY;
uint8_t themeColorIndex = 1;
uint8_t buttonColorIndex = 0;

const uint16_t THEME_COLORS[THEME_COLOR_COUNT] = {
    COLOR_DARK_BLUE, COLOR_DARK_GREY, COLOR_DARK_GREEN, COLOR_DARK_RED,
    COLOR_DARK_SLATE, 0x1865, 0x0925, 0x1901, 0x2843, 0x20E2};
const uint16_t BUTTON_COLORS[BUTTON_COLOR_COUNT] = {
    0x2945, 0x1908, 0x1A28, 0x4148, 0x5182, 0x0372, 0x680A, 0x8A81,
    0x0B26, 0x496F};

namespace {

// 8-bit sprites halve the memory of RGB565 sprites; the low-memory profile
// skips the full-screen sprite entirely and keeps the heap free for TLS.
TFT_eSprite screenFrameBuffer(&display);
bool framebufferReady = false;

TFT_eSprite animationSprite(&display);
bool animationReady = false;
int16_t animationX = 0;
int16_t animationY = 0;
uint16_t animationIntervalMs = 40;
uint32_t lastAnimationAt = 0;

}  // namespace

void displayBegin() {
  display.init();
  display.setRotation(1);
  display.fillScreen(TFT_BLACK);
#if !defined(CYD_LOW_MEMORY_DIRECT_RENDER)
  screenFrameBuffer.setColorDepth(8);
  framebufferReady =
      screenFrameBuffer.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  if (framebufferReady) {
    Serial.printf("Display uses %u-byte screen frame buffer.\n",
                  static_cast<unsigned>(DISPLAY_WIDTH * DISPLAY_HEIGHT));
  } else {
    Serial.printf("WARNING: could not allocate %u-byte frame buffer; "
                  "drawing directly (%u bytes of heap remain).\n",
                  static_cast<unsigned>(DISPLAY_WIDTH * DISPLAY_HEIGHT),
                  static_cast<unsigned>(ESP.getFreeHeap()));
  }
#else
  Serial.println("Display: low-memory direct rendering.");
#endif
}

void displayLoadSettings() {
  themeColorIndex = min<uint8_t>(preferences.getUChar("theme", 1),
                                 THEME_COLOR_COUNT - 1);
  backgroundColor = THEME_COLORS[themeColorIndex];
  buttonColorIndex = min<uint8_t>(preferences.getUChar("buttonColor", 0),
                                  BUTTON_COLOR_COUNT - 1);
}

void setThemeColor(uint8_t index) {
  if (index >= THEME_COLOR_COUNT || index == themeColorIndex) {
    return;
  }
  themeColorIndex = index;
  backgroundColor = THEME_COLORS[index];
  settingSaved(preferences.putUChar("theme", themeColorIndex),
               "background color");
  markUiDirty();
}

void setButtonColor(uint8_t index) {
  if (index >= BUTTON_COLOR_COUNT || index == buttonColorIndex) {
    return;
  }
  buttonColorIndex = index;
  settingSaved(preferences.putUChar("buttonColor", buttonColorIndex),
               "button color");
  markUiDirty();
}

void displayBeginFrame() {
  renderTarget = framebufferReady
                     ? static_cast<TFT_eSPI *>(&screenFrameBuffer)
                     : &display;
  renderTarget->fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, backgroundColor);
}

void displayEndFrame() {
  if (framebufferReady) {
    screenFrameBuffer.pushSprite(0, 0);
  }
  renderTarget = &display;
}

bool pointInRect(int16_t x, int16_t y, int16_t left, int16_t top,
                 int16_t width, int16_t height) {
  return x >= left && x < left + width && y >= top && y < top + height;
}

bool pointOnMenuBackButton(int16_t x, int16_t y) {
  return pointInRect(x, y, UI_LEFT + UI_WIDTH - 56, 0, 56, 40);
}

uint16_t primaryTextColor() {
  return TFT_WHITE;
}

String fittedText(const String &source, int16_t maxWidth, uint8_t font) {
  TFT_eSPI &gfx = *renderTarget;
  String result = source;
  if (gfx.textWidth(result, font) <= maxWidth) {
    return result;
  }
  while (result.length() > 3 &&
         gfx.textWidth(result + "...", font) > maxWidth) {
    result.remove(result.length() - 1);
  }
  return result + "...";
}

void drawText(const String &text, int16_t x, int16_t y, uint8_t font,
              uint16_t colour) {
  TFT_eSPI &gfx = *renderTarget;
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(colour == TFT_WHITE ? primaryTextColor() : colour,
                   backgroundColor);
  gfx.drawString(text, x, y, font);
}

void drawButton(int16_t x, int16_t y, int16_t width, int16_t height,
                const String &label, uint16_t fill, uint16_t colour,
                uint8_t font, bool outline) {
  TFT_eSPI &gfx = *renderTarget;
  if (fill == PANEL_LIGHT) {
    fill = BUTTON_COLORS[buttonColorIndex];
  }
  gfx.fillRoundRect(x, y, width, height, 7, fill);
  if (outline) {
    gfx.drawRoundRect(x, y, width, height, 7, 0x4208);
  }
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(colour == TFT_WHITE ? primaryTextColor() : colour, fill);
  gfx.drawString(label, x + width / 2, y + height / 2, font);
}

void drawPanelHeader(const String &title) {
  TFT_eSPI &gfx = *renderTarget;
  gfx.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, backgroundColor);
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(TFT_CYAN, backgroundColor);
  gfx.drawString(title, UI_LEFT, 9, 2);
  gfx.drawFastHLine(UI_LEFT, 31, UI_WIDTH, 0x4208);
}

void drawMenuBackButton() {
  drawButton(UI_LEFT + UI_WIDTH - 56, 3, 56, 32, "BACK", PANEL_LIGHT,
             TFT_WHITE, 1);
}

bool animationCanvasCreate(int16_t x, int16_t y, int16_t width, int16_t height,
                           uint16_t frameIntervalMs) {
  animationCanvasDestroy();
  animationSprite.setColorDepth(8);
  animationReady = animationSprite.createSprite(width, height);
  if (!animationReady) {
    Serial.printf("WARNING: could not allocate %u-byte animation canvas.\n",
                  static_cast<unsigned>(width * height));
    return false;
  }
  animationX = x;
  animationY = y;
  animationIntervalMs = frameIntervalMs;
  lastAnimationAt = 0;
  Serial.printf("Animation canvas: %ux%u (%u bytes).\n",
                static_cast<unsigned>(width), static_cast<unsigned>(height),
                static_cast<unsigned>(width * height));
  return true;
}

void animationCanvasDestroy() {
  if (animationReady) {
    animationSprite.deleteSprite();
    animationReady = false;
  }
}

bool animationCanvasReady() {
  return animationReady;
}

bool animationCanvasDue(uint32_t now) {
  return animationReady && now - lastAnimationAt >= animationIntervalMs;
}

TFT_eSprite &animationCanvas() {
  return animationSprite;
}

void animationCanvasPush(uint32_t now) {
  lastAnimationAt = now;
  animationSprite.pushSprite(animationX, animationY);
}
