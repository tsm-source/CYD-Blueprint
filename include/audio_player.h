#pragma once

#include <Arduino.h>

void audioBegin();
// Call as often as possible; the decoder needs frequent servicing.
void audioLoop();

// Streams an HTTP(S) MP3/AAC URL (requires Wi-Fi).
bool audioPlayUrl(const char *url);
void audioStop();
bool audioIsPlaying();
const String &audioStreamTitle();

uint8_t audioVolume();
void audioSetVolume(uint8_t volume);
bool audioBoostEnabled();
void audioSetBoost(bool enabled);
bool audioCompressorEnabled();
void audioSetCompressor(bool enabled);
bool audioNoiseShapingEnabled();
void audioSetNoiseShaping(bool enabled);
bool audioAmpEnabled();
void audioSetAmp(bool enabled);
