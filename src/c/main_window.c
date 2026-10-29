#include "main_window.h"

#include "answer_window.h"
#include "assistant.h"
#include "gesture.h"
#include "menu_window.h"
#include "mic_icon.h"
#include "theme.h"

#define ANIM_INTERVAL_MS 110

static Window *s_window;
static Layer *s_canvas;

static GRect s_status_rect;
static GRect s_hint_rect;
static GPoint s_mic_center;
static int16_t s_mic_radius;
static bool s_mic_pressed;
static int16_t s_pan_dy;

static AppTimer *s_anim_timer;
static int s_anim_phase;

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

static void prv_layout(GRect bounds) {
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;
  int16_t short_side = w < h ? w : h;

  s_mic_radius = short_side * 26 / 100;
  s_mic_center = GPoint(w / 2, (28 + (h - 26)) / 2 - 6);
  s_status_rect = GRect(4, s_mic_center.y + s_mic_radius + 6, w - 8, 36);
#if defined(PBL_ROUND)
  // The bottom of a round screen is narrow, so the hint sits a little higher.
  s_hint_rect = GRect(0, h - 34, w, 20);
#else
  s_hint_rect = GRect(0, h - 24, w, 20);
#endif
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

static void prv_anim_tick(void *context) {
  s_anim_timer = NULL;
  AssistantState state = assistant_get_state();
  if (state == AssistantListening || state == AssistantThinking) {
    s_anim_phase = (s_anim_phase + 1) % 32;
    layer_mark_dirty(s_canvas);
    s_anim_timer = app_timer_register(ANIM_INTERVAL_MS, prv_anim_tick, NULL);
  }
}

static void prv_anim_start(void) {
  if (!s_anim_timer) {
    s_anim_timer = app_timer_register(ANIM_INTERVAL_MS, prv_anim_tick, NULL);
  }
}

static void prv_anim_stop(void) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

static void prv_draw_state_text(GContext *ctx, AssistantState state) {
  if (state == AssistantError) {
    graphics_context_set_text_color(ctx, THEME_TITLE);
    graphics_draw_text(ctx, assistant_error(), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       s_status_rect, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }

  if (state == AssistantIdle && !assistant_is_configured()) {
    graphics_context_set_text_color(ctx, THEME_LISTEN);
    graphics_draw_text(ctx, "NOT SET UP", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(s_status_rect.origin.x, s_status_rect.origin.y, s_status_rect.size.w, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  const char *label = NULL;
  if (state == AssistantListening) {
    label = "LISTENING";
  } else if (state == AssistantThinking) {
    label = "THINKING";
  }
  if (!label) {
    return;
  }

  graphics_context_set_text_color(ctx,
                                  state == AssistantListening ? THEME_LISTEN : THEME_THINK);
  graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(s_status_rect.origin.x, s_status_rect.origin.y, s_status_rect.size.w, 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void prv_draw_hint(GContext *ctx, AssistantState state) {
  const char *hint;
  switch (state) {
    case AssistantListening:
      hint = "BACK TO CANCEL";
      break;
    case AssistantThinking:
      hint = "SELECT TO CANCEL";
      break;
    default:
      if (!assistant_is_configured()) {
        hint = gesture_touch_available() ? "TAP FOR SETTINGS" : "DOWN FOR SETTINGS";
      } else {
        hint = gesture_touch_available() ? "TAP OR PRESS SELECT" : "PRESS SELECT";
      }
      break;
  }

  graphics_context_set_text_color(ctx, THEME_DIM);
  graphics_draw_text(ctx, hint, fonts_get_system_font(FONT_KEY_GOTHIC_14), s_hint_rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void prv_draw_pulse(GContext *ctx) {
  // A ring that grows outwards from the button while the watch listens.
  int16_t spread = 6 + (s_anim_phase % 8) * 2;
  GColor color = (s_anim_phase / 8) % 2 ? THEME_LISTEN : THEME_LISTEN_DIM;
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_antialiased(ctx, true);
  graphics_draw_circle(ctx, s_mic_center, s_mic_radius + spread);
}

static void prv_draw_spinner(GContext *ctx) {
  int16_t r = s_mic_radius + 8;
  GRect rect = GRect(s_mic_center.x - r, s_mic_center.y - r, r * 2, r * 2);
  int32_t start = DEG_TO_TRIGANGLE((s_anim_phase * 11) % 360);
  graphics_context_set_stroke_color(ctx, THEME_TITLE);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_antialiased(ctx, true);
  graphics_draw_arc(ctx, rect, GOvalScaleModeFitCircle, start, start + DEG_TO_TRIGANGLE(90));
}

static void prv_canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  AssistantState state = assistant_get_state();

  graphics_context_set_text_color(ctx, THEME_TITLE);
  graphics_draw_text(ctx, "HA VOICE", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(0, 4, bounds.size.w, 20), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  if (state == AssistantListening) {
    prv_draw_pulse(ctx);
  } else if (state == AssistantThinking) {
    prv_draw_spinner(ctx);
  }

  GColor fill = THEME_MIC;
  if (state == AssistantListening) {
    fill = THEME_LISTEN;
  } else if (state == AssistantThinking) {
    fill = THEME_THINK;
  } else if (state == AssistantError) {
    fill = THEME_ERROR;
  } else if (s_mic_pressed) {
    fill = THEME_MIC_PRESSED;
  }

  graphics_context_set_fill_color(ctx, fill);
  graphics_fill_circle(ctx, s_mic_center, s_mic_radius);
  mic_icon_draw(ctx, s_mic_center, s_mic_radius, THEME_GLYPH);

  prv_draw_state_text(ctx, state);
  prv_draw_hint(ctx, state);
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

static bool prv_point_in_mic(GPoint point) {
  int16_t slop = 12;
  int16_t dx = point.x - s_mic_center.x;
  int16_t dy = point.y - s_mic_center.y;
  int16_t r = s_mic_radius + slop;
  return (int32_t)dx * dx + (int32_t)dy * dy <= (int32_t)r * r;
}

static void prv_activate(void) {
  AssistantState state = assistant_get_state();
  if (state == AssistantThinking) {
    assistant_cancel();
  } else if (!assistant_is_configured()) {
    // Nothing can be asked yet: open the page on the phone that fixes it.
    assistant_open_settings();
  } else {
    assistant_start();
  }
}

static void prv_select_click(ClickRecognizerRef recognizer, void *context) {
  prv_activate();
}

static void prv_up_click(ClickRecognizerRef recognizer, void *context) {
  if (assistant_has_answer()) {
    answer_window_push();
  }
}

static void prv_down_click(ClickRecognizerRef recognizer, void *context) {
  menu_window_push();
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click);
}

// ---------------------------------------------------------------------------
// Touch
// ---------------------------------------------------------------------------

static void prv_touch_down(GPoint point, void *context) {
  AssistantState state = assistant_get_state();
  if (state != AssistantIdle && state != AssistantError) {
    return;
  }
  if (prv_point_in_mic(point)) {
    s_mic_pressed = true;
    layer_mark_dirty(s_canvas);
  }
}

static void prv_touch_tap(GPoint point, void *context) {
  bool was_pressed = s_mic_pressed;
  s_mic_pressed = false;
  layer_mark_dirty(s_canvas);
  if (was_pressed || prv_point_in_mic(point)) {
    prv_activate();
  }
}

static void prv_touch_pan(int16_t dx, int16_t dy, bool ended, void *context) {
  if (s_mic_pressed) {
    s_mic_pressed = false;
    layer_mark_dirty(s_canvas);
  }
  s_pan_dy += dy;
  if (ended) {
    // A downward swipe opens the menu, like the down button.
    if (s_pan_dy > 30) {
      menu_window_push();
    }
    s_pan_dy = 0;
  }
}

// ---------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------

static void prv_state_changed(AssistantState state, void *context) {
  s_mic_pressed = false;
  if (state == AssistantListening || state == AssistantThinking) {
    prv_anim_start();
  } else {
    prv_anim_stop();
  }
  if (s_canvas) {
    layer_mark_dirty(s_canvas);
  }
}

static void prv_answer_ready(void *context) {
  answer_window_push();
}

static void prv_window_appear(Window *window) {
  s_pan_dy = 0;
  // The phone is the only side that knows whether the app is set up.
  assistant_refresh_config();
  gesture_begin(&(GestureHandlers){
    .tap = prv_touch_tap,
    .pan = prv_touch_pan,
    .down = prv_touch_down,
    .context = NULL,
  });
  AssistantState state = assistant_get_state();
  if (state == AssistantListening || state == AssistantThinking) {
    prv_anim_start();
  }
  layer_mark_dirty(s_canvas);
}

static void prv_window_disappear(Window *window) {
  gesture_end();
  prv_anim_stop();
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  prv_layout(layer_get_bounds(root));

  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, prv_canvas_update);
  layer_add_child(root, s_canvas);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void main_window_push(void) {
  s_window = window_create();
  window_set_background_color(s_window, THEME_BG);
  window_set_click_config_provider(s_window, prv_click_config_provider);
#if defined(PBL_TOUCH)
  // Handle every touch ourselves instead of letting the system bridge turn
  // swipes and taps into button presses.
  window_set_touch_bridge_disabled(s_window, true);
#endif
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
    .appear = prv_window_appear,
    .disappear = prv_window_disappear,
  });

  assistant_set_state_handler(prv_state_changed, NULL);
  assistant_set_answer_handler(prv_answer_ready, NULL);

  window_stack_push(s_window, true);
}

void main_window_ask(void) {
  assistant_start();
}
