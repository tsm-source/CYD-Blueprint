#pragma once

#include <Arduino.h>

void networkBegin();
void networkLoop();
// Saves the credentials and starts connecting.
void wifiConnect(const String &ssid, const String &password);
bool wifiConnected();
bool wifiConnecting();
const String &wifiSsid();
const String &wifiPassword();
void wifiSetCredentials(const String &ssid, const String &password);
// One-line description for status displays.
String wifiStatusText();
