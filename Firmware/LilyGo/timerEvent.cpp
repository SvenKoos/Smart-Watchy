#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "dataCollection.h"
#include "config.h"

extern int guiState;

static void timerWatchface_cb(lv_timer_t *timer) {
  Serial.println("timerWatchface_cb Start");

  // lv_obj_invalidate(lv_timer_get_user_data(timer));

  // get the data
  collectData();

  // GUI state
  guiState = WATCHFACE_STATE;

  // draw watch face
  drawWatchFace();
}

static void timerBrightness_cb(lv_timer_t *timer) {
  Serial.println("timerBrightness_cb Start");

  // lv_obj_invalidate(lv_timer_get_user_data(timer));

  // Set brightness to MIN
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  instance.setBrightness(DEVICE_MIN_BRIGHTNESS_LEVEL);

  lv_timer_pause(timer);

  // GUI state
  guiState = DARK_STATE;
}

lv_timer_t timerEventWatchface(void) {
  lv_obj_t *cont = lv_obj_create(lv_screen_active());
  lv_obj_add_flag(cont, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_timer_t *timer = lv_timer_create(timerWatchface_cb, 1000000, cont);
}

lv_timer_t timerEventBrightness(void) {
  lv_obj_t *cont = lv_obj_create(lv_screen_active());
  lv_obj_add_flag(cont, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_timer_t *timer = lv_timer_create(timerBrightness_cb, 10000, cont);
}
