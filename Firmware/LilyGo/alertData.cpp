#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>
#include <Time.h>
#include <TimeLib.h>
#include <esp_sntp.h>

#include "dataCollection.h"
#include "alertData.h"
#include "config.h"
#include "settings.h"
#include "powerData.h"

extern alertData currentAlerts;
extern singleAlert allAlerts[ALERT_MAX_NO];

extern powerData currentPower;

alertData getAlertData(const String gatewayIP, const String macAdress) {
  JSONVar alerts;

  Serial.println("getAlertData Start");

  currentAlerts.code = CODE_NO_ERROR;
  HTTPClient http;               // Use location API if WiFi is connected
  http.setConnectTimeout(3000);  // 3 second max timeout
  String locationQueryURL = "http://" + gatewayIP + ":8080/alert?MAC=" + macAdress;
  http.begin(locationQueryURL.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    int oldNo = currentAlerts.count;
    int oldMin = 0;
    int oldMax = 0;
    if (oldNo > 0) {
      oldMin = allAlerts[0].id;
      oldMax = allAlerts[oldNo - 1].id;
    }

    String payload = http.getString();
    if (payload.length() > 8192) {
      currentAlerts.code = CODE_PARSE_ERROR;
      strncpy(currentAlerts.log, "Invalid payload", LOG_LEN - 1);
      return currentAlerts;
    }

    alerts = JSON.parse(payload);
    if (!alerts.hasOwnProperty("data") || JSON.typeof(alerts["data"]) != "array") {
      currentAlerts.code = CODE_PARSE_ERROR;
      return currentAlerts;
    }

    int alertNo = alerts["data"].length();
    if (alertNo > ALERT_MAX_NO) {
      alertNo = ALERT_MAX_NO;
    }
    currentAlerts.count = alertNo;

    for (int i = 0; i < currentAlerts.count; i++) {
      JSONVar alert = alerts["data"][i];

      String _string = (const char *)alert["appName"];
      strncpy(allAlerts[i].appName, _string.c_str(), NAME_LEN - 1);
      allAlerts[i].appName[NAME_LEN - 1] = '\0';

      _string = (const char *)alert["title"];
      if (_string == "null")
        _string = "";
      strncpy(allAlerts[i].title, _string.c_str(), TITLE_LEN - 1);
      allAlerts[i].title[TITLE_LEN - 1] = '\0';

      _string = (const char *)alert["body"];
      if (_string == nullptr) _string = "";

      if ((_string != null) && (_string.length() > 0)) {
        if (_string == "null")
          _string = "";
        strncpy(allAlerts[i].body, _string.c_str(), BODY_LEN - 1);
        allAlerts[i].body[BODY_LEN - 1] = '\0';
      } else {
        strcpy(allAlerts[i].body, "");
      }

      _string = (const char *)alert["dismissed"];
      if (_string == "false") {
        allAlerts[i].dismissed = false;
      } else
        allAlerts[i].dismissed = true;

      _string = (const char *)(alert["timestamp"]);
      _string.replace("T", " ");
      int index = _string.lastIndexOf(".");
      if (index < 0) index = _string.length();
      _string = _string.substring(0, index);
      strncpy(allAlerts[i].timeStamp, _string.c_str(), sizeof(allAlerts[i].timeStamp) - 1);
      allAlerts[i].timeStamp[sizeof(allAlerts[i].timeStamp) - 1] = '\0';

      allAlerts[i].id = int(alert["id"]);
    }

    int newNo = currentAlerts.count;
    int newMin = 0;
    int newMax = 0;
    if (newNo > 0) {
      newMin = allAlerts[0].id;
      newMax = allAlerts[newNo - 1].id;

      String log = String(newMin) + " " + String(newMax) + " " + String(oldMin) + " " + String(oldMax);
      strncpy(currentAlerts.log, log.c_str(), sizeof(currentAlerts.log) - 1);
      currentAlerts.log[sizeof(currentAlerts.log) - 1] = '\0';

      if ((oldNo != newNo) || (oldMin != newMin) || (oldMax != newMax)) {
        if (currentPower.batteryPercent > 10) {
          vibMotor();
        }
      }
    }

    Serial.print("getAlertData No. of alerts: "); Serial.println(currentAlerts.count, DEC);
  } else {
    // http error
    currentAlerts.code = CODE_HTTP_ERROR;

    Serial.print("getAlertData Error code: "); Serial.println(currentAlerts.code, DEC);
  }
  strncpy(currentAlerts.log, String(httpResponseCode).c_str(), sizeof(currentAlerts.log) - 1);
  currentAlerts.log[sizeof(currentAlerts.log) - 1] = '\0';

  http.end();
  return currentAlerts;
}

void vibMotor() {
  instance.drv.setWaveform(0, 10);  // play effect: 0...10
  instance.drv.setWaveform(1, 0);   // end waveform ?
  // play the effect
  instance.drv.run();
}

void showAlert(singleAlert alert, lv_obj_t *screen, lv_style_t style) {
  // timestamp
  lv_obj_t *labelTimestamp = lv_label_create(screen);
  lv_obj_add_style(labelTimestamp, &style, LV_PART_MAIN);
  lv_label_set_text_fmt(labelTimestamp, "%.16s", alert.timeStamp);
  lv_obj_align(labelTimestamp, LV_ALIGN_TOP_LEFT, 5, 5);

  // app
  lv_obj_t *labelApp = lv_label_create(screen);
  lv_obj_add_style(labelApp, &style, LV_PART_MAIN);
  lv_label_set_text(labelApp, alert.appName);
  lv_obj_align(labelApp, LV_ALIGN_TOP_LEFT, 5, 20);

  // title
  lv_obj_t *labelTitle = lv_label_create(screen);
  lv_obj_add_style(labelTitle, &style, LV_PART_MAIN);
  lv_label_set_text(labelTitle, alert.title);
  lv_obj_align(labelTitle, LV_ALIGN_TOP_LEFT, 5, 5);

  // body
  lv_obj_t *labelBody = lv_label_create(screen);
  lv_obj_add_style(labelBody, &style, LV_PART_MAIN);
  lv_label_set_text(labelBody, alert.body);
  lv_obj_align(labelBody, LV_ALIGN_TOP_LEFT, 5, 5);
}
