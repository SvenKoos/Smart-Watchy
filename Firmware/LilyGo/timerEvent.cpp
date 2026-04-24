#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "dataCollection.h"
#include "config.h"

extern bool update_gui_request;
extern int guiState;

esp_timer_handle_t brightness_timer;
esp_timer_handle_t minute_timer;

void timerWatchface_cb(void *arg) {
  Serial.println("timerWatchface_cb Start");

  update_gui_request = true;
}

void timerEventWatchface(void) {
  const esp_timer_create_args_t timer_args = {
    .callback = &timerWatchface_cb,
    .name = "watch_minute_timer"
  };

  // Erstellen und direkt periodisch starten
  esp_err_t err = esp_timer_create(&timer_args, &minute_timer);
  if (err == ESP_OK) {
    esp_timer_start_periodic(minute_timer, 60 * 1000000);   // 60sec
  }
}

void timerBrightness_cb(void *arg) {
  Serial.println("timerBrightness_cb Start");
  // Set brightness to MIN
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  instance.setBrightness(DEVICE_MIN_BRIGHTNESS_LEVEL);

  // GUI state
  guiState = DARK_STATE;
}

void timerEventBrightness(void) {
  const esp_timer_create_args_t brightness_timer_args = {
    .callback = &timerBrightness_cb,
    .arg = (void *)brightness_timer,
    .name = "brightness_timer"
  };

  esp_timer_create(&brightness_timer_args, &brightness_timer);

  startBrightnessTimer();
}

void startBrightnessTimer() {
  // Falls der Timer bereits läuft, stoppen wir ihn erst,
  // um ihn mit neuen 10 Sekunden frisch zu starten.
  if (esp_timer_is_active(brightness_timer)) {
    esp_timer_stop(brightness_timer);
  }

  esp_timer_start_once(brightness_timer, 15 * 1000000); // 15 sec

  Serial.println("startBrightnessTimer Start");
}

void stopBrightnessTimer() {
  esp_err_t err = esp_timer_stop(brightness_timer);
  if (err == ESP_OK)
    Serial.println("stopBrightnessTimer Stop Success");
  else
    Serial.printf("stopBrightnessTimer Stop Failure: %u\n", err);
}
