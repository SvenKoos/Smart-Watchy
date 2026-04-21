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

#include "syncNTP.h"

bool syncNTP(long gmt, const char *ntpServer) {
  struct tm hwTimeinfo;

  Serial.println("syncNTP Start");

  // hardware clock
  instance.rtc.getDateTime(&hwTimeinfo);
  Serial.print("syncNTP hardware clock:");
  Serial.println(&hwTimeinfo, "%A, %B %d %Y %H:%M:%S");

  Serial.print("syncNTP uses ntpServer ");
  Serial.println(ntpServer);
  configTime(gmt, 0, ntpServer);
  Serial.println("syncNTP synchronising time");

  // system clock
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("syncNTP No time available (yet)");
  } else {
    Serial.print("syncNTP system clock:");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
}

void setupNTPSync() {
  sntp_set_time_sync_notification_cb(timeavailable);
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
  Serial.println("syncNTP got time adjustment from NTP, write the hardware clock");

  // Write synchronization time to hardware
  instance.rtc.hwClockWrite();
}
