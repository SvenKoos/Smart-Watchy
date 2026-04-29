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
    esp_timer_start_periodic(minute_timer, 60 * 1000000);  // 60sec
  }
}

void timerBrightness_cb(void *arg) {
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
  // 1. Hardware-Befehl: Sleep Out
  // your_send_cmd(0x11);

  // 2. WICHTIG: Wartezeit für die Ladungspumpen des Displays
  // Laut MIPI-Spezifikation sind 120ms sicher, oft reichen 5-20ms.
  // delay(120);
  /*
  // 3. LVGL Re-Aktivieren
  lv_display_t *disp = lv_display_get_default();
  if (disp) {
    lv_display_remove_flag(disp, LV_DISPLAY_FLAG_READ_ONLY);

    // In v9.5 setzt dies den Inaktivitäts-Timer zurück
    lv_display_trigger_activity(disp);
  }
*/
  instance.pmu.enableALDO2();  // Erst Strom an...
  delay(5);                    // Ganz kurzes Warten für stabile Spannung
  
  // 4. Licht wieder an
  instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
}

void displayGoToSleep() {
  // 1. Licht aus (Soforteffekt)
  instance.setBrightness(DEVICE_MIN_BRIGHTNESS_LEVEL);

  instance.pmu.disableALDO2();  // Schaltet die Stromversorgung der LEDs physisch ab
  /*
  // 2. LVGL-Rendering für dieses Display stoppen
  lv_display_t *disp = lv_display_get_default();
  if (disp) {
    lv_display_add_flag(disp, LV_DISPLAY_FLAG_READ_ONLY);
  }
*/
  // 3. Hardware-Befehl an den Controller senden (Sleep In)
  // Ersetze 'your_send_cmd' durch deine SPI-Schreibfunktion
  // your_send_cmd(0x10);
}
