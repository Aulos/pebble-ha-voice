#!/usr/bin/env python3
"""Draws the app icons from the same microphone as the on-watch screen.

    scripts/make-icons.py

Everything is drawn at 8x and scaled down, so the small sizes come out with
clean edges without needing any antialiasing tricks. The output is
resources/images/menu_icon.png (the app menu) and app_icon.png (store listing).
"""
import os

from PIL import Image, ImageDraw

SCALE = 8
CYAN = (0, 170, 255, 255)
BLACK = (0, 0, 0, 255)


def draw_small(size=25):
    """The 25 pixel menu icon, drawn pixel by pixel.

    Supersampling is the wrong tool at this size: the strokes are one or two
    pixels wide, and scaling down turns them grey. Drawing on the pixel grid
    keeps the icon crisp on the watch's app menu.
    """
    image = Image.new("RGBA", (size, size), BLACK)
    d = ImageDraw.Draw(image)

    # Capsule, with three grill bars cut out of it.
    d.rounded_rectangle([8, 1, 16, 14], radius=4, fill=CYAN)
    for y in (4, 7, 10):
        d.rectangle([10, y, 14, y], fill=BLACK)

    # Stem and base: the base is narrower than the capsule, which is what makes
    # the shape read as a microphone rather than a lollipop.
    d.rectangle([11, 15, 13, 20], fill=CYAN)
    d.rectangle([9, 21, 15, 22], fill=CYAN)
    return image


def draw_icon(size):
    """One microphone, drawn to fill a square of `size` pixels."""
    n = size * SCALE
    image = Image.new("RGBA", (n, n), BLACK)
    d = ImageDraw.Draw(image)
    cx = n / 2

    # A 25 pixel icon has very few pixels to work with, so it gets thicker
    # strokes and a wider capsule; at that size the extra weight is what keeps
    # the shape readable.
    small = size <= 25
    stroke = 1.35 if small else 1.0

    # Capsule. Narrow enough that the cradle can sit outside it.
    capsule_w = n * (0.32 if small else 0.28)
    capsule_top = n * 0.06
    capsule_bottom = n * 0.50
    d.rounded_rectangle(
        [cx - capsule_w / 2, capsule_top, cx + capsule_w / 2, capsule_bottom],
        radius=capsule_w / 2,
        fill=CYAN,
    )

    # Grill. Two bars at the small size, three when there is room.
    bars = 2 if size <= 25 else 3
    bar_w = capsule_w * 0.52
    bar_h = n * (0.042 if small else 0.030)
    first = n * (0.17 if bars == 2 else 0.15)
    step = n * (0.13 if bars == 2 else 0.10)
    for i in range(bars):
        y = first + step * i
        d.rectangle([cx - bar_w / 2, y - bar_h / 2, cx + bar_w / 2, y + bar_h / 2], fill=BLACK)

    # Cradle: an arc that wraps the bottom of the capsule. At 25 pixels the gap
    # between the capsule and the cradle is less than a pixel, so the small icon
    # leaves it out and reads as a plain microphone on a stand instead.
    if not small:
        cradle_r = n * 0.22
        cradle_cy = n * 0.40
        d.arc(
            [cx - cradle_r, cradle_cy - cradle_r, cx + cradle_r, cradle_cy + cradle_r],
            start=25,
            end=155,
            fill=CYAN,
            width=int(n * 0.058 * stroke),
        )

    # Stem and base.
    stem_w = n * 0.052 * stroke
    stem_top = n * (0.60 if not small else 0.62)
    d.rectangle([cx - stem_w / 2, stem_top, cx + stem_w / 2, n * 0.84], fill=CYAN)
    base_w = n * (0.28 if small else 0.24)
    base_h = n * 0.052 * stroke
    d.rectangle([cx - base_w / 2, n * 0.84 - base_h, cx + base_w / 2, n * 0.84], fill=CYAN)

    return image.resize((size, size), Image.LANCZOS)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "..", "resources", "images")
    os.makedirs(out, exist_ok=True)
    for name, size in (("menu_icon.png", 25), ("app_icon.png", 48)):
        path = os.path.join(out, name)
        icon = draw_small(25) if size <= 25 else draw_icon(size)
        icon.save(path)
        print("wrote", os.path.normpath(path), f"({size}x{size})")


if __name__ == "__main__":
    main()
