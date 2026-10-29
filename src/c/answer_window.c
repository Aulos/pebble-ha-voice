#include "answer_window.h"

#include "assistant.h"
#include "gesture.h"
#include "main_window.h"
#include "mic_icon.h"
#include "theme.h"

#define FOOTER_HEIGHT 30
#define QUESTION_HEIGHT 34

// A round screen has no corners. The question and the footer move in from the
// edge and the answer keeps a wider margin, so nothing lands where the circle
// has already cut the screen away.
#if defined(PBL_ROUND)
#define TEXT_INSET 26
#define HEADER_INSET 56
#define HEADER_TOP 26
#define FOOTER_LIFT 18
#define FOOTER_CENTERED true
#else
#define TEXT_INSET 4
#define HEADER_INSET 0
#define HEADER_TOP 0
#define FOOTER_LIFT 0
#define FOOTER_CENTERED false
#endif

static Window *s_window;
static ScrollLayer *s_scroll;
static TextLayer *s_answer_layer;
static Layer *s_header;
static Layer *s_footer;

static GRect s_footer_frame;
static GPoint s_chip_center;
static int16_t s_chip_radius;

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

static void prv_update_content(void) {
  GRect scroll_frame = layer_get_frame(scroll_layer_get_layer(s_scroll));
  int16_t width = scroll_frame.size.w - TEXT_INSET * 2;

  GSize content = graphics_text_layout_get_content_size(
      assistant_answer(), fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(0, 0, width, 2000), GTextOverflowModeWordWrap, GTextAlignmentLeft);

  int16_t height = content.h + 8;
  if (height < scroll_frame.size.h) {
    height = scroll_frame.size.h;
  }

  layer_set_frame(text_layer_get_layer(s_answer_layer),
                  GRect(TEXT_INSET, 0, width, content.h + 4));
  scroll_layer_set_content_size(s_scroll, GSize(scroll_frame.size.w, height));
  scroll_layer_set_content_offset(s_scroll, GPointZero, false);
}

