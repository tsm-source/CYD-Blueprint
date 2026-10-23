#pragma once

#include <Arduino.h>

constexpr int16_t DISPLAY_WIDTH = 320;
constexpr int16_t DISPLAY_HEIGHT = 240;

// Menu screens use the full width; the home screen belongs to the app.
constexpr int16_t UI_LEFT = 12;
constexpr int16_t UI_WIDTH = DISPLAY_WIDTH - 2 * UI_LEFT;

constexpr char PREFS_NAMESPACE[] = "blueprint";

// The only element on the start screen.
constexpr int16_t OPTIONS_BUTTON_WIDTH = 110;
constexpr int16_t OPTIONS_BUTTON_HEIGHT = 34;
constexpr int16_t OPTIONS_BUTTON_X = DISPLAY_WIDTH - OPTIONS_BUTTON_WIDTH - 8;
constexpr int16_t OPTIONS_BUTTON_Y = DISPLAY_HEIGHT - OPTIONS_BUTTON_HEIGHT - 6;

constexpr uint16_t TOUCH_DEBOUNCE_MS = 180;

constexpr uint32_t WIFI_TIMEOUT_MS = 25000;
constexpr uint32_t WIFI_RECONNECT_DELAY_MS = 30000;
constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";

// Without PSRAM the audio library defaults to ~6 KB, which underruns whenever
// a display refresh delays audioLoop().
constexpr int AUDIO_INPUT_BUFFER_BYTES = 20000;
constexpr int8_t AUDIO_BOOST_DB = 6;
// Active-low amplifier enable on compatible CYD boards.
constexpr uint8_t AUDIO_AMP_ENABLE_PIN = 4;
constexpr uint8_t MAX_VOLUME = 21;
constexpr char AUDIO_TEST_STREAM_URL[] =
    "http://stream.antenne.de/antenne";

#if defined(BOARD_FREENOVE_FNK0114L)
constexpr int16_t SETTINGS_ITEM_HEIGHT = 32;
constexpr int16_t SETTINGS_ITEM_GAP = 6;
constexpr uint8_t SETTINGS_ITEM_COUNT = 4;
#else
constexpr int16_t SETTINGS_ITEM_HEIGHT = 40;
constexpr int16_t SETTINGS_ITEM_GAP = 8;
constexpr uint8_t SETTINGS_ITEM_COUNT = 3;
#endif
constexpr int16_t SETTINGS_ITEM_START_Y = 39;

#if defined(BOARD_FREENOVE_FNK0114L)
#if defined(CYD_LOW_MEMORY_DIRECT_RENDER)
constexpr char BUILD_PROFILE_NAME[] = "fnk0114l-low-memory";
#else
constexpr char BUILD_PROFILE_NAME[] = "fnk0114l";
#endif
#elif defined(ILI9341_DRIVER)
#if defined(CYD_LOW_MEMORY_DIRECT_RENDER)
constexpr char BUILD_PROFILE_NAME[] = "esp32dev-ili9341-low-memory";
#else
constexpr char BUILD_PROFILE_NAME[] = "esp32dev-ili9341";
#endif
#elif defined(CYD_LOW_MEMORY_DIRECT_RENDER)
constexpr char BUILD_PROFILE_NAME[] = "esp32dev-fast-low-memory";
#elif defined(SPI_FREQUENCY) && SPI_FREQUENCY >= 80000000
constexpr char BUILD_PROFILE_NAME[] = "esp32dev-fast";
#else
constexpr char BUILD_PROFILE_NAME[] = "esp32dev";
#endif
