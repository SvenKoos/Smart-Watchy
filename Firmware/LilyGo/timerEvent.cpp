#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "dataCollection.h"
#include "config.h"

extern int guiState;
esp_timer_handle_t brightness_timer;

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

lv_timer_t timerEventWatchface(void) {
  lv_obj_t *cont = lv_screen_active();
  // lv_obj_add_flag(cont, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  // lv_timer_t *timer = lv_timer_create(timerWatchface_cb, 60000, cont);
}

static void timerBrightness_cb(void *arg) {
  Serial.println("timerBrightness_cb Start");
  // Set brightness to MIN
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  instance.setBrightness(DEVICE_MIN_BRIGHTNESS_LEVEL);

  // GUI state
  guiState = DARK_STATE;
}

esp_timer_handle_t timerEventBrightness(void) {
  const esp_timer_create_args_t brightness_timer_args = {
    .callback = &timerBrightness_cb,
    .arg = (void *)brightness_timer,
    .name = "brightness_timer"
  };

  esp_timer_create(&brightness_timer_args, &brightness_timer);

  startBrightnessTimer(brightness_timer);

  return brightness_timer;
}

void startBrightnessTimer(esp_timer_handle_t timer) {
  // Falls der Timer bereits läuft, stoppen wir ihn erst,
  // um ihn mit neuen 10 Sekunden frisch zu starten.
  if (esp_timer_is_active(timer)) {
    esp_timer_stop(timer);
  }

  esp_timer_start_once(timer, 10*1000000);

  Serial.println("startBrightnessTimer Start");
}

void stopBrightnessTimer(esp_timer_handle_t timer) {
  esp_err_t err = esp_timer_stop(timer);
  if (err == ESP_OK)
    Serial.println("stopBrightnessTimer Stop Success");
  else
    Serial.printf("stopBrightnessTimer Stop Failure: %u\n", err);
}
