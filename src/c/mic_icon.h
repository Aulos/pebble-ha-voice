#pragma once
#include <pebble.h>

// Draws the microphone glyph (capsule, holder arc, stem, base) centred on
// `center`, sized relative to `radius`.
void mic_icon_draw(GContext *ctx, GPoint center, int16_t radius, GColor color);
