#ifndef ARDUINO_T_WATCH_S3
#error "Halt! Du hast vergessen, das Board auf T-Watch S3 umzustellen!"
#endif

#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "dataCollection.h"
#include "alertData.h"
#include "deviceEvent.h"
#include "accellData.h"
#include "settings.h"
#include "locationData.h"
#include "weatherData.h"
#include "syncNTP.h"
#include "powerData.h"
#include "ble.h"
#include "config.h"
#include "timerEvent.h"
#include "TOTP.h"
#include "lora.h"
#include "deviceEvent.h"
#include "menuHandler.h"

RTC_DATA_ATTR bool update_gui_request;

RTC_DATA_ATTR uint32_t stepCounter;

RTC_DATA_ATTR locationData currentLocation;

RTC_DATA_ATTR alertData currentAlerts;
RTC_DATA_ATTR singleAlert allAlerts[ALERT_MAX_NO];

RTC_DATA_ATTR accellData currentAccelleration;

RTC_DATA_ATTR bool WIFI_CONFIGURED = false;
RTC_DATA_ATTR bool WIFI_CONNECTED = false;

RTC_DATA_ATTR int guiState = -1;

RTC_DATA_ATTR lilygoSettings settings;

RTC_DATA_ATTR weatherData currentWeather;

RTC_DATA_ATTR powerData currentPower;

RTC_DATA_ATTR bool bleBonded = false;

RTC_DATA_ATTR uint8_t binKey[32];
RTC_DATA_ATTR int keyLen = 0;

// Diese Variablen überleben Sleeps und Resets im speziellen RTC-SRAM
RTC_DATA_ATTR int last_wakeup_cause = 0;
RTC_DATA_ATTR uint64_t last_ext1_status = 0;

uint8_t batteryHistory[1440];

uint16_t stepCounterHistory[1440];

void setup() {
  Serial.begin(115200);

  instance.begin();
  Serial.println("setup Instance started");

  beginLvglHelper(instance);
  Serial.println("setup LVGL helper started");

  // take settings
  settings = setSetting();
  Serial.println("setup Settings loaded");

  // Accelerator
  setupAccellData();
  Serial.println("setup Accelerometer");

  // Register power event
  // Register sensor event
  setupDeviceEvent();
  Serial.println("setup Power events registered");

  // setup power management
  setupPowerMgt();
  Serial.println("setup power management");

  // setup data collection
  setupDataCollection();

  // setup TOTP
  setupTOTP(settings.totpSecret);

  // setup Lora
  setupLora();

  // Set brightness to MAX
  // T-LoRa-Pager brightness level is 0 ~ 16
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  displayWakup();
  Serial.println("setup Brightness set to Max");

  // initial operation:
  // get the data initially
  Serial.println("setup Collect data");
  collectData();

  // GUI state
  guiState = WATCHFACE_STATE;
  // draw the watch face initially
  watchFaceSetup();
  drawWatchFace();

  // start timer for watch face - Problem
  update_gui_request = false;
  // timerEventWatchface();

  // start timer for minimal brightness
  timerEventBrightness(10);
}

void loop() {
  // 1. Haupt-Routine der LilyGo-Library
  instance.loop();

  bool isPluggedIn = instance.pmu.isVbusIn();

  // STRENGER SOFTWARE-TIMER (Sorgt für das Update im wachen Zustand)
  static unsigned long lastDataUpdate = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastDataUpdate >= 60000) {
    lastDataUpdate = currentMillis;
    update_gui_request = true; 
  }

  if ((guiState != DARK_STATE) || isPluggedIn) {
    // =================================================================
    // --- INTERAKTIVER MODUS (Uhr ist wach oder lädt) ---
    // =================================================================

    // SELEKTIVES HARDWARE-POLLING (NUR POWER BUTTON VIA PMU)
    static unsigned long lastPmuCheck = 0;
    if (millis() - lastPmuCheck > 20) {
      lastPmuCheck = millis();
      instance.pmu.getIrqStatus(); 

      if (instance.pmu.isPekeyShortPressIrq()) {
        Serial.println("POLLING: Power Button Klick erkannt!");
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        if (guiState == WATCHFACE_STATE) {
          guiState = MENU_STATE;
          menuHandler();
        }
      }
      instance.pmu.clearIrqStatus();
    }

    lv_timer_handler();  
    delay(5);

  } else {
    // =================================================================
    // --- SCHLAF-BLOCK (Uhr geht in den Light Sleep) ---
    // =================================================================
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // 1. Licht ausschalten (Logikspannung ALDO2 bleibt für Sensorik an!)
    displayGoToSleep();

    // 2. DYNAMISCHE ZEITBERECHNUNG FÜR DEN INTERNEN WEKER
    unsigned long timeSpentInCurrentMinute = millis() - lastDataUpdate;
    long timeLeftInMinute = 60000 - timeSpentInCurrentMinute;

    // Schutznetz gegen negative Zeiten oder Dauerschleifen
    if (timeLeftInMinute < 5000 || timeLeftInMinute > 60000) {
      timeLeftInMinute = 60000; 
    }

    // Internen CPU-Timer auf die verbleibenden Mikrosekunden einstellen
    uint64_t wakeup_time_us = (uint64_t)timeLeftInMinute * 1000ULL;
    esp_sleep_enable_timer_wakeup(wakeup_time_us);

    // 3. HARDWARE-INTERRUPT FÜR DEN DOUBLE-TAP SCHARFSCHALTEN
    uint64_t wakeup_pin_mask = (1ULL << PMU_INT);
    esp_sleep_enable_ext1_wakeup(wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_LOW);

    instance.touch.sleep(); 

    // PMU-Register vor dem Schlafen leeren, um Pegel-Zappeln zu verhindern
    for (int i = 0; i < 3; i++) {
      instance.pmu.getIrqStatus();
      instance.pmu.clearIrqStatus();
      delay(10);
    }

    // CPU SCHLÄFT EIN (Light Sleep startet hier)
    esp_light_sleep_start();

    // =================================================================
    // --- HIER WACHT DIE UHR GERADE AUF ---
    // =================================================================
    delay(60); 
    pinMode(PMU_INT, INPUT_PULLUP);
    
    // Ursachen-Check: Warum sind wir aufgewacht?
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
      // Fall A: Reines Hintergrund-Update (Minute um) -> Display bleibt AUS
      lastDataUpdate = millis(); 
      update_gui_request = true; 
    } 
    else if (cause == ESP_SLEEP_WAKEUP_EXT1) {
      // Fall B: User hat geklopft (Double-Tap) -> Erst JETZT Licht an!
      displayWakup();
    }

    instance.touch.wakeup(); 
    delay(30);

    // Register direkt wieder leeren, um den Aufwach-Impuls zu quittieren
    instance.pmu.getIrqStatus();
    instance.pmu.clearIrqStatus();
    delay(5);
  }

  // =================================================================
  // --- DATEN-UPDATE-BLOCK (Führt HTTP-Abruf aus) ---
  // =================================================================
  if (update_gui_request) {
    update_gui_request = false; 
    collectData(); // Startet deinen (teilweise blockierenden) Netzwerk-Call
    
    if (guiState == WATCHFACE_STATE) {
      drawWatchFace(); // Zeichnet das Chart neu
    }
  }
}
