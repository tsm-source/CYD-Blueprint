#include "settings.h"

#include "ui.h"

Preferences preferences;

bool settingSaved(size_t written, const char *what) {
  if (written != 0) {
    return true;
  }
  Serial.printf("ERROR: could not save %s.\n", what);
  setStatus(String("Could not save ") + what);
  return false;
}
