#include "aleph.h"
#include <pebble.h>
#include <time.h>

#include "string.h"
#include "xprintf.h"
#include "stdlib.h"
#include "hebrewdate.h"

static struct SimpleAnalogData {
  Layer *simple_bg_layer;
  Layer *date_layer;
  
  TextLayer *hebrew_day_label;
  char hebrew_day_buffer[10];
  TextLayer *day_of_week_label;
  char day_of_week_buffer[10];
  TextLayer *gregorian_month_label;
  char gregorian_month_buffer[10];
  TextLayer *hebrew_month_label;
  char hebrew_month_buffer[10];
  TextLayer *num_label;
  char num_buffer[4];
  
  GPath *minute_arrow, *hour_arrow;
  GPath *tick_paths[NUM_CLOCK_TICKS];
  Layer *hands_layer;
  
  TextLayer *hourOne;
  TextLayer *hourTwo;
  TextLayer *hourThree;
  TextLayer *hourFour;
  TextLayer *hourFive;
  TextLayer *hourSix;
  TextLayer *hourSeven;
  TextLayer *hourEight;
  TextLayer *hourNine;
  TextLayer *hourTen;
  TextLayer *hourEleven;
  TextLayer *hourTwelve;
  
  Window *window;
} s_data;


static void bg_update_proc(Layer* me, GContext* ctx) {

  graphics_context_set_fill_color(ctx, GColorBlack);
  GRect bounds = layer_get_bounds(me);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_fill_color(ctx, GColorWhite);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
	Layer *window_layer = window_get_root_layer(s_data.window);
	layer_mark_dirty(window_layer);
}

static void hands_update_proc(Layer* me, GContext* ctx) {
  GRect bounds = layer_get_bounds(me);
  const GPoint center = grect_center_point(&bounds);
  const int16_t secondHandLength = layer_get_bounds(me).size.w / 2;

  GPoint secondHand;
  time_t my_time = time(NULL);
  struct tm *t = localtime(&my_time);

  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  secondHand.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
  secondHand.x = (int16_t)(sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;

  // second hand
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, secondHand, center);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  gpath_rotate_to(s_data.minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_data.minute_arrow);
  gpath_draw_outline(ctx, s_data.minute_arrow);

  gpath_rotate_to(s_data.hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_data.hour_arrow);
  gpath_draw_outline(ctx, s_data.hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(layer_get_bounds(me).size.w / 2-1, layer_get_bounds(me).size.h / 2-1, 3, 3), 0, GCornerNone);
}

static void date_update_proc(Layer* me, GContext* ctx) {

  time_t my_time = time(NULL);
  struct tm *currentPblTime = localtime(&my_time); 

  strftime(s_data.num_buffer, sizeof(s_data.num_buffer), "%d", currentPblTime);
  text_layer_set_text(s_data.num_label, s_data.num_buffer);
  
  int julianDay = hdate_gdate_to_jd(currentPblTime->tm_mday, currentPblTime->tm_mon + 1, currentPblTime->tm_year + 1900);
  //Convert julian day to hebrew date
  int hDay, hMonth, hYear, hDayTishrey, hNextTishrey;
  hdate_jd_to_hdate(julianDay, &hDay, &hMonth, &hYear, &hDayTishrey, &hNextTishrey);
  xsprintf(s_data.hebrew_day_buffer, "%s ",hebrewNumbers[hDay-1]);
  text_layer_set_text(s_data.hebrew_day_label, s_data.hebrew_day_buffer);
  
  xsprintf(s_data.hebrew_month_buffer, "%s ",hebrewMonths[hMonth-1]);
  text_layer_set_text(s_data.hebrew_month_label, s_data.hebrew_month_buffer);    
  
  
  xsprintf(s_data.day_of_week_buffer, "%s ",hebrewDays[currentPblTime->tm_wday]);
  text_layer_set_text(s_data.day_of_week_label, s_data.day_of_week_buffer);
  
  xsprintf(s_data.gregorian_month_buffer, "%s ",gregoianMonths[currentPblTime->tm_mon]);
  text_layer_set_text(s_data.gregorian_month_label, s_data.gregorian_month_buffer);

}

TextLayer* initAlephLayers(GFont hebrewFont, GRect rec,const char *text ){
  TextLayer *newLayer = text_layer_create(rec);
  text_layer_set_background_color(newLayer, GColorBlack);
  text_layer_set_text_color(newLayer, GColorWhite);
  text_layer_set_font(newLayer, hebrewFont);
  text_layer_set_overflow_mode(newLayer,GTextOverflowModeFill);
  text_layer_set_text_alignment(newLayer, GTextAlignmentCenter);  
  
  text_layer_set_text(newLayer, text);
  layer_add_child(s_data.simple_bg_layer, text_layer_get_layer(newLayer));
  return newLayer;
}

