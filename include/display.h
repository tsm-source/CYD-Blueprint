#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "colors.h"

extern TFT_eSPI display;
// Drawing target for the current frame: the full-screen sprite when it fits
// in RAM, the display itself in the low-memory profile.
extern TFT_eSPI *renderTarget;
extern uint16_t backgroundColor;
extern uint8_t themeColorIndex;
extern uint8_t buttonColorIndex;

constexpr uint8_t THEME_COLOR_COUNT = 10;
constexpr uint8_t BUTTON_COLOR_COUNT = 10;
constexpr uint8_t COLORS_PER_ROW = 5;
extern const uint16_t THEME_COLORS[THEME_COLOR_COUNT];
extern const uint16_t BUTTON_COLORS[BUTTON_COLOR_COUNT];

void displayBegin();
void displayLoadSettings();
void setThemeColor(uint8_t index);
void setButtonColor(uint8_t index);

void displayBeginFrame();
void displayEndFrame();

bool pointInRect(int16_t x, int16_t y, int16_t left, int16_t top,
                 int16_t width, int16_t height);
uint16_t primaryTextColor();
String fittedText(const String &source, int16_t maxWidth, uint8_t font = 2);
void drawText(const String &text, int16_t x, int16_t y, uint8_t font = 2,
              uint16_t colour = TFT_WHITE);
void drawButton(int16_t x, int16_t y, int16_t width, int16_t height,
                const String &label, uint16_t fill = PANEL_LIGHT,
                uint16_t colour = TFT_WHITE, uint8_t font = 2,
                bool outline = true);
void drawPanelHeader(const String &title);
void drawMenuBackButton();
bool pointOnMenuBackButton(int16_t x, int16_t y);

// Optional animation region. It is a small 8-bit sprite that is composed off
// screen and pushed in one piece, so animations do not flicker even in the
// low-memory profile. Nothing is allocated until an app asks for it.
// colorDepth 16 reproduces every color exactly (use it when the canvas shares
// the themed background); 8 halves the memory but turns dark colors into
// black, so only use it with bright colors on a black background.
bool animationCanvasCreate(int16_t x, int16_t y, int16_t width, int16_t height,
                           uint16_t frameIntervalMs, uint8_t colorDepth = 16);
void animationCanvasDestroy();
bool animationCanvasReady();
bool animationCanvasDue(uint32_t now);
TFT_eSprite &animationCanvas();
void animationCanvasPush(uint32_t now);
