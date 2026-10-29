#!/usr/bin/env python3
"""Draws the app icons: a house with a microphone in it.

    scripts/make-icons.py

The house says Home Assistant, the microphone says voice. One geometry is
shared by both sizes, so the small icon and the large one are the same mark:
the 25 pixel menu icon is drawn straight onto the pixel grid, because scaling
it down would grey its one and two pixel strokes, and the 48 pixel icon is the
same drawing at 8x, scaled down, which gives it clean edges.

Output: resources/images/menu_icon.png (the app menu on the watch) and
app_icon.png (the phone app and store listing).
"""
import os

from PIL import Image, ImageDraw

SCALE = 8
CYAN = (0, 170, 255, 255)
BLACK = (0, 0, 0, 255)

# The mark is designed on a 25 pixel grid; every other size is this grid scaled.
GRID = 25


def draw_mark(n):
    """Draws the house and microphone onto an n by n canvas."""
    s = n / GRID
    image = Image.new("RGBA", (n, n), BLACK)
    d = ImageDraw.Draw(image)

    def box(x0, y0, x1, y1, fill=CYAN):
        # A rectangle given in grid units, grown by one pixel on its far edges so
        # that neighbours touch instead of leaving a hairline gap.
        d.rectangle([x0 * s, y0 * s, (x1 + 1) * s - 1, (y1 + 1) * s - 1], fill=fill)

    # Roof: two strokes from the apex, run a little past the tops of the walls so
    # the joint is solid rather than showing a notch.
    stroke = max(1, round(2 * s))
    d.line([(12 * s, 2 * s), (3.6 * s, 11 * s)], fill=CYAN, width=stroke)
    d.line([(12 * s, 2 * s), (20.4 * s, 11 * s)], fill=CYAN, width=stroke)

    # Walls and floor. The black square behind them matters: the watch's app menu
    # draws the selected row in cyan, and a transparent icon would vanish in it.
    box(4, 10, 5, 21)
    box(19, 10, 20, 21)
    box(4, 21, 20, 22)

    # Microphone: a capsule with three grill bars, then a stem and a base. Three
    # bars, not two: at 25 pixels two wide bars leave only a one pixel edge on
    # each side and the capsule falls apart into stripes.
    d.rounded_rectangle(
        [9 * s, 11 * s, (15 + 1) * s - 1, (18 + 1) * s - 1],
        radius=2 * s,
        fill=CYAN,
    )
    for y in (12, 14, 16):
        box(11, y, 13, y, fill=BLACK)
    box(11, 18, 13, 20)
    box(9, 20, 15, 21)

    return image


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(here, "..", "resources", "images")
    os.makedirs(out, exist_ok=True)

    menu = draw_mark(GRID)
    menu.save(os.path.join(out, "menu_icon.png"))
    print("wrote", os.path.normpath(os.path.join(out, "menu_icon.png")), "(25x25)")

    big = draw_mark(GRID * SCALE).resize((48, 48), Image.LANCZOS)
    big.save(os.path.join(out, "app_icon.png"))
    print("wrote", os.path.normpath(os.path.join(out, "app_icon.png")), "(48x48)")


if __name__ == "__main__":
    main()
