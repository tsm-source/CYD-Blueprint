#include "app.h"

// Starting point for the real application. Useful building blocks:
//   display.h       drawButton(), drawText(), animationCanvasCreate()
//   audio_player.h  audioPlayUrl(), audioStop(), audioSetVolume()
//   network.h       wifiConnected(), wifiStatusText()
//   ui.h            markUiDirty() to repaint the home screen
// Animation example (call in appSetup(), then draw in appDrawAnimation()):
//   animationCanvasCreate(100, 60, 120, 120, 40);

void appSetup() {}

void appLoop(uint32_t now) {
  (void)now;
}

void appDrawHome(TFT_eSPI &gfx) {
  (void)gfx;
}

void appDrawAnimation(TFT_eSprite &canvas, uint32_t now) {
  (void)canvas;
  (void)now;
}

bool appHandleTouch(int16_t x, int16_t y) {
  (void)x;
  (void)y;
  return false;
}