static void prv_layout(GRect bounds) {
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  s_footer_frame = GRect(0, h - FOOTER_HEIGHT - FOOTER_LIFT, w, FOOTER_HEIGHT);
  s_chip_radius = 14;
  s_chip_center = GPoint(20, s_footer_frame.size.h / 2);

  layer_set_frame(s_header, GRect(HEADER_INSET, HEADER_TOP, w - HEADER_INSET * 2, QUESTION_HEIGHT));

  int16_t scroll_top = HEADER_TOP + QUESTION_HEIGHT + 2;
  layer_set_frame(scroll_layer_get_layer(s_scroll),
                  GRect(0, scroll_top, w, s_footer_frame.origin.y - scroll_top));
  layer_set_frame(s_footer, s_footer_frame);

  if (FOOTER_CENTERED) {
    // The chip and its label are centred as one group.
    GSize label = graphics_text_layout_get_content_size(
        "ASK AGAIN", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 0, 200, 20),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    int16_t group = s_chip_radius * 2 + 8 + label.w;
    s_chip_center.x = (w - group) / 2 + s_chip_radius;
  }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

static void prv_header_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t text_left = 0;

#if !defined(PBL_ROUND)
  // Accent bar marking the question the answer belongs to. On a round screen
  // there is no room for it beside the question.
  graphics_context_set_fill_color(ctx, THEME_MIC);
  graphics_fill_rect(ctx, GRect(5, 5, 3, bounds.size.h - 10), 1, GCornersAll);
  text_left = 8;
#endif

  graphics_context_set_text_color(ctx, THEME_DIM);
  graphics_draw_text(ctx, assistant_question(), fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(text_left, 2, bounds.size.w - text_left, bounds.size.h - 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void prv_footer_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, THEME_MIC);
  graphics_fill_circle(ctx, s_chip_center, s_chip_radius);
  mic_icon_draw(ctx, s_chip_center, s_chip_radius, THEME_GLYPH);

  graphics_context_set_text_color(ctx, THEME_DIM);
  graphics_draw_text(ctx, "ASK AGAIN", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(s_chip_center.x + s_chip_radius + 8,
                           bounds.size.h / 2 - 9,
                           bounds.size.w - (s_chip_center.x + s_chip_radius + 12), 20),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------

static void prv_ask_again(void) {
  window_stack_pop(true);
  main_window_ask();
}

static void prv_select_click(ClickRecognizerRef recognizer, void *context) {
  prv_ask_again();
}

static void prv_click_config_provider(void *context) {
  window_set_click_context(BUTTON_ID_UP, s_scroll);
  window_set_click_context(BUTTON_ID_DOWN, s_scroll);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 60, scroll_layer_scroll_up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 60, scroll_layer_scroll_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click);
}

// ---------------------------------------------------------------------------
// Touch
// ---------------------------------------------------------------------------

static void prv_scroll_by(int16_t dy) {
  GRect frame = layer_get_frame(scroll_layer_get_layer(s_scroll));
  GSize content = scroll_layer_get_content_size(s_scroll);
  GPoint offset = scroll_layer_get_content_offset(s_scroll);

  int16_t min_offset = -(content.h - frame.size.h);
  if (min_offset > 0) {
    min_offset = 0;
  }

  offset.y += dy;
  if (offset.y > 0) {
    offset.y = 0;
  } else if (offset.y < min_offset) {
    offset.y = min_offset;
  }
  scroll_layer_set_content_offset(s_scroll, offset, false);
}

static void prv_touch_pan(int16_t dx, int16_t dy, bool ended, void *context) {
  prv_scroll_by(dy);
}

static void prv_touch_tap(GPoint point, void *context) {
  if (point.y >= s_footer_frame.origin.y) {
    prv_ask_again();
  }
}

// ---------------------------------------------------------------------------
// Window handlers
// ---------------------------------------------------------------------------

static void prv_window_appear(Window *window) {
  gesture_begin(&(GestureHandlers){
    .tap = prv_touch_tap,
    .pan = prv_touch_pan,
    .context = NULL,
  });
}

static void prv_window_disappear(Window *window) {
  gesture_end();
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_header = layer_create(GRectZero);
  layer_set_update_proc(s_header, prv_header_update);
  layer_add_child(root, s_header);

  s_scroll = scroll_layer_create(GRectZero);
  scroll_layer_set_shadow_hidden(s_scroll, true);
  layer_add_child(root, scroll_layer_get_layer(s_scroll));

  s_answer_layer = text_layer_create(GRectZero);
  text_layer_set_background_color(s_answer_layer, GColorClear);
  text_layer_set_text_color(s_answer_layer, THEME_ANSWER);
  text_layer_set_font(s_answer_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_overflow_mode(s_answer_layer, GTextOverflowModeWordWrap);
  text_layer_set_text(s_answer_layer, assistant_answer());
  scroll_layer_add_child(s_scroll, text_layer_get_layer(s_answer_layer));

  s_footer = layer_create(GRectZero);
  layer_set_update_proc(s_footer, prv_footer_update);
  layer_add_child(root, s_footer);

  prv_layout(bounds);
  prv_update_content();
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_footer);
  text_layer_destroy(s_answer_layer);
  scroll_layer_destroy(s_scroll);
  layer_destroy(s_header);
  window_destroy(s_window);
  s_window = NULL;
}

void answer_window_push(void) {
  s_window = window_create();
  window_set_background_color(s_window, THEME_BG);
  window_set_click_config_provider(s_window, prv_click_config_provider);
#if defined(PBL_TOUCH)
  window_set_touch_bridge_disabled(s_window, true);
#endif
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
    .appear = prv_window_appear,
    .disappear = prv_window_disappear,
  });
  window_stack_push(s_window, true);
}
