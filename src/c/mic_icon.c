#include "mic_icon.h"

// The classic Pebble microphone: a narrow grilled capsule with chamfered
// shoulders and a rounded bottom, three sound rays above it, a wide 45-degree
// cup below, then a stem and a base. Proportions follow the stock Pebble
// dictation glyph (42 wide, 80 tall), scaled to `radius`.

#define MIC_REF_RADIUS 52  // radius at which the glyph is drawn at unit scale

static int16_t prv_scale(int16_t radius, int16_t value) {
  return (int16_t)((int32_t)value * radius / MIC_REF_RADIUS);
}

static void prv_draw_capsule(GContext *ctx, int16_t cx, int16_t top, int16_t radius) {
  const int16_t half = prv_scale(radius, 13);
  const int16_t shoulder = prv_scale(radius, 7);
  const int16_t side_bottom = top + prv_scale(radius, 53);

  // Chamfered shoulders, straight sides and a narrowed, rounded bottom.
  GPoint points[8] = {
    {cx - half, side_bottom},
    {cx - half, top + prv_scale(radius, 21)},
    {cx - shoulder, top + prv_scale(radius, 15)},
    {cx + shoulder, top + prv_scale(radius, 15)},
    {cx + half, top + prv_scale(radius, 21)},
    {cx + half, side_bottom},
    {cx + prv_scale(radius, 5), top + prv_scale(radius, 62)},
    {cx - prv_scale(radius, 5), top + prv_scale(radius, 62)},
  };
  GPathInfo info = {.num_points = 8, .points = points};
  GPath *path = gpath_create(&info);
  if (path) {
    gpath_draw_outline(ctx, path);
    gpath_destroy(path);
  }
}

void mic_icon_draw(GContext *ctx, GPoint center, int16_t radius, GColor color) {
  const int16_t cx = center.x;
  const int16_t top = center.y - prv_scale(radius, 40);

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_antialiased(ctx, true);

  const int16_t stroke = radius >= 46 ? 3 : 2;
  graphics_context_set_stroke_width(ctx, stroke);

  prv_draw_capsule(ctx, cx, top, radius);

  // Grill: a centre spine crossed by three bars.
  const int16_t inner = prv_scale(radius, 11);
  graphics_draw_line(ctx, GPoint(cx, top + prv_scale(radius, 17)),
                     GPoint(cx, top + prv_scale(radius, 59)));
  const int16_t bars[] = {26, 36, 47};
  for (unsigned i = 0; i < sizeof(bars) / sizeof(bars[0]); i++) {
    int16_t y = top + prv_scale(radius, bars[i]);
    graphics_draw_line(ctx, GPoint(cx - inner, y), GPoint(cx + inner, y));
  }

  // Sound rays: straight up, and 45 degrees off the shoulders.
  graphics_draw_line(ctx, GPoint(cx, top), GPoint(cx, top + prv_scale(radius, 8)));
  graphics_draw_line(ctx, GPoint(cx - prv_scale(radius, 21), top + prv_scale(radius, 5)),
                     GPoint(cx - prv_scale(radius, 14), top + prv_scale(radius, 14)));
  graphics_draw_line(ctx, GPoint(cx + prv_scale(radius, 21), top + prv_scale(radius, 5)),
                     GPoint(cx + prv_scale(radius, 14), top + prv_scale(radius, 14)));

  // Cup: two 45-degree arms closing on a short flat bottom, wider than the capsule.
  graphics_draw_line(ctx, GPoint(cx - prv_scale(radius, 18), top + prv_scale(radius, 54)),
                     GPoint(cx - prv_scale(radius, 9), top + prv_scale(radius, 65)));
  graphics_draw_line(ctx, GPoint(cx + prv_scale(radius, 18), top + prv_scale(radius, 54)),
                     GPoint(cx + prv_scale(radius, 9), top + prv_scale(radius, 65)));
  graphics_draw_line(ctx, GPoint(cx - prv_scale(radius, 10), top + prv_scale(radius, 66)),
                     GPoint(cx + prv_scale(radius, 10), top + prv_scale(radius, 66)));

  // Stem and base.
  graphics_draw_line(ctx, GPoint(cx, top + prv_scale(radius, 69)),
                     GPoint(cx, top + prv_scale(radius, 75)));
  int16_t base_w = prv_scale(radius, 22);
  int16_t base_h = prv_scale(radius, 4);
  graphics_fill_rect(ctx, GRect(cx - base_w / 2, top + prv_scale(radius, 76), base_w, base_h),
                     base_h / 2, GCornersAll);
}
