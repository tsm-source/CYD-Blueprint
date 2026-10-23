#include "audio_player.h"

#include <Audio.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "audio_compressor.h"
#include "audio_dac.h"
#include "config.h"
#include "settings.h"
#include "ui.h"

namespace {

// DAC channel 2 is GPIO 26, the CYD speaker output.
Audio audioEngine(true, I2S_DAC_CHANNEL_LEFT_EN);

uint8_t volume = 10;
bool boost = false;
bool compressor = false;
bool noiseShaping = false;
bool amp = false;
bool playing = false;
String streamTitle;

void applyTone() {
  const int8_t gain = boost ? AUDIO_BOOST_DB : 0;
  audioEngine.setTone(gain, gain, gain);
}

void applyAmpPin() {
  digitalWrite(AUDIO_AMP_ENABLE_PIN, amp ? LOW : HIGH);
  pinMode(AUDIO_AMP_ENABLE_PIN, OUTPUT);
}

}  // namespace

void audioBegin() {
  volume = min<uint8_t>(preferences.getUChar("volume", 10), MAX_VOLUME);
  boost = preferences.getBool("boost", false);
  amp = preferences.getBool("amp4", false);
  compressor = preferences.getBool("compressor", false);
  noiseShaping = preferences.getBool("noiseshape", false);
  setAudioCompressorEnabled(compressor);
  setAudioNoiseShapingEnabled(noiseShaping);
  applyAmpPin();

  audioEngine.setBufsize(AUDIO_INPUT_BUFFER_BYTES, 0);
  audioEngine.forceMono(true);
  applyTone();
  audioEngine.setVolume(volume);
}

void audioLoop() {
  audioEngine.loop();
}

bool audioPlayUrl(const char *url) {
  if (WiFi.status() != WL_CONNECTED) {
    setStatus("Wi-Fi not connected");
    return false;
  }
  streamTitle = "";
  playing = false;
  audioEngine.stopSong();
  audioEngine.setVolume(volume);
  startAudioFadeIn();
  Serial.printf("Memory before stream: internal free=%u bytes, largest "
                "block=%u bytes, PSRAM=%u bytes.\n",
                static_cast<unsigned>(heap_caps_get_free_size(
                    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_largest_free_block(
                    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                static_cast<unsigned>(ESP.getPsramSize()));
  if (!audioEngine.connecttohost(url)) {
    setStatus("Could not start stream");
    return false;
  }
  playing = true;
  setStatus("Connecting to stream...");
  return true;
}

void audioStop() {
  audioEngine.stopSong();
  playing = false;
  markUiDirty();
}

bool audioIsPlaying() {
  return playing;
}

const String &audioStreamTitle() {
  return streamTitle;
}

uint8_t audioVolume() {
  return volume;
}

void audioSetVolume(uint8_t newVolume) {
  volume = min<uint8_t>(newVolume, MAX_VOLUME);
  audioEngine.setVolume(volume);
  if (settingSaved(preferences.putUChar("volume", volume), "volume")) {
    markUiDirty();
  }
}

bool audioBoostEnabled() {
  return boost;
}

void audioSetBoost(bool enabled) {
  boost = enabled;
  applyTone();
  if (settingSaved(preferences.putBool("boost", boost), "audio boost")) {
    setStatus(boost ? "Audio boost enabled" : "Audio boost disabled");
  }
}

bool audioCompressorEnabled() {
  return compressor;
}

void audioSetCompressor(bool enabled) {
  compressor = enabled;
  setAudioCompressorEnabled(enabled);
  if (settingSaved(preferences.putBool("compressor", compressor),
                   "compression setting")) {
    setStatus(compressor ? "Audio compression enabled"
                         : "Audio compression disabled");
  }
}

bool audioNoiseShapingEnabled() {
  return noiseShaping;
}

void audioSetNoiseShaping(bool enabled) {
  noiseShaping = enabled;
  setAudioNoiseShapingEnabled(enabled);
  if (settingSaved(preferences.putBool("noiseshape", noiseShaping),
                   "noise shaping setting")) {
    setStatus(noiseShaping ? "Noise shaping enabled"
                           : "Noise shaping disabled");
  }
}

bool audioAmpEnabled() {
  return amp;
}

void audioSetAmp(bool enabled) {
  amp = enabled;
  applyAmpPin();
  if (settingSaved(preferences.putBool("amp4", amp), "GPIO 4 setting")) {
    setStatus(amp ? "GPIO 4 amplifier enabled" : "GPIO 4 amplifier disabled");
  }
}

// Callbacks required by the audio library (global scope).
void audio_info(const char *info) {
  Serial.printf("audio: %s\n", info);
}

void audio_showstreamtitle(const char *info) {
  streamTitle = info;
  Serial.printf("stream title: %s\n", info);
}

void audio_eof_stream(const char *info) {
  playing = false;
  setStatus("Stream ended");
  Serial.printf("audio: stream ended (%s)\n", info);
}

// Chain DSP stages in the decoder's pre-I2S callback; continueI2S is retained
// for the audio library's callback contract.
void audio_process_i2s(uint32_t *sample, bool *continueI2S) {
  *continueI2S = true;
  processAudioCompressor(sample);
  processAudioDac(sample);
}