static void setupLayer(TextLayer *layer, GFont hebrewFont){
  text_layer_set_background_color(layer, GColorBlack);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, hebrewFont);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_data.window);
  
  // init layers
  GRect bounds = layer_get_bounds(window_layer);
  s_data.simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_data.simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_data.simple_bg_layer);

  
  // init hand paths
  s_data.minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_data.hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  const GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_data.minute_arrow, center);
  gpath_move_to(s_data.hour_arrow, center);

  // init clock face paths
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
	s_data.tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  GFont hebrewFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NRKIS_20));

  // init date layer -> a plain parent layer to create a date update proc
  s_data.date_layer = layer_create(bounds);
  layer_set_update_proc(s_data.date_layer, date_update_proc);
  layer_add_child(s_data.simple_bg_layer, s_data.date_layer);  
  
  //init hebrew day
  s_data.hebrew_day_label = text_layer_create(GRect(63, 23, 25,70));
  setupLayer(s_data.hebrew_day_label,hebrewFont);
  layer_add_child(s_data.date_layer, text_layer_get_layer(s_data.hebrew_day_label));
  
  //init hebrew month
  s_data.hebrew_month_label = text_layer_create(GRect(50, 43, 50, 20));
  setupLayer(s_data.hebrew_month_label,hebrewFont);
  layer_add_child(s_data.date_layer, text_layer_get_layer(s_data.hebrew_month_label));
  
  // init hebrew day of week
  s_data.day_of_week_label = text_layer_create(GRect(55, 92, 50, 20));
  setupLayer(s_data.day_of_week_label,hebrewFont);
  layer_add_child(s_data.date_layer, text_layer_get_layer(s_data.day_of_week_label));
  
  s_data.gregorian_month_label = text_layer_create(GRect(40, 111, 50, 20));
  setupLayer(s_data.gregorian_month_label,hebrewFont);
  layer_add_child(s_data.date_layer, text_layer_get_layer(s_data.gregorian_month_label));
    
  
  //init aleph numbers
  s_data.hourOne = initAlephLayers(hebrewFont,GRect(100, 10, 20, 20),hebrewNumbers[0]);
  s_data.hourTwo = initAlephLayers(hebrewFont,GRect(115, 40, 25, 25),hebrewNumbers[1]);
  s_data.hourThree = initAlephLayers(hebrewFont,GRect(125, 70, 20, 20),hebrewNumbers[2]);
  s_data.hourFour = initAlephLayers(hebrewFont,GRect(115, 100, 25, 25),hebrewNumbers[3]);
  s_data.hourFive = initAlephLayers(hebrewFont,GRect(100, 130, 20, 20),hebrewNumbers[4]);
  s_data.hourSix = initAlephLayers(hebrewFont,GRect(62, 140, 20, 20),hebrewNumbers[5]);
  s_data.hourSeven = initAlephLayers(hebrewFont,GRect(25, 130, 20, 20),hebrewNumbers[6]);
  s_data.hourEight = initAlephLayers(hebrewFont,GRect(10, 100, 20, 20),hebrewNumbers[7]);
  s_data.hourNine = initAlephLayers(hebrewFont,GRect(0, 70, 20, 20),hebrewNumbers[8]);
  s_data.hourTen = initAlephLayers(hebrewFont,GRect(10, 37, 20, 20),hebrewNumbers[9]);
  s_data.hourEleven = initAlephLayers(hebrewFont,GRect(25, 10, 20, 20),hebrewNumbers[10]);
  s_data.hourTwelve = initAlephLayers(hebrewFont,GRect(62, 0, 25, 25),hebrewNumbers[11]);

  // init gregorian day
  s_data.num_label = text_layer_create(GRect(90, 111, 30, 30));
  setupLayer(s_data.num_label,hebrewFont);
  layer_add_child(s_data.date_layer, text_layer_get_layer(s_data.num_label));

  // init hands
  bounds = layer_get_bounds(window_layer);
  s_data.hands_layer = layer_create(bounds);
  layer_set_update_proc(s_data.hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_data.hands_layer);
  
  // Push the window onto the stack
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);	
}

static void window_unload(Window *window) {
	text_layer_destroy(s_data.hebrew_day_label);
	text_layer_destroy(s_data.hebrew_month_label);
	text_layer_destroy(s_data.day_of_week_label);
	text_layer_destroy(s_data.gregorian_month_label);
	text_layer_destroy(s_data.num_label);
	layer_destroy(s_data.date_layer);
	
	text_layer_destroy(s_data.hourOne);
	text_layer_destroy(s_data.hourTwo);
	text_layer_destroy(s_data.hourThree);
	text_layer_destroy(s_data.hourFour);
	text_layer_destroy(s_data.hourFive);
	text_layer_destroy(s_data.hourSix);
	text_layer_destroy(s_data.hourSeven);
	text_layer_destroy(s_data.hourEight);
	text_layer_destroy(s_data.hourNine);
	text_layer_destroy(s_data.hourTen);
	text_layer_destroy(s_data.hourEleven);
	text_layer_destroy(s_data.hourTwelve);
	
	layer_destroy(s_data.simple_bg_layer);
}

static void handle_init() {
  s_data.day_of_week_buffer[0] = '\0';
  s_data.num_buffer[0] = '\0';
  s_data.hebrew_day_buffer[0] = '\0';
  s_data.hebrew_month_buffer[0] = '\0';

  s_data.window = window_create();
   window_set_window_handlers(s_data.window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const bool animated = true;
  window_stack_push(s_data.window, animated);
  
}

static void handle_deinit(){
	tick_timer_service_unsubscribe();
	window_destroy(s_data.window);
}

// MAIN - starts the app
int main(void) {
  handle_init();
  APP_LOG(APP_LOG_LEVEL_INFO, "initializing, pushed window");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_data.window);
  app_event_loop();
  handle_deinit();
}

