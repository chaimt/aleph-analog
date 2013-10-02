#include "aleph.h"

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "string.h"
#include "xprintf.h"
#include "stdlib.h"
#include "hebrewdate.h"

#define MY_UUID {0xe0, 0xfc, 0x1c, 0x31, 0x46, 0x8c, 0x4f, 0x69, 0xba, 0xbd, 0xd2, 0xec, 0x7a, 0xb3, 0x18, 0x13}
PBL_APP_INFO(MY_UUID,
             "Aleph Bet",
             "Pebble Technology",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_ALEPH,
             APP_INFO_WATCH_FACE);

static struct SimpleAnalogData {
  Layer simple_bg_layer;

  Layer date_layer;
  TextLayer hebrew_day_label;
  char hebrew_day_buffer[6];
  TextLayer day_label;
  char day_buffer[6];
  TextLayer hebrew_month_label;
  char hebrew_month_buffer[6];
  TextLayer num_label;
  char num_buffer[4];
  
  GPath minute_arrow, hour_arrow;
  GPath tick_paths[NUM_CLOCK_TICKS];
  Layer hands_layer;
  
  TextLayer hourOne;
  TextLayer hourTwo;
  TextLayer hourThree;
  TextLayer hourFour;
  TextLayer hourFive;
  TextLayer hourSix;
  TextLayer hourSeven;
  TextLayer hourEight;
  TextLayer hourNine;
  TextLayer hourTen;
  TextLayer hourEleven;
  TextLayer hourTwelve;
  
  Window window;
} s_data;

static void bg_update_proc(Layer* me, GContext* ctx) {

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, me->bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorWhite);
}



static void handle_second_tick(AppContextRef ctx, PebbleTickEvent* t) {
  layer_mark_dirty(&s_data.window.layer);
}

