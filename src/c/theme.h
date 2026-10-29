#pragma once
#include <pebble.h>

// Color palette for the app. The Time 2 has a 64-color display, so these are
// deliberately saturated Pebble-palette colors on a black background.
//
// The 1-bit platforms (Pebble 2, and the originals) cannot show those colors:
// a saturated color becomes a dither pattern there, and black text on a dithered
// cyan is close to unreadable, which is what the menu looked like on a Pebble 2.
// On those platforms the accent is plain white instead, so the selected row and
// the microphone are solid and their black glyphs stay legible.
#if defined(PBL_COLOR)
#define THEME_BG            GColorBlack
#define THEME_TITLE         GColorWhite
#define THEME_DIM           GColorLightGray
#define THEME_MIC           GColorVividCerulean
#define THEME_MIC_PRESSED   GColorTiffanyBlue
#define THEME_LISTEN        GColorRed
#define THEME_LISTEN_DIM    GColorBulgarianRose
#define THEME_THINK         GColorChromeYellow
#define THEME_ERROR         GColorDarkGray
#define THEME_ANSWER        GColorMintGreen
#define THEME_GLYPH         GColorBlack
#else
#define THEME_BG            GColorBlack
#define THEME_TITLE         GColorWhite
#define THEME_DIM           GColorLightGray
#define THEME_MIC           GColorWhite
#define THEME_MIC_PRESSED   GColorLightGray
// Listening and thinking still need to look different from idle, and the
// listening state already blinks, so the blink alternates between white and a
// dithered grey.
#define THEME_LISTEN        GColorLightGray
#define THEME_LISTEN_DIM    GColorWhite
#define THEME_THINK         GColorWhite
#define THEME_ERROR         GColorLightGray
#define THEME_ANSWER        GColorWhite
#define THEME_GLYPH         GColorBlack
#endif
