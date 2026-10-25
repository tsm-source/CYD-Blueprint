#include "app.h"

#include "colors.h"
#include "config.h"
#include "display.h"
#include "settings.h"
#include "ui.h"

// Tutorial "Fang den Punkt": ein Punkt springt umher, antippen gibt Punkte,
// verpasst man ihn, endet die Runde. Anleitung: docs/tutorial-spiel.md

namespace {

// 240x120 = 28 KB: groesser findet im Profil mit Framebuffer (esp32dev-fast)
// keinen zusammenhaengenden Speicher mehr.
constexpr int16_t FIELD_X = 40;
constexpr int16_t FIELD_Y = 44;
constexpr int16_t FIELD_WIDTH = 240;
constexpr int16_t FIELD_HEIGHT = 120;
constexpr int16_t TARGET_RADIUS = 20;
// Etwas grosszuegiger als der sichtbare Kreis: Finger sind dicker als Pixel.
constexpr int16_t TOUCH_TOLERANCE = 8;
constexpr uint16_t FRAME_INTERVAL_MS = 33;
constexpr uint32_t START_LIFETIME_MS = 1500;
constexpr uint32_t MIN_LIFETIME_MS = 500;
constexpr uint32_t LIFETIME_STEP_MS = 50;

uint16_t score = 0;
uint16_t highScore = 0;
int16_t targetX = 0;
int16_t targetY = 0;
uint32_t targetMovedAt = 0;

// Je mehr Treffer, desto kuerzer bleibt der Punkt stehen.
uint32_t lifetimeMs() {
  const uint32_t reduction = score * LIFETIME_STEP_MS;
  return reduction >= START_LIFETIME_MS - MIN_LIFETIME_MS
             ? MIN_LIFETIME_MS
             : START_LIFETIME_MS - reduction;
}

void moveTarget(uint32_t now) {
  targetX = random(TARGET_RADIUS, FIELD_WIDTH - TARGET_RADIUS);
  // Oben bleibt eine Zeile fuer die Punktanzeige frei.
  targetY = random(TARGET_RADIUS + 18, FIELD_HEIGHT - TARGET_RADIUS);
  targetMovedAt = now;
}

void endRound() {
  if (score > highScore) {
    highScore = score;
    settingSaved(preferences.putUInt("hiscore", highScore), "high score");
  }
  score = 0;
}

}  // namespace

void appSetup() {
  // Schwarzes Spielfeld: 8 Bit Farbtiefe reicht und spart Speicher.
  animationCanvasCreate(FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT,
                        FRAME_INTERVAL_MS, 8);
  highScore = preferences.getUInt("hiscore", 0);
  moveTarget(millis());
}

void appLoop(uint32_t now) {
  // Waehrend das Optionsmenue offen ist, laeuft das Spiel nicht weiter.
  if (currentScreen() != Screen::Home) {
    targetMovedAt = now;
    return;
  }
  if (now - targetMovedAt >= lifetimeMs()) {
    endRound();
    moveTarget(now);
  }
}

void appDrawHome(TFT_eSPI &gfx) {
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(TFT_CYAN, backgroundColor);
  gfx.drawString("FANG DEN PUNKT", 12, 9, 2);
  gfx.drawFastHLine(12, 31, DISPLAY_WIDTH - 24, 0x4208);
  gfx.setTextColor(TEXT_MUTED, backgroundColor);
  gfx.drawString("Tippe auf den Punkt!", 12, 212, 2);
}

void appDrawAnimation(TFT_eSprite &canvas, uint32_t now) {
  canvas.fillSprite(TFT_BLACK);
  canvas.drawRect(0, 0, FIELD_WIDTH, FIELD_HEIGHT, 0x4208);

  // Aussenring = Ziel, Fuellung schrumpft mit der verbleibenden Zeit.
  const uint32_t age = min<uint32_t>(now - targetMovedAt, lifetimeMs());
  const int16_t inner = TARGET_RADIUS * (lifetimeMs() - age) / lifetimeMs();
  canvas.drawCircle(targetX, targetY, TARGET_RADIUS, TFT_WHITE);
  canvas.fillCircle(targetX, targetY, inner, TFT_ORANGE);

  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(String("Treffer: ") + score, 6, 4, 2);
  canvas.setTextDatum(TR_DATUM);
  canvas.drawString(String("Rekord: ") + highScore, FIELD_WIDTH - 6, 4, 2);
}

bool appHandleTouch(int16_t x, int16_t y) {
  const int16_t dx = x - (FIELD_X + targetX);
  const int16_t dy = y - (FIELD_Y + targetY);
  const int16_t reach = TARGET_RADIUS + TOUCH_TOLERANCE;
  if (dx * dx + dy * dy > reach * reach) {
    return false;
  }
  ++score;
  moveTarget(millis());
  return true;
}
