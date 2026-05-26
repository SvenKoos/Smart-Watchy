#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "dataCollection.h"
#include "config.h"
#include "weatherData.h"

extern bool update_gui_request;
extern int guiState;
extern weatherData currentWeather;

esp_timer_handle_t brightness_timer;
esp_timer_handle_t minute_timer;

static void timerWatchface_cb(void *arg) {
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
    esp_timer_start_periodic(minute_timer, 60 * 1000000);  // 60sec
  }
}

static void timerBrightness_cb(void *arg) {
  Serial.println("timerBrightness_cb Start");
  // Set brightness to MIN
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  displayGoToSleep();

  // GUI state
  guiState = DARK_STATE;
}

void timerEventBrightness(uint seconds) {
  const esp_timer_create_args_t brightness_timer_args = {
    .callback = &timerBrightness_cb,
    .arg = (void *)brightness_timer,
    .name = "brightness_timer"
  };

  esp_timer_create(&brightness_timer_args, &brightness_timer);

  startBrightnessTimer(seconds);
}

void startBrightnessTimer(uint seconds) {
  // Falls der Timer bereits läuft, stoppen wir ihn erst,
  // um ihn mit neuen 10 Sekunden frisch zu starten.
  if (esp_timer_is_active(brightness_timer)) {
    esp_timer_stop(brightness_timer);
  }

  esp_timer_start_once(brightness_timer, seconds * 1000000);  // 15 sec

  Serial.println("startBrightnessTimer Start");
}

void stopBrightnessTimer() {
  esp_err_t err = esp_timer_stop(brightness_timer);
  if (err == ESP_OK)
    Serial.println("stopBrightnessTimer Stop Success");
  else
    Serial.printf("stopBrightnessTimer Stop Failure: %u\n", err);
}

void displayWakup() {
  // instance.pmu.enableALDO2();  // Erst Strom an...
  // delay(50);                    // Ganz kurzes Warten für stabile Spannung

  // 4. Licht wieder an
  uint brightness = DEVICE_MAX_BRIGHTNESS_LEVEL;
  if (currentWeather.weatherIcon[2] == 'd')
    brightness = BRIGHTNESS_DAY;
  else if (currentWeather.weatherIcon[2] == 'n')
    brightness = BRIGHTNESS_NIGHT;
  // instance.incrementalBrightness(brightness);
  instance.setBrightness(brightness);
}

void displayGoToSleep() {
  // 1. Licht aus (Soforteffekt)
  // instance.setBrightness(DEVICE_MIN_BRIGHTNESS_LEVEL);
  // instance.decrementBrightness(DEVICE_MIN_BRIGHTNESS_LEVEL);
  instance.decrementBrightness(0);
  delay(10); // Kurz warten, bis das PWM-Signal der LEDs komplett abgeflacht ist
  // instance.pmu.disableALDO2();  // Schaltet die Stromversorgung der LEDs physisch ab
}
