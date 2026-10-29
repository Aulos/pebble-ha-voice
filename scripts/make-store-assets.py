#!/usr/bin/env python3
"""Builds the appstore listing's images from the icon and the screenshots.

    .venv/bin/python scripts/make-store-assets.py

Writes into docs/store/:
  icon-144.png            the 144 pixel icon the appstore asks for (also made by
                          make-icons.py, which this script leaves alone)
  banner-720x320.png      the banner at the top of the app's page
  screenshots/*.png       the watch screens, one set for the 200x228 platform

Needs cairosvg for the icon:  .venv/bin/pip install cairosvg
"""
import io
import os
import re

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
SVG = os.path.join(ROOT, "resources", "icon.svg")
SCREENS = os.path.join(ROOT, "docs", "screens")
OUT = os.path.join(ROOT, "docs", "store")

BANNER = (720, 320)
CYAN = (0, 170, 255)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FONT_REGULAR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def mark_without_background(size):
    """The house and microphone alone, on a transparent background."""
    import cairosvg

    svg = open(SVG, encoding="utf-8").read()
    # The black square exists for the watch's app menu, where it keeps the icon
    # visible on a cyan selected row. On the banner the black is already there.
    svg = re.sub(r'\s*<rect width="25" height="25" fill="#000000"/>', "", svg)
    png = cairosvg.svg2png(bytestring=svg.encode(), output_width=size, output_height=size)
    return Image.open(io.BytesIO(png)).convert("RGBA")


def make_banner():
    banner = Image.new("RGB", BANNER, BLACK)
    d = ImageDraw.Draw(banner)

    # A thin cyan rule down the left edge ties the banner to the app's screens.
    d.rectangle([0, 0, 5, BANNER[1]], fill=CYAN)

    mark = mark_without_background(180)
    banner.paste(mark, (56, (BANNER[1] - 180) // 2), mark)

    title_font = ImageFont.truetype(FONT_BOLD, 74)
    sub_font = ImageFont.truetype(FONT_REGULAR, 28)

    x = 270
    d.text((x, 96), "HA Voice", font=title_font, fill=WHITE)
    d.text((x + 4, 190), "Home Assistant, by voice", font=sub_font, fill=CYAN)
    return banner


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(os.path.join(OUT, "screenshots"), exist_ok=True)

    banner_path = os.path.join(OUT, "banner-720x320.png")
    make_banner().save(banner_path)
    print("wrote", os.path.normpath(banner_path), f"({BANNER[0]}x{BANNER[1]})")

    # The appstore wants the screens at the watch's own resolution, one set for
    # each platform it runs on.
    for name in ("main", "answer", "menu", "test"):
        source = os.path.join(SCREENS, name + ".png")
        if not os.path.exists(source):
            print("skipped", name, "(no screenshot)")
            continue
        target = os.path.join(OUT, "screenshots", f"emery-{name}.png")
        Image.open(source).convert("RGB").save(target)
        print("wrote", os.path.normpath(target))


if __name__ == "__main__":
    main()
