#include "gesture.h"

// A touch that stays within this many pixels of where it started, and is
// shorter than this, counts as a tap.
#define GESTURE_TAP_SLOP_PX 12
#define GESTURE_TAP_MAX_MS 450

static struct {
  GestureHandlers handlers;
  bool active;
  bool down;
  GPoint start;
  GPoint last;
  bool moved;
  uint32_t start_ms;
} s_gesture;

#if defined(PBL_TOUCH)

static void prv_touch_handler(const TouchEvent *event, void *context) {
  if (!s_gesture.active || event->non_navigational) {
    return;
  }

  GPoint point = GPoint(event->x, event->y);

  switch (event->type) {
    case TouchEvent_Touchdown:
      s_gesture.down = true;
      s_gesture.moved = false;
      s_gesture.start = point;
      s_gesture.last = point;
      s_gesture.start_ms = time_ms(NULL, NULL);
      if (s_gesture.handlers.down) {
        s_gesture.handlers.down(point, s_gesture.handlers.context);
      }
      break;

    case TouchEvent_PositionUpdate:
      if (!s_gesture.down) {
        break;
      }
      if (!s_gesture.moved) {
        int16_t tdx = point.x - s_gesture.start.x;
        int16_t tdy = point.y - s_gesture.start.y;
        if (tdx > GESTURE_TAP_SLOP_PX || tdx < -GESTURE_TAP_SLOP_PX ||
            tdy > GESTURE_TAP_SLOP_PX || tdy < -GESTURE_TAP_SLOP_PX) {
          s_gesture.moved = true;
        }
      }
      if (s_gesture.moved && s_gesture.handlers.pan) {
        s_gesture.handlers.pan(point.x - s_gesture.last.x, point.y - s_gesture.last.y,
                               false, s_gesture.handlers.context);
      }
      s_gesture.last = point;
      break;

    case TouchEvent_Liftoff:
      if (!s_gesture.down) {
        break;
      }
      s_gesture.down = false;
      if (s_gesture.moved) {
        if (s_gesture.handlers.pan) {
          s_gesture.handlers.pan(point.x - s_gesture.last.x, point.y - s_gesture.last.y,
                                 true, s_gesture.handlers.context);
        }
      } else if (time_ms(NULL, NULL) - s_gesture.start_ms <= GESTURE_TAP_MAX_MS) {
        if (s_gesture.handlers.tap) {
          s_gesture.handlers.tap(point, s_gesture.handlers.context);
        }
      }
      s_gesture.last = point;
      break;
  }
}

#endif  // PBL_TOUCH

void gesture_begin(const GestureHandlers *handlers) {
  s_gesture.handlers = *handlers;
  s_gesture.down = false;
  s_gesture.moved = false;
#if defined(PBL_TOUCH)
  if (!s_gesture.active) {
    touch_service_subscribe(prv_touch_handler, NULL);
    s_gesture.active = true;
  }
#else
  s_gesture.active = false;
#endif
}

void gesture_end(void) {
#if defined(PBL_TOUCH)
  if (s_gesture.active) {
    touch_service_unsubscribe();
    s_gesture.active = false;
  }
#else
  s_gesture.active = false;
#endif
  s_gesture.down = false;
  s_gesture.moved = false;
  s_gesture.handlers.tap = NULL;
  s_gesture.handlers.pan = NULL;
  s_gesture.handlers.down = NULL;
  s_gesture.handlers.context = NULL;
}

bool gesture_touch_available(void) {
#if defined(PBL_TOUCH)
  return touch_service_is_enabled();
#else
  return false;
#endif
}
