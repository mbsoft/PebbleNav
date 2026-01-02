#include "pebble.h"

static Window *s_main_window;

static TextLayer *s_distance_layer;
static TextLayer *s_duration_layer;
static TextLayer *s_timestamp_layer;
static TextLayer *s_title_layer;
static Layer *s_nav_icon_layer;

static void nav_icon_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 3);
  
  // Draw a simple navigation "arrow" (compass needle/arrowhead style)
  // Triangle pointing top-right
  GPoint p0 = GPoint(2, bounds.size.h - 2);   // Bottom-left
  GPoint p1 = GPoint(bounds.size.w - 2, 2);   // Top-right (tip)
  GPoint p2 = GPoint(bounds.size.w / 2, bounds.size.h / 2); // Middle
  GPoint p3 = GPoint(bounds.size.w - 10, bounds.size.h - 2); // Bottom-right-ish
  
  // Simpler arrow:
  graphics_draw_line(ctx, GPoint(2, bounds.size.h / 2), GPoint(bounds.size.w - 2, bounds.size.h / 2));
  graphics_draw_line(ctx, GPoint(bounds.size.w - 2, bounds.size.h / 2), GPoint(bounds.size.w - 8, bounds.size.h / 2 - 6));
  graphics_draw_line(ctx, GPoint(bounds.size.w - 2, bounds.size.h / 2), GPoint(bounds.size.w - 8, bounds.size.h / 2 + 6));
}

static AppSync s_sync;
static uint8_t s_sync_buffer[256];

enum TravelKey {
  TRAVEL_ICON_KEY = 0x0,         // TUPLE_INT
  TRAVEL_DISTANCE_KEY = 0x1,     // TUPLE_CSTRING
  TRAVEL_DURATION_KEY = 0x2,     // TUPLE_CSTRING
  TRAVEL_TIMESTAMP_KEY = 0x3,    // TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case TRAVEL_DISTANCE_KEY:
      text_layer_set_text(s_distance_layer, new_tuple->value->cstring);
      break;

    case TRAVEL_DURATION_KEY:
      text_layer_set_text(s_duration_layer, new_tuple->value->cstring);
      break;

    case TRAVEL_TIMESTAMP_KEY:
      text_layer_set_text(s_timestamp_layer, new_tuple->value->cstring);
      break;
  }
}

static void request_travel_data(void) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) {
    return;
  }

  // Update UI to show we are requesting
  text_layer_set_text(s_timestamp_layer, "Requesting...");

  int value = 1;
  dict_write_int(iter, 1, &value, sizeof(int), true);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_title_layer = text_layer_create(GRect(0, 20, 110, 30));
  text_layer_set_text_color(s_title_layer, GColorWhite);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentRight);
  text_layer_set_text(s_title_layer, "To CMH");
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));

  // Nav Icon Layer (placed after the text)
  s_nav_icon_layer = layer_create(GRect(115, 26, 20, 20));
  layer_set_update_proc(s_nav_icon_layer, nav_icon_update_proc);
  layer_add_child(window_layer, s_nav_icon_layer);

  // Distance: Prominent
  s_distance_layer = text_layer_create(GRect(0, 50, bounds.size.w, 40));
  text_layer_set_text_color(s_distance_layer, GColorWhite);
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_font(s_distance_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));

  // Duration: Also Prominent
  s_duration_layer = text_layer_create(GRect(0, 90, bounds.size.w, 40));
  text_layer_set_text_color(s_duration_layer, GColorWhite);
  text_layer_set_background_color(s_duration_layer, GColorClear);
  text_layer_set_font(s_duration_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_duration_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_duration_layer));

  // Timestamp: Small at bottom
  s_timestamp_layer = text_layer_create(GRect(0, 135, bounds.size.w, 20));
  text_layer_set_text_color(s_timestamp_layer, GColorLightGray);
  text_layer_set_background_color(s_timestamp_layer, GColorClear);
  text_layer_set_font(s_timestamp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_timestamp_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_timestamp_layer));

  Tuplet initial_values[] = {
    TupletInteger(TRAVEL_ICON_KEY, (uint8_t) 0),
    TupletCString(TRAVEL_DISTANCE_KEY, "--- mi"),
    TupletCString(TRAVEL_DURATION_KEY, "---"),
    TupletCString(TRAVEL_TIMESTAMP_KEY, "Loading..."),
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  request_travel_data();
}

static void window_unload(Window *window) {
  layer_destroy(s_nav_icon_layer);
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_distance_layer);
  text_layer_destroy(s_duration_layer);
  text_layer_destroy(s_timestamp_layer);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", reason);
  text_layer_set_text(s_timestamp_layer, "Outbox Failed");
}

static void init(void) {
  s_main_window = window_create();
  window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorFromHEX(0x8F59A0), GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_main_window, true);

  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_open(256, 256);
}

static void deinit(void) {
  window_destroy(s_main_window);

  app_sync_deinit(&s_sync);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
