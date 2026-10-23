#include "network.h"

#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "settings.h"
#include "ui.h"

namespace {

String ssid;
String password;
bool connecting = false;
uint32_t connectStartedAt = 0;
volatile uint8_t disconnectReason = 0;
volatile bool disconnectReasonPending = false;
uint32_t wifiLostAt = 0;

void startConnection() {
  if (ssid.isEmpty()) {
    connecting = false;
    setStatus("Enter an SSID first");
    return;
  }
  // Open networks have no password; otherwise WPA2 needs 8-63 characters.
  if (!password.isEmpty() && (password.length() < 8 || password.length() > 63)) {
    connecting = false;
    setStatus("WPA2 password must be 8-63 characters");
    return;
  }

  disconnectReason = 0;
  disconnectReasonPending = false;
  connecting = true;
  connectStartedAt = millis();
  setStatus("Connecting to Wi-Fi...");

  const wl_status_t result = WiFi.begin(
      ssid.c_str(), password.isEmpty() ? nullptr : password.c_str());
  if (result == WL_CONNECT_FAILED) {
    connecting = false;
    setStatus("Could not start Wi-Fi");
    Serial.println("ERROR: Wi-Fi rejected the connection request.");
  }
  Serial.printf("Wi-Fi connect requested (SSID length %u, password length "
                "%u, initial status %d).\n",
                static_cast<unsigned>(ssid.length()),
                static_cast<unsigned>(password.length()),
                static_cast<int>(result));
}

String failureMessage(wl_status_t status, uint8_t reason) {
  if (reason == WIFI_REASON_NO_AP_FOUND || status == WL_NO_SSID_AVAIL) {
    return "SSID not found (check 2.4 GHz network)";
  }
  if (reason == WIFI_REASON_AUTH_FAIL ||
      reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
      reason == WIFI_REASON_HANDSHAKE_TIMEOUT ||
      status == WL_CONNECT_FAILED) {
    return "Authentication failed: check WPA2 password";
  }
  if (reason == WIFI_REASON_ASSOC_FAIL) {
    return "Router rejected the connection";
  }
  if (status == WL_CONNECTION_LOST) {
    return "Wi-Fi connection lost";
  }
  return "Wi-Fi error " + String(reason) + " (status " +
         String(static_cast<int>(status)) + ")";
}

}  // namespace

void networkBegin() {
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");

  WiFi.mode(WIFI_STA);
  configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");
  // Modem sleep makes streams stutter.
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      disconnectReason = info.wifi_sta_disconnected.reason;
      disconnectReasonPending = true;
    }
  });
  if (!ssid.isEmpty()) {
    startConnection();
  } else {
    setStatus("Set up Wi-Fi");
  }
}

void networkLoop() {
  if (disconnectReasonPending) {
    const uint8_t reason = disconnectReason;
    disconnectReasonPending = false;
    Serial.printf("Wi-Fi disconnected: reason %u (%s), status %d.\n", reason,
                  WiFi.disconnectReasonName(
                      static_cast<wifi_err_reason_t>(reason)),
                  static_cast<int>(WiFi.status()));
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiLostAt = 0;
    if (connecting) {
      connecting = false;
      disconnectReason = 0;
      configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");
      setStatus("Wi-Fi connected");
    }
    return;
  }

  // Restart the connection when Wi-Fi stays down for 30 s.
  if (connecting || ssid.isEmpty()) {
    wifiLostAt = 0;
  } else if (wifiLostAt == 0) {
    wifiLostAt = millis();
  } else if (millis() - wifiLostAt >= WIFI_RECONNECT_DELAY_MS) {
    wifiLostAt = 0;
    Serial.println("Wi-Fi down for 30 s, reconnecting.");
    WiFi.disconnect();
    startConnection();
    return;
  }

  if (connecting && millis() - connectStartedAt > WIFI_TIMEOUT_MS) {
    connecting = false;
    const wl_status_t status = WiFi.status();
    const uint8_t reason = disconnectReason;
    setStatus(failureMessage(status, reason));
    Serial.printf("ERROR: Wi-Fi connection timed out (status %d, reason %u).\n",
                  static_cast<int>(status), reason);
  }
}

void wifiSetCredentials(const String &newSsid, const String &newPassword) {
  ssid = newSsid;
  password = newPassword;
}

void wifiConnect(const String &newSsid, const String &newPassword) {
  ssid = newSsid;
  password = newPassword;
  if (password.isEmpty()) {
    preferences.remove("password");
  }
  const bool savedSsid = preferences.putString("ssid", ssid) > 0;
  const bool savedPassword =
      password.isEmpty() || preferences.putString("password", password) > 0;
  if (!savedSsid || !savedPassword) {
    Serial.println("ERROR: could not save Wi-Fi credentials.");
    setStatus("Could not save Wi-Fi credentials");
    return;
  }
  WiFi.disconnect();
  startConnection();
}

bool wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool wifiConnecting() {
  return connecting;
}

const String &wifiSsid() {
  return ssid;
}

const String &wifiPassword() {
  return password;
}

String wifiStatusText() {
  if (connecting) {
    return "Connecting to Wi-Fi...";
  }
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  if (ssid.isEmpty()) {
    return "Wi-Fi not configured";
  }
  return statusText();
}
