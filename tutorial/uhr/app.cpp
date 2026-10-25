#include "app.h"

#include <time.h>

#include "colors.h"
#include "config.h"
#include "display.h"
#include "network.h"

// Tutorial "WLAN-Uhr": zeigt Uhrzeit und Datum, die per WLAN (NTP) gestellt
// werden. Anleitung: docs/tutorial-uhr.md

namespace {

constexpr int16_t CLOCK_X = 20;
constexpr int16_t CLOCK_Y = 70;
constexpr int16_t CLOCK_WIDTH = 280;
constexpr int16_t CLOCK_HEIGHT = 80;
// Einmal pro Sekunde reicht fuer eine Uhr.
constexpr uint16_t CLOCK_INTERVAL_MS = 1000;
// 8 Bit Farbtiefe: nur 22 KB statt 45 KB. 16 Bit passt im Profil mit
// Bildschirm-Framebuffer (esp32dev-fast) nicht mehr in den Speicher.
constexpr uint8_t CLOCK_COLOR_DEPTH = 8;
// Alles vor 2021 bedeutet: die Uhr wurde noch nicht per NTP gestellt.
constexpr time_t MIN_VALID_TIME = 1609459200;

const char *const WEEKDAYS[] = {"Sonntag",    "Montag",  "Dienstag", "Mittwoch",
                                "Donnerstag", "Freitag", "Samstag"};

}  // namespace

void appSetup() {
  animationCanvasCreate(CLOCK_X, CLOCK_Y, CLOCK_WIDTH, CLOCK_HEIGHT,
                        CLOCK_INTERVAL_MS, CLOCK_COLOR_DEPTH);
}

void appLoop(uint32_t now) {
  (void)now;
}

// Statischer Teil: Titel und WLAN-Status.
void appDrawHome(TFT_eSPI &gfx) {
  gfx.setTextDatum(TL_DATUM);
  gfx.setTextColor(TFT_CYAN, backgroundColor);
  gfx.drawString("UHR", 12, 9, 2);
  gfx.drawFastHLine(12, 31, DISPLAY_WIDTH - 24, 0x4208);
  gfx.setTextColor(TEXT_MUTED, backgroundColor);
  gfx.drawString(fittedText(wifiStatusText(), 180, 2), 12, 212, 2);
}

// Wird jede Sekunde aufgerufen und malt die Uhr in den Canvas.
void appDrawAnimation(TFT_eSprite &canvas, uint32_t now) {
  (void)now;
  // Schwarzer Kasten: sehr dunkle Themefarben gaebe bei 8 Bit Artefakte.
  canvas.fillSprite(TFT_BLACK);
  canvas.drawRect(0, 0, CLOCK_WIDTH, CLOCK_HEIGHT, 0x4208);
  canvas.setTextDatum(MC_DATUM);

  const time_t raw = time(nullptr);
  if (raw < MIN_VALID_TIME) {
    canvas.setTextColor(TEXT_MUTED, TFT_BLACK);
    canvas.drawString("Warte auf WLAN...", CLOCK_WIDTH / 2, CLOCK_HEIGHT / 2,
                      2);
    return;
  }

  struct tm t;
  localtime_r(&raw, &t);
  char text[16];

  strftime(text, sizeof(text), "%H:%M:%S", &t);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextSize(3);
  canvas.drawString(text, CLOCK_WIDTH / 2, 28, 2);
  canvas.setTextSize(1);

  strftime(text, sizeof(text), "%d.%m.%Y", &t);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString(String(WEEKDAYS[t.tm_wday]) + ", " + text,
                    CLOCK_WIDTH / 2, 66, 2);
}

bool appHandleTouch(int16_t x, int16_t y) {
  (void)x;
  (void)y;
  return false;
}
