#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

// Hooks for the actual application. The blueprint ships them empty, so the
// start screen shows nothing but the OPTIONS button.

// Called once after display, touch, Wi-Fi and audio are initialized.
void appSetup();

// Called on every pass of loop(); keep it short and never block.
void appLoop(uint32_t now);

// Draws the static home screen content. The OPTIONS button is added on top.
void appDrawHome(TFT_eSPI &gfx);

// Draws one animation frame into the canvas created with
// animationCanvasCreate() (see display.h). Called at the canvas frame interval
// while the home screen is visible.
void appDrawAnimation(TFT_eSprite &canvas, uint32_t now);

// Home screen touch (outside the OPTIONS button). The screen is not repainted
// automatically; call markUiDirty() if appDrawHome() content changed.
// Return true if the touch was used.
bool appHandleTouch(int16_t x, int16_t y);
