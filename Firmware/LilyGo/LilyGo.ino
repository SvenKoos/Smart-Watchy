#ifndef ARDUINO_T_WATCH_S3
#error "HALT! Bitte im Menü das Board auf 'LilyGo T-Watch-S3' umstellen!"
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

RTC_DATA_ATTR String lastLocalIP = "";
RTC_DATA_ATTR String lastGatewayIP = "";
RTC_DATA_ATTR String lastMACAdress = "";

int watchType = DIGITAL_WATCH;

bool newAlertsIndicator = false;

uint8_t batteryCapacityHistory[1440];
uint16_t batteryVoltageHistory[1440];

uint16_t stepCounterHistory[1440];

lv_color_t color_bg;
lv_color_t color_text;

void setup() {
  Serial.begin(115200);

  // onetime action: reset battery calibration
  // resetBatteryCalibration();

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

  // setup location and weather data
  setupLocationData();
  setupWeatherData();

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
  timerEventWatchface();

  // start timer for minimal brightness
  timerEventBrightness(10);
}

void loop() {
  instance.loop();

  bool isPluggedIn = instance.pmu.isVbusIn();

  if ((guiState != DARK_STATE) || isPluggedIn) {
    lv_timer_handler();  // Verarbeitet die Timer
  } else {
    // light sleep mode test with timer
    esp_sleep_enable_timer_wakeup(1000 * 1000);
    esp_light_sleep_start();

    // ---------------------- Double Tap filter begin
    // --- HIER WACHT DIE UHR GERADE AUF (JEDE SEKUNDE ODER BEI REIZ) ---

    // Änderung 3: SCHÜTTEL-SCHUTZSCHILD
    // Prüfen, ob der BMA423-Sensor einen Hardware-Interrupt im Speicher hat
    if (instance.sensor.readIrqStatus()) {
      // Hat der Sensor das "Wakeup-Event" (Double-Tap) registriert?
      if (instance.sensor.isDoubleTap()) {
        // Änderung 2: Touch-Modul sofort wieder empfangsbereit machen
        instance.touch.wakeup();

        // Wir deklarieren drei einfache Variablen für die Rohdaten der Achsen
        int16_t xAcc = 0;
        int16_t yAcc = 0;
        int16_t zAcc = 0;

        // Aufruf der korrekten Funktion der Library
        instance.sensor.getAccelerometer(xAcc, yAcc, zAcc);
        // Mathematische Filterung: Wir addieren die Querkräfte der X- und Y-Achse.
        // Beim Klopfen aufs Glas (Z-Achse) bleiben X und Y relativ ruhig.
        // Beim Schütteln des Arms schlagen X oder Y extrem aus.
        long lateralMovement = abs(xAcc) + abs(yAcc);

        Serial.printf("[Sensor] Double-Tap detektiert. X/Y-Kräfte: %ld\n", lateralMovement);

        // threshold
        // Klopfe ganz normal auf das Display und schau, wie hoch der Wert steigt (sollte meist unter 200–300 bleiben)
        // Schüttle nun deinen Arm so, wie es im Alltag ungewollt passiert, und beobachte den Ausschlag (sollte weit über 500–800 schießen).
        // Passe die Zahl 450 im Code so an, dass sie genau zwischen deinen Klopf- und deinen Schüttelwerten liegt.
        if (lateralMovement > 450) {
          Serial.println("Schüttel-Schutz gegriffen: Unruhige Armbewegung blockiert.");

          // WICHTIG: Wir brechen den aktuellen loop()-Durchlauf hier ab (return).
          // guiState bleibt DARK_STATE, das Display physisch aus.
          // Die Uhr läuft im nächsten Durchlauf sofort wieder in den Schlaf.
          return;
        }

        // Wenn die Querkräfte unter dem Schwellenwert lagen -> Echtes Klopfen!
        // handle as in device event SENSOR_DOUBLE_TAP_DETECTED in case of guiState == DARK_STATE
        Serial.println("Echtes Klopfen erkannt! Wecke System...");
        guiState = WATCHFACE_STATE;
        currentAccelleration.isMoved = true;
        drawWatchFace();
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);
      }
    }
    // ---------------------- Double Tap filter end
  }

  // 2. Prüfen, ob der Hardware-Timer die Flag gesetzt hat
  if (update_gui_request) {
    Serial.println("loop update watchface");

    update_gui_request = false;  // Flag sofort zurücksetzen

    // get the data
    collectData();

    // verify power mgt setup
    verifyPowerMgt();

    if (guiState == WATCHFACE_STATE) {
      // draw watch face
      drawWatchFace();
    }
  }

  delay(10);
}
