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

static Window *s_window;
static MenuLayer *s_menu;
static TextLayer *s_header;

static uint16_t prv_num_rows(MenuLayer *menu, uint16_t section, void *context) {
  return 3;
}

static void prv_draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *context) {
  GRect bounds = layer_get_bounds(cell);
  bool selected = menu_cell_layer_is_highlighted(cell);

  if (selected) {
    graphics_context_set_fill_color(ctx, THEME_MIC);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  GColor title_color = selected ? THEME_GLYPH : THEME_TITLE;
  GColor subtitle_color = selected ? THEME_GLYPH : THEME_DIM;

  const char *titles[] = {"Ask again", "Settings", "Test connection"};
  const char *subtitles[] = {"Talk to Home Assistant", "Address and token, on the phone",
                             "Check the address and token"};
  const char *title = titles[index->row];
  const char *subtitle = subtitles[index->row];

  graphics_context_set_text_color(ctx, title_color);
  graphics_draw_text(ctx, title, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                     GRect(8, 2, bounds.size.w - 16, 22), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  graphics_context_set_text_color(ctx, subtitle_color);
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(8, 22, bounds.size.w - 16, 18), GTextOverflowModeTrailingEllipsis,
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

  // The title sits in its own layer: the menu layer's own header cell is drawn
  // a few pixels too high on this display.
  s_header = text_layer_create(GRect(8, 4, bounds.size.w - 16, 20));
  text_layer_set_background_color(s_header, GColorClear);
  text_layer_set_text_color(s_header, THEME_MIC);
  text_layer_set_font(s_header, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text(s_header, "HA VOICE");
  layer_add_child(root, text_layer_get_layer(s_header));

  s_menu = menu_layer_create(GRect(0, HEADER_HEIGHT, bounds.size.w, bounds.size.h - HEADER_HEIGHT));
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
