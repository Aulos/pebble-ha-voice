#!/usr/bin/env python3
"""Renders the app icons from resources/icon.svg.

    python3 scripts/make-icons.py

The SVG is the source of truth: it is drawn on the 25 pixel grid the watch's app
menu uses, with every edge on a whole pixel. The 25 pixel menu icon is that
drawing rendered straight at 25 pixels, so it stays sharp; the 48 pixel icon is
rendered four times larger and scaled down, which gives it clean edges.

Needs cairosvg:  python3 -m venv .venv && .venv/bin/pip install cairosvg
Then run:        .venv/bin/python scripts/make-icons.py

Output: resources/images/menu_icon.png (the app menu on the watch),
resources/images/app_icon.png (48 pixels, packed into the app) and
docs/store/icon-144.png (the appstore listing's icon, which is what the phone
app draws for an app it installed from the store).
"""
import io
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SVG = os.path.join(HERE, "..", "resources", "icon.svg")
OUT = os.path.join(HERE, "..", "resources", "images")

MENU_SIZE = 25
APP_SIZE = 48
STORE_SIZE = 144
SUPERSAMPLE = 4


def render(size):
    """Rasterises the SVG at `size` by `size` pixels."""
    try:
        import cairosvg
    except ImportError:
        sys.exit(
            "cairosvg is missing. Run:\n"
            "  python3 -m venv .venv && .venv/bin/pip install cairosvg\n"
            "  .venv/bin/python scripts/make-icons.py"
        )
    png = cairosvg.svg2png(url=SVG, output_width=size, output_height=size)
    return Image.open(io.BytesIO(png)).convert("RGBA")


def main():
    os.makedirs(OUT, exist_ok=True)

    menu = render(MENU_SIZE)
    menu.save(os.path.join(OUT, "menu_icon.png"))
    print("wrote", os.path.normpath(os.path.join(OUT, "menu_icon.png")), f"({MENU_SIZE}x{MENU_SIZE})")

    big = render(APP_SIZE * SUPERSAMPLE).resize((APP_SIZE, APP_SIZE), Image.LANCZOS)
    big.save(os.path.join(OUT, "app_icon.png"))
    print("wrote", os.path.normpath(os.path.join(OUT, "app_icon.png")), f"({APP_SIZE}x{APP_SIZE})")

    store_dir = os.path.join(HERE, "..", "docs", "store")
    os.makedirs(store_dir, exist_ok=True)
    store = render(STORE_SIZE * SUPERSAMPLE).resize((STORE_SIZE, STORE_SIZE), Image.LANCZOS)
    store.save(os.path.join(store_dir, "icon-144.png"))
    print("wrote", os.path.normpath(os.path.join(store_dir, "icon-144.png")), f"({STORE_SIZE}x{STORE_SIZE})")


if __name__ == "__main__":
    main()
