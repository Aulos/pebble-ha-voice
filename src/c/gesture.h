#pragma once
#include <pebble.h>

// A thin wrapper over the touch service that turns raw touch events into taps
// and pans, so every screen can support touch and buttons with the same code.
// On platforms without a touchscreen (basalt, flint) all of this is a no-op.

typedef void (*GestureTapHandler)(GPoint point, void *context);
typedef void (*GestureDownHandler)(GPoint point, void *context);
// dy is the movement since the previous update; ended is true on liftoff.
typedef void (*GesturePanHandler)(int16_t dx, int16_t dy, bool ended, void *context);

typedef struct {
  GestureTapHandler tap;
  GesturePanHandler pan;
  GestureDownHandler down;
  void *context;
} GestureHandlers;

// Start receiving touch events for the focused window. Copies the handlers.
void gesture_begin(const GestureHandlers *handlers);

// Stop receiving touch events (and power down the touch sensor).
void gesture_end(void);

// True when touch input reaches apps right now, so screens can word their hints
// accordingly.
bool gesture_touch_available(void);
