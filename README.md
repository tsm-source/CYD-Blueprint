# CYD-Blueprint

Startprojekt für das ESP32-basierte CYD (Cheap Yellow Display) mit Touch, WLAN,
Ton und Animationen. Basis ist [CYD-PET-Radio](https://github.com/tsm-source/CYD-PET-Radio):
Grundfunktionen, Optionsmenü und Speichertricks wurden übernommen, die
Radio-/Pet-Funktionen entfernt.

Repository: <https://github.com/tsm-source/CYD-Blueprint>

Der Startscreen ist leer und zeigt nur den Button **OPTIONS**.

## Unterstützte Boards

| Board | PlatformIO-Environment |
| --- | --- |
| CYD 2.8" ST7789 | `esp32dev` (40 MHz), `esp32dev-fast` (80 MHz), `esp32dev-fast-low-memory` |
| CYD 2.8" ILI9341 | `esp32dev-ili9341` (40 MHz), `esp32dev-ili9341-low-memory` |
| Freenove FNK0114L 3.2" | `fnk0114l` (80 MHz), `fnk0114l-low-memory` (zusätzlich RGB-LED-Menü) |

```
pio run -e esp32dev-fast-low-memory -t upload
```

## Optionsmenü

- **Audio**: Boost +6 dB, dynamische Kompression, GPIO4-Verstärker, Noise
  Shaping, Test-Stream (Ein/Aus), Lautstärke
- **Display & Touch**: Hintergrund- und Buttonfarbe, Touch neu kalibrieren
  (Erststart: automatische 5-Punkt-Kalibrierung)
- **Wi-Fi**: SSID/Passwort per Bildschirmtastatur, Verbinden (offene Netze mit
  leerem Passwort möglich)
- **RGB LED** (nur FNK0114L)

Alle Einstellungen liegen im NVS (Namespace `blueprint`).

## Aufbau

| Datei | Zweck |
| --- | --- |
| `src/app.cpp`, `include/app.h` | **Hier entsteht die eigene Anwendung** (Hooks für Setup, Loop, Home-Zeichnung, Animation, Touch) |
| `src/ui.cpp` | Screens, Menüs, Tastatur, Touch-Verteilung |
| `src/display.cpp` | TFT, Framebuffer, Zeichenhelfer, Animations-Canvas |
| `src/touch.cpp` | Touch (XPT2046 bzw. FNK0114L) und Kalibrierung |
| `src/network.cpp` | WLAN, Reconnect, NTP/Zeitzone |
| `src/audio_player.cpp` | Streaming-Wiedergabe, Audio-Einstellungen, DSP-Hooks |
| `src/audio_compressor.cpp`, `src/audio_dac.cpp` | Kompressor, Fade-in und Noise Shaping für den 8-Bit-DAC |
| `include/config.h` | Konstanten (Pins, Puffer, Layout, Test-Stream) |

Eigene Anwendung: `appSetup()` für Initialisierung, `appDrawHome()` für
statische Inhalte, `appHandleTouch()` für Berührungen, `appLoop()` für Logik.
Töne: `audioPlayUrl()` / `audioStop()` / `audioSetVolume()` (`audio_player.h`).

## Animationen

`animationCanvasCreate(x, y, w, h, intervallMs, colorDepth = 16)` legt einen kleinen
Sprite an (z. B. in `appSetup()`). `appDrawAnimation()` zeichnet jedes Bild dort
hinein, das Framework schiebt es flackerfrei auf das Display. Ohne Aufruf wird kein
Speicher belegt.

Mit `colorDepth = 8` braucht der Canvas nur die Hälfte des Speichers, aber sehr
dunkle Farben werden schwarz (RGB332). Das passt bei schwarzem Hintergrund mit
hellen Farben; sonst die 16 Bit belassen.

## Tutorials

Mit Lösungen in `tutorial/` (`src/app.cpp` bleibt leer):

- [WLAN-Uhr](docs/tutorial-uhr.md)
- [Fang den Punkt (Touch-Spiel)](docs/tutorial-spiel.md)

Die Bilder darin sind simulierte Nachzeichnungen (`scripts/make_tutorial_images.py`).

## Web-Flasher

Die Firmware lässt sich direkt im Browser installieren:
<https://tsm-source.github.io/CYD-Blueprint/>. Jeder Push auf `main` baut die drei
`*-low-memory`-Varianten und veröffentlicht den Flasher über GitHub Pages
(einmalig unter *Settings → Pages → Source: GitHub Actions* aktivieren).
Die Seite liegt in `web-flasher/`, der Ablauf in `.github/workflows/web-flasher.yml`.

## Speichertricks (aus dem PET-Radio)

- **8-Bit-Sprites** statt RGB565 (Framebuffer 320×240 = 75 KB).
- **`*-low-memory`-Profile** (`CYD_LOW_MEMORY_DIRECT_RENDER`): kein Vollbild-Framebuffer,
  direkt aufs Display zeichnen; mehr freier Heap für HTTPS/TLS. Schlägt die Allokation
  fehl, wird automatisch direkt gezeichnet.
- **Audio-Eingangspuffer 20000 Byte** ohne PSRAM (Standard ~6 KB läuft bei
  Display-Updates leer), Mono-Ausgabe.
- Große Daten (z. B. URLs) im **NVS** halten und bei Bedarf in einen Stackpuffer lesen.
- Der Animations-Canvas wird nur bei Bedarf und in der benötigten Größe angelegt.
- Heap-Log vor jedem Stream-Start (`Memory before stream ...`).
- Pinned Bibliotheken (`ESP32-audioI2S` 2.0.6, Plattform 7.1.3).
