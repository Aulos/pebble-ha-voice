#include "menu_window.h"

#include "assistant.h"
#include "theme.h"

// A small menu, drawn to match the rest of the app: black background, white
// titles, and the cyan accent on the selected row. It uses the system menu
// layer, so scrolling, the buttons, and (on the Time 2) swipes and taps all work
// without extra code. Unlike the main window this screen leaves the touch bridge
// enabled on purpose.

#define ROW_HEIGHT 44
#define HEADER_HEIGHT 26

#if defined(PBL_ROUND)
// A round screen has no corners, so a full width row would be cut off at the
// edges and the text along with it. Every row is pulled in by the width the
// circle takes away at that row's height.
#define ROW_INSET 6
#define HEADER_TOP 4
#else
#define ROW_INSET 0
#define HEADER_TOP 4
#endif

static Window *s_window;
static MenuLayer *s_menu;
static TextLayer *s_header;

static uint16_t prv_num_rows(MenuLayer *menu, uint16_t section, void *context) {
  return 3;
}

#if defined(PBL_ROUND)
static int16_t prv_isqrt(int32_t value) {
  int32_t root = 0;
  while ((root + 1) * (root + 1) <= value) {
    root++;
  }
  return (int16_t)root;
}

// How far the circle's edge is from the left edge of the screen at the row's
// furthest point from the middle, plus a small margin. The menu layer keeps its
// cells inside a scrolling child layer, so the row is measured where it really
// is on the screen rather than in its parent's coordinates.
static int16_t prv_row_inset(const Layer *cell) {
  GRect screen = layer_get_bounds(window_get_root_layer(s_window));
  GRect on_screen = layer_convert_rect_to_screen(cell, layer_get_bounds(cell));

  int16_t center_y = screen.size.h / 2;
  int16_t top = on_screen.origin.y - center_y;
  int16_t bottom = top + on_screen.size.h;
  int16_t distance = top < 0 ? -top : top;
  int16_t far = bottom < 0 ? -bottom : bottom;
  if (far > distance) {
    distance = far;
  }

  int16_t radius = screen.size.w / 2;
  int32_t chord = (int32_t)radius * radius - (int32_t)distance * distance;
  if (chord <= 0) {
    return radius;
  }
  return radius - prv_isqrt(chord) + ROW_INSET;
}
#endif

static void prv_draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *context) {
  GRect bounds = layer_get_bounds(cell);
  bool selected = menu_cell_layer_is_highlighted(cell);

#if defined(PBL_ROUND)
  int16_t inset = prv_row_inset(cell);
#else
  int16_t inset = ROW_INSET;
#endif
  int16_t width = bounds.size.w - inset * 2;

  if (selected) {
    graphics_context_set_fill_color(ctx, THEME_MIC);
    graphics_fill_rect(ctx, GRect(inset, 0, width, bounds.size.h), ROW_INSET ? 10 : 0,
                       ROW_INSET ? GCornersAll : GCornerNone);
  }

  GColor title_color = selected ? THEME_GLYPH : THEME_TITLE;
  GColor subtitle_color = selected ? THEME_GLYPH : THEME_DIM;

  const char *titles[] = {"Ask again", "Settings", "Test connection"};
  // Kept short: the 144 pixel platforms cut a longer line off at the edge.
  const char *subtitles[] = {"Talk to Home Assistant", "Set on the phone", "Check the setup"};
  const char *title = titles[index->row];
  const char *subtitle = subtitles[index->row];

  graphics_context_set_text_color(ctx, title_color);
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                     GRect(inset + 8, 2, width - 16, 22), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  graphics_context_set_text_color(ctx, subtitle_color);
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(inset + 8, 22, width - 16, 18), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
}

static int16_t prv_row_height(MenuLayer *menu, MenuIndex *index, void *context) {
  return ROW_HEIGHT;
}

static void prv_select(MenuLayer *menu, MenuIndex *index, void *context) {
  switch (index->row) {
    case 0:
      window_stack_pop(false);
      assistant_start();
      break;
    case 1:
      assistant_open_settings();
      break;
    default:
      window_stack_pop(false);
      assistant_test_connection();
      break;
  }
}

static void prv_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

#if defined(PBL_ROUND)
  // On a round screen the menu layer centres the selected row inside its own
  // frame, so it takes the whole window and the rows land around the middle,
  // where the circle is widest.
  GRect menu_frame = bounds;
#else
  GRect menu_frame = GRect(0, HEADER_HEIGHT, bounds.size.w, bounds.size.h - HEADER_HEIGHT);
#endif

  s_menu = menu_layer_create(menu_frame);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = prv_num_rows,
    .get_cell_height = prv_row_height,
    .draw_row = prv_draw_row,
    .select_click = prv_select,
  });
  menu_layer_set_normal_colors(s_menu, THEME_BG, THEME_TITLE);
  menu_layer_set_highlight_colors(s_menu, THEME_MIC, THEME_GLYPH);
  layer_add_child(root, menu_layer_get_layer(s_menu));
  menu_layer_set_click_config_onto_window(s_menu, window);

  // The title sits in its own layer, on top of the menu: the menu layer's own
  // header cell is drawn a few pixels too high on this display.
  s_header = text_layer_create(GRect(8, HEADER_TOP, bounds.size.w - 16, 20));
  text_layer_set_background_color(s_header, GColorClear);
  text_layer_set_text_color(s_header, THEME_MIC);
  text_layer_set_font(s_header, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text(s_header, "HA VOICE");
#if defined(PBL_ROUND)
  // The top of a round screen is narrow, so the title is centred there.
  text_layer_set_text_alignment(s_header, GTextAlignmentCenter);
#endif
  layer_add_child(root, text_layer_get_layer(s_header));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_header);
  s_header = NULL;
  menu_layer_destroy(s_menu);
  s_menu = NULL;
}

void menu_window_push(void) {
  if (s_window) {
    window_stack_push(s_window, true);
    return;
  }
  s_window = window_create();
  window_set_background_color(s_window, THEME_BG);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);
}