static void hands_update_proc(Layer* me, GContext* ctx) {
  const GPoint center = grect_center_point(&me->bounds);
  const int16_t secondHandLength = me->bounds.size.w / 2;

  GPoint secondHand;

  PblTm t;
  get_time(&t);

  int32_t second_angle = TRIG_MAX_ANGLE * t.tm_sec / 60;
  secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
  secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;

  // second hand
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, secondHand, center);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(&s_data.minute_arrow, TRIG_MAX_ANGLE * t.tm_min / 60);
  gpath_draw_filled(ctx, &s_data.minute_arrow);
  gpath_draw_outline(ctx, &s_data.minute_arrow);

  gpath_rotate_to(&s_data.hour_arrow, (TRIG_MAX_ANGLE * (((t.tm_hour % 12) * 6) + (t.tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, &s_data.hour_arrow);
  gpath_draw_outline(ctx, &s_data.hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(me->bounds.size.w / 2-1, me->bounds.size.h / 2-1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer* me, GContext* ctx) {

  PblTm currentPblTime;
  get_time(&currentPblTime);

  string_format_time(s_data.day_buffer, sizeof(s_data.day_buffer), "%a", &currentPblTime);
  text_layer_set_text(&s_data.day_label, s_data.day_buffer);

  string_format_time(s_data.num_buffer, sizeof(s_data.num_buffer), "%d", &currentPblTime);
  text_layer_set_text(&s_data.num_label, s_data.num_buffer);
  
   int julianDay = hdate_gdate_to_jd(currentPblTime.tm_mday, currentPblTime.tm_mon + 1, currentPblTime.tm_year + 1900);
  // Convert julian day to hebrew date
  int hDay, hMonth, hYear, hDayTishrey, hNextTishrey;
  hdate_jd_to_hdate(julianDay, &hDay, &hMonth, &hYear, &hDayTishrey, &hNextTishrey);
  //string_format_time(s_data.hebrew_day_buffer, sizeof(s_data.hebrew_day_buffer), "%d",&hDay);
  xsprintf(s_data.hebrew_day_buffer, "%s",hebrewNumbers[hDay-1]);
  text_layer_set_text(&s_data.hebrew_day_label, s_data.hebrew_day_buffer);
  
  xsprintf(s_data.hebrew_month_buffer, "%s",hebrewMonths[hMonth-1]);
  text_layer_set_text(&s_data.hebrew_month_label, s_data.hebrew_month_buffer);
    
}

void initAlephLayers(TextLayer* textLayer, GFont davidFont, GRect rec,const char * 	text ){
  text_layer_init(textLayer, rec);
  text_layer_set_background_color(textLayer, GColorBlack);
  text_layer_set_text_color(textLayer, GColorWhite);
  text_layer_set_font(textLayer, davidFont);
  
  text_layer_set_text(textLayer, text);
  layer_add_child(&s_data.simple_bg_layer, &textLayer->layer);
}

static void handle_init(AppContextRef app_ctx) {
  window_init(&s_data.window, "Aleph Bet");

  s_data.day_buffer[0] = '\0';
  s_data.num_buffer[0] = '\0';
  s_data.hebrew_day_buffer[0] = '\0';
  

  // init hand paths
  gpath_init(&s_data.minute_arrow, &MINUTE_HAND_POINTS);
  gpath_init(&s_data.hour_arrow, &HOUR_HAND_POINTS);
  resource_init_current_app(&APP_RESOURCES);

  const GPoint center = grect_center_point(&s_data.window.layer.bounds);
  gpath_move_to(&s_data.minute_arrow, center);
  gpath_move_to(&s_data.hour_arrow, center);

  // init clock face paths
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_init(&s_data.tick_paths[i], &ANALOG_BG_POINTS[i]);
  }

  // init layers
  layer_init(&s_data.simple_bg_layer, s_data.window.layer.frame);
  s_data.simple_bg_layer.update_proc = &bg_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.simple_bg_layer);

  // init date layer -> a plain parent layer to create a date update proc
  layer_init(&s_data.date_layer, s_data.window.layer.frame);
  s_data.date_layer.update_proc = &date_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.date_layer);

  // init day
  text_layer_init(&s_data.day_label, GRect(46, 114, 27, 20));
  text_layer_set_text(&s_data.day_label, s_data.day_buffer);
  text_layer_set_background_color(&s_data.day_label, GColorBlack);
  text_layer_set_text_color(&s_data.day_label, GColorWhite);
  GFont norm18 = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  text_layer_set_font(&s_data.day_label, norm18);
  layer_add_child(&s_data.date_layer, &s_data.day_label.layer);
  
  //init hebrew day
  text_layer_init(&s_data.hebrew_day_label, GRect(65, 25, 27, 20));
  text_layer_set_text(&s_data.hebrew_day_label, s_data.hebrew_day_buffer);
  text_layer_set_background_color(&s_data.hebrew_day_label, GColorBlack);
  text_layer_set_text_color(&s_data.hebrew_day_label, GColorWhite);
  GFont davidFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DAVID_20));
  text_layer_set_font(&s_data.hebrew_day_label, davidFont);
  text_layer_set_text(&s_data.hebrew_day_label, "abc");
  layer_add_child(&s_data.date_layer, &s_data.hebrew_day_label.layer);
  
  //init hebrew month
  text_layer_init(&s_data.hebrew_month_label, GRect(50, 45, 50, 70));
  text_layer_set_text(&s_data.hebrew_month_label, s_data.hebrew_month_buffer);
  text_layer_set_background_color(&s_data.hebrew_month_label, GColorBlack);
  text_layer_set_text_color(&s_data.hebrew_month_label, GColorWhite);
  text_layer_set_font(&s_data.hebrew_month_label, davidFont);
  text_layer_set_text(&s_data.hebrew_month_label, "abc");
  text_layer_set_text_alignment(&s_data.hebrew_month_label, GTextAlignmentCenter);
  layer_add_child(&s_data.date_layer, &s_data.hebrew_month_label.layer);
  
  
  
  //init aleph numbers
  initAlephLayers(&s_data.hourTwelve,davidFont,GRect(67, 0, 20, 20),"בי");
  initAlephLayers(&s_data.hourOne,davidFont,GRect(100, 10, 20, 20),"א");
  initAlephLayers(&s_data.hourTwo,davidFont,GRect(125, 35, 20, 20),"ב");
  initAlephLayers(&s_data.hourThree,davidFont,GRect(135, 70, 20, 20),"ג");
  initAlephLayers(&s_data.hourFour,davidFont,GRect(125, 105, 20, 20),"ד");
  initAlephLayers(&s_data.hourFive,davidFont,GRect(105, 130, 20, 20),"ה");
  initAlephLayers(&s_data.hourSix,davidFont,GRect(70, 140, 20, 20),"ו");
  initAlephLayers(&s_data.hourSeven,davidFont,GRect(35, 135, 20, 20),"ז");
  initAlephLayers(&s_data.hourEight,davidFont,GRect(10, 105, 20, 20),"ח");
  initAlephLayers(&s_data.hourNine,davidFont,GRect(0, 70, 20, 20),"ט");
  initAlephLayers(&s_data.hourTen,davidFont,GRect(10, 35, 20, 20),"י");
  initAlephLayers(&s_data.hourEleven,davidFont,GRect(25, 10, 20, 20),"אי");
  

  // init num
  text_layer_init(&s_data.num_label, GRect(73, 114, 18, 20));

  text_layer_set_text(&s_data.num_label, s_data.num_buffer);
  text_layer_set_background_color(&s_data.num_label, GColorBlack);
  text_layer_set_text_color(&s_data.num_label, GColorWhite);
  GFont bold18 = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  text_layer_set_font(&s_data.num_label, bold18);

  layer_add_child(&s_data.date_layer, &s_data.num_label.layer);

  // init hands
  layer_init(&s_data.hands_layer, s_data.simple_bg_layer.frame);
  s_data.hands_layer.update_proc = &hands_update_proc;
  layer_add_child(&s_data.window.layer, &s_data.hands_layer);

  
  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(&s_data.window, animated);
}


// MAIN - starts the app
void pbl_main(void* params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
