"""Erzeugt simulierte Display-Bilder (320x240, 2x skaliert) fuer die Tutorials.

Das sind Nachzeichnungen, keine Fotos vom Geraet.
Aufruf: python scripts/make_tutorial_images.py
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent.parent / "docs" / "img"
OUT.mkdir(parents=True, exist_ok=True)
S = 2
FONT = "C:/Windows/Fonts/arial.ttf"
BG = (0, 0, 0)
MUTED = (150, 150, 150)
CYAN = (0, 255, 255)
LINE = (66, 66, 66)
WHITE = (255, 255, 255)
RED = (255, 70, 70)
GREEN = (80, 255, 80)


def font(px):
    return ImageFont.truetype(FONT, px * S)


def new():
    img = Image.new("RGB", (320 * S, 240 * S), BG)
    return img, ImageDraw.Draw(img)


def text(d, xy, s, px, fill, anchor="la"):
    d.text((xy[0] * S, xy[1] * S), s, font=font(px), fill=fill, anchor=anchor)


def rect(d, x, y, w, h, outline=None, fill=None, r=0):
    box = (x * S, y * S, (x + w) * S, (y + h) * S)
    if r:
        d.rounded_rectangle(box, r * S, outline=outline, fill=fill, width=S)
    else:
        d.rectangle(box, outline=outline, fill=fill, width=S)


def options_button(d):
    rect(d, 202, 200, 110, 34, outline=CYAN, fill=(0, 40, 50), r=6)
    text(d, (257, 217), "OPTIONS", 15, CYAN, "mm")


def header(d, title):
    text(d, (12, 9), title, 15, CYAN)
    d.line((12 * S, 31 * S, 308 * S, 31 * S), fill=LINE, width=S)


def target(d, cx, cy):
    d.ellipse(((cx - 20) * S, (cy - 20) * S, (cx + 20) * S, (cy + 20) * S),
              outline=WHITE, width=S)
    d.ellipse(((cx - 12) * S, (cy - 12) * S, (cx + 12) * S, (cy + 12) * S),
              fill=(255, 165, 0))


def save(img, name):
    img.save(OUT / name)


def clock_content(d):
    header(d, "UHR")
    text(d, (160, 98), "14:32:07", 54, WHITE, "mm")
    text(d, (160, 136), "Donnerstag, 14.05.2026", 15, CYAN, "mm")
    text(d, (12, 212), "WiFi: verbunden", 12, MUTED)
    options_button(d)


img, d = new()
options_button(d)
save(img, "start-leer.png")

img, d = new()
header(d, "OPTIONS")
for i, name in enumerate(["WI-FI", "DISPLAY", "SOUND", "SYSTEM"]):
    y = 42 + i * 40
    rect(d, 12, y, 296, 34, outline=LINE, fill=(20, 20, 20), r=6)
    text(d, (24, y + 17), name, 15, WHITE, "lm")
save(img, "optionen.png")

img, d = new()
clock_content(d)
save(img, "uhr-ergebnis.png")

img, d = new()
header(d, "UHR")
text(d, (160, 110), "Warte auf WLAN...", 15, MUTED, "mm")
text(d, (12, 212), "WiFi: verbinde...", 12, MUTED)
options_button(d)
save(img, "uhr-warten.png")

img, d = new()
clock_content(d)
rect(d, 20, 70, 280, 80, outline=RED)
text(d, (24, 152), "Canvas 280x80 bei (20,70)", 12, RED)
rect(d, 12, 9, 296, 22, outline=GREEN)
text(d, (300, 36), "appDrawHome", 12, GREEN, "ra")
save(img, "uhr-layout.png")

cx, cy = 40 + 150, 44 + 70
img, d = new()
header(d, "FANG DEN PUNKT")
rect(d, 40, 44, 240, 120, outline=LINE)
text(d, (46, 48), "Treffer: 7", 14, WHITE)
text(d, (274, 48), "Rekord: 12", 14, WHITE, "ra")
target(d, cx, cy)
text(d, (12, 212), "Tippe auf den Punkt!", 12, MUTED)
options_button(d)
save(img, "spiel-ergebnis.png")

img, d = new()
header(d, "FANG DEN PUNKT")
rect(d, 40, 44, 240, 120, outline=RED)
target(d, cx, cy)
options_button(d)
text(d, (44, 166), "Canvas 240x120 bei (40,44)", 12, RED)
rect(d, cx - 28, cy - 28, 56, 56, outline=GREEN)
text(d, (cx + 30, cy - 30), "Trefferzone R+8", 11, GREEN)
save(img, "spiel-layout.png")
print("fertig:", OUT)
