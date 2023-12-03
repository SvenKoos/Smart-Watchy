#include "Watchy.h"

WatchyRTC Watchy::RTC;
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> Watchy::display(
    GxEPD2_154_D67(DISPLAY_CS, DISPLAY_DC, DISPLAY_RES, DISPLAY_BUSY));

RTC_DATA_ATTR int guiState;
RTC_DATA_ATTR int menuIndex;
RTC_DATA_ATTR BMA423 sensor;
RTC_DATA_ATTR bool WIFI_CONFIGURED;
RTC_DATA_ATTR bool BLE_CONFIGURED;
RTC_DATA_ATTR weatherData currentWeather;
RTC_DATA_ATTR int weatherIntervalCounter = -1;
RTC_DATA_ATTR bool displayFullInit = true;

// SvKo added
RTC_DATA_ATTR locationData currentLocation;
RTC_DATA_ATTR int locationIntervalCounter = -1;
RTC_DATA_ATTR alertData currentAlerts;
RTC_DATA_ATTR int alertIndex = -1;
RTC_DATA_ATTR singleAlert allAlerts[ALERT_MAX_NO];
RTC_DATA_ATTR bool darkMode = false;

// SvKo added
RTC_DATA_ATTR accelData currentAccel;
RTC_DATA_ATTR bool quietMode = false;

void Watchy::init(String datetime) {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause(); // get wake up reason
  Wire.begin(SDA, SCL);                         // init i2c
  RTC.init();

  // Init the display here for all cases, if unused, it will do nothing
  display.epd2.selectSPI(SPI, SPISettings(20000000, MSBFIRST, SPI_MODE0)); // Set SPI to 20Mhz (default is 4Mhz)
  display.init(0, displayFullInit, 10,
               true); // 10ms by spec, and fast pulldown reset
  display.epd2.setBusyCallback(displayBusyCallback);

  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
    if (guiState == WATCHFACE_STATE) {
      RTC.read(currentTime);
      showWatchFace(true); // partial updates on tick
    }
    break;
  case ESP_SLEEP_WAKEUP_EXT1: // button Press
    handleButtonPress();
    break;
  default: // reset
    RTC.config(datetime);
    _bmaConfig();
    RTC.read(currentTime);
    showWatchFace(false); // full update on reset
    break;
  }
  deepSleep();
}

void Watchy::displayBusyCallback(const void *) {
  gpio_wakeup_enable((gpio_num_t)DISPLAY_BUSY, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
}

void Watchy::deepSleep() {
  display.hibernate();
  if (displayFullInit) // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  displayFullInit = false; // Notify not to init it again
  RTC.clearAlarm();        // resets the alarm flag in the RTC

  // Set GPIOs 0-39 to input to avoid power leaking out
  const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
  for (int i = 0; i < GPIO_NUM_MAX; i++) {
    if ((ignore >> i) & 0b1)
      continue;
    pinMode(i, INPUT);
  }
  esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN,
                               0); // enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(
      BTN_PIN_MASK,
      ESP_EXT1_WAKEUP_ANY_HIGH); // enable deep sleep wake on button press
  esp_deep_sleep_start();
}

void Watchy::handleButtonPress() {
  uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
  
  // Menu Button
  if (wakeupBit & MENU_BTN_MASK) {
	// SvKo
	quietMode = false;

    if (guiState ==
        WATCHFACE_STATE) { // enter menu state if coming from watch face
      showMenu(menuIndex, true);
    } else if (guiState ==
               MAIN_MENU_STATE) { // if already in menu, then select menu item
      switch (menuIndex) {
      case 0:
        showAbout();
        break;
      case 1:
        showBuzz();
        break;
      case 2:
        showAccelerometer();
        break;
      case 3:
        setTime();
        break;
      case 4:
        setupWifi();
        break;
      case 5:
        showUpdateFW();
        break;
      case 6:
        showSyncNTP();
        break;
  // SvKo added
      case 7:
        bondBLE();
        break;
      default:
        break;
      }
    } else if (guiState == FW_UPDATE_STATE) {
      updateFWBegin();
    }
  }
  // Back Button
  else if (wakeupBit & BACK_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // exit to watch face if already in menu
      RTC.read(currentTime);
      showWatchFace(true);
    } else if (guiState == APP_STATE) {
      showMenu(menuIndex, true); // exit to menu if already in app
    } else if (guiState == FW_UPDATE_STATE) {
      showMenu(menuIndex, true); // exit to menu if already in app
    } else if (guiState == ALERT_STATE) {
      RTC.read(currentTime);
      showWatchFace(true);		
	} else if (guiState == WATCHFACE_STATE) {
	  // SvKo
	  if (quietMode == true) {
		quietMode = false;
		RTC.read(currentTime);
		weatherIntervalCounter = -1;
		locationIntervalCounter = -1;
		showWatchFace(true);
	  }
      return;
    }
  }
  // Up Button
  else if (wakeupBit & UP_BTN_MASK) {
	// SvKo
	quietMode = false;

    if (guiState == MAIN_MENU_STATE) { // increment menu index
      menuIndex--;
      if (menuIndex < 0) {
        menuIndex = MENU_LENGTH - 1;
      }
      showMenu(menuIndex, true);
    } else if (guiState == WATCHFACE_STATE) {
		if (currentAlerts.count > 0) {
			alertIndex = currentAlerts.count - 1;
			showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
		}	
	  return;
    } else if (guiState == ALERT_STATE) {
		if (currentAlerts.count > 0) {
			if (alertIndex < currentAlerts.count - 1) {
				alertIndex++;
				showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
			}
		}	
      return;
    }
  }
  // Down Button
  else if (wakeupBit & DOWN_BTN_MASK) {
	// SvKo
	quietMode = false;

    if (guiState == MAIN_MENU_STATE) { // decrement menu index
      menuIndex++;
      if (menuIndex > MENU_LENGTH - 1) {
        menuIndex = 0;
      }
      showMenu(menuIndex, true);
    } else if (guiState == WATCHFACE_STATE) {
		if (currentAlerts.count > 0) {
			alertIndex = 0;
			showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
		}
      return;
    } else if (guiState == ALERT_STATE) {
		if (currentAlerts.count > 0) {
			if (alertIndex > 0) {
				alertIndex--;
				showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
			}
		}
      return;
    }
  }

  /***************** fast menu *****************/
  bool timeout     = false;
  long lastTimeout = millis();
  pinMode(MENU_BTN_PIN, INPUT);
  pinMode(BACK_BTN_PIN, INPUT);
  pinMode(UP_BTN_PIN, INPUT);
  pinMode(DOWN_BTN_PIN, INPUT);
  while (!timeout) {
    if (millis() - lastTimeout > 5000) {
      timeout = true;
    } else {
      if (digitalRead(MENU_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState ==
            MAIN_MENU_STATE) { // if already in menu, then select menu item
          switch (menuIndex) {
          case 0:
            showAbout();
            break;
          case 1:
            showBuzz();
            break;
          case 2:
            showAccelerometer();
            break;
          case 3:
            setTime();
            break;
          case 4:
            setupWifi();
            break;
          case 5:
            showUpdateFW();
            break;
          case 6:
            showSyncNTP();
            break;
  // SvKo added
          case 7:
            bondBLE();
            break;
          default:
            break;
          }
        } else if (guiState == FW_UPDATE_STATE) {
          updateFWBegin();
        }
      } else if (digitalRead(BACK_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState ==
            MAIN_MENU_STATE) { // exit to watch face if already in menu
          RTC.read(currentTime);
          showWatchFace(true);
          break; // leave loop
        } else if (guiState == APP_STATE) {
          showMenu(menuIndex, true); // exit to menu if already in app
        } else if (guiState == FW_UPDATE_STATE) {
          showMenu(menuIndex, true); // exit to menu if already in app
        } else if (guiState == ALERT_STATE) {
			RTC.read(currentTime);
			showWatchFace(true);		
		}
      } else if (digitalRead(UP_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) { // increment menu index
          menuIndex--;
          if (menuIndex < 0) {
            menuIndex = MENU_LENGTH - 1;
          }
          showFastMenu(menuIndex);
        } else if (guiState == WATCHFACE_STATE) {
			if (currentAlerts.count > 0) {
				alertIndex = currentAlerts.count - 1;
				showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
			}	
		} else if (guiState == ALERT_STATE) {
			if (currentAlerts.count > 0) {
				if (alertIndex < currentAlerts.count - 1) {
					alertIndex++;
					showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
				}
			}	
		}
      } else if (digitalRead(DOWN_BTN_PIN) == 1) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) { // decrement menu index
          menuIndex++;
          if (menuIndex > MENU_LENGTH - 1) {
            menuIndex = 0;
          }
          showFastMenu(menuIndex);
        } else if (guiState == WATCHFACE_STATE) {
			if (currentAlerts.count > 0) {
				alertIndex = 0;
				showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
			}
		} else if (guiState == ALERT_STATE) {
			if (currentAlerts.count > 0) {
				if (alertIndex > 0) {
					alertIndex--;
					showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
				}
			}
		}
      }
    }
  }
}

void Watchy::showMenu(byte menuIndex, bool partialRefresh) {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;
  int16_t yPos;

  const char *menuItems[] = {
      "About Watchy", "Vibrate Motor", "Show Accelerometer",
      "Set Time",     "Setup WiFi",    "Update Firmware",
      "Sync NTP",     "Bond BLE"};
  for (int i = 0; i < MENU_LENGTH; i++) {
    yPos = MENU_HEIGHT + (MENU_HEIGHT * i) - 5;
    display.setCursor(0, yPos);
    if (i == menuIndex) {
      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
      // display.fillRect(x1 - 1, y1 - 10, 200, h + 15, GxEPD_WHITE);
      // display.setTextColor(GxEPD_BLACK);
      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, darkMode ? GxEPD_WHITE : GxEPD_BLACK);
      display.setTextColor(darkMode ? GxEPD_BLACK : GxEPD_WHITE);

      display.println(menuItems[i]);
    } else {
      // display.setTextColor(GxEPD_WHITE);
      display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
      display.println(menuItems[i]);
    }
  }

  display.display(partialRefresh);

  guiState = MAIN_MENU_STATE;
}

void Watchy::showFastMenu(byte menuIndex) {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;
  int16_t yPos;

  const char *menuItems[] = {
      "About Watchy", "Vibrate Motor", "Show Accelerometer",
      "Set Time",     "Setup WiFi",    "Update Firmware",
      "Sync NTP",     "Bond BLE"};
  for (int i = 0; i < MENU_LENGTH; i++) {
    yPos = MENU_HEIGHT + (MENU_HEIGHT * i) - 5;
    display.setCursor(0, yPos);
    if (i == menuIndex) {
      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, darkMode ? GxEPD_WHITE : GxEPD_BLACK);
      display.setTextColor(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
      display.println(menuItems[i]);
    } else {
      display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
      display.println(menuItems[i]);
    }
  }

  display.display(true);

  guiState = MAIN_MENU_STATE;
}

void Watchy::showAbout() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(0, 20);

  display.print("LibVer: ");
  display.println(WATCHY_LIB_VER);

  const char *RTC_HW[3] = {"<UNKNOWN>", "DS3231", "PCF8563"};
  display.print("RTC: ");
  display.println(RTC_HW[RTC.rtcType]); // 0 = UNKNOWN, 1 = DS3231, 2 = PCF8563

  display.print("Batt: ");
  float voltage = getBatteryVoltage();
  display.print(voltage);
  display.println("V");
  
  String localIP;
  String gatewayIP;
  String macAdress;

  if (connectWiFi(localIP, gatewayIP, macAdress)) {
    display.println("");
	display.println("Local IP:"); display.println(localIP);
    display.println("Gateway IP:"); display.println(gatewayIP);
    display.println("MAC Address:"); display.println(macAdress);
	  
	WiFi.mode(WIFI_OFF);
	btStop();	  
  } else {
    display.println("WiFi Not Configured");
  }  

  display.display(true); // full refresh

  guiState = APP_STATE;
}

void Watchy::showBuzz() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(70, 80);
  display.println("Buzz!");
  display.display(true); // full refresh
  vibMotor();
  showMenu(menuIndex, true);
}

void Watchy::vibMotor(uint8_t intervalMs, uint8_t length) {
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  bool motorOn = false;
  for (int i = 0; i < length; i++) {
    motorOn = !motorOn;
    digitalWrite(VIB_MOTOR_PIN, motorOn);
    delay(intervalMs);
  }
}

void Watchy::setTime() {

  guiState = APP_STATE;

  RTC.read(currentTime);

  int8_t minute = currentTime.Minute;
  int8_t hour   = currentTime.Hour;
  int8_t day    = currentTime.Day;
  int8_t month  = currentTime.Month;
  int8_t year   = tmYearToY2k(currentTime.Year);

  int8_t setIndex = SET_HOUR;

  int8_t blink = 0;

  pinMode(DOWN_BTN_PIN, INPUT);
  pinMode(UP_BTN_PIN, INPUT);
  pinMode(MENU_BTN_PIN, INPUT);
  pinMode(BACK_BTN_PIN, INPUT);

  display.setFullWindow();

  while (1) {

    if (digitalRead(MENU_BTN_PIN) == 1) {
      setIndex++;
      if (setIndex > SET_DAY) {
        break;
      }
    }
    if (digitalRead(BACK_BTN_PIN) == 1) {
      if (setIndex != SET_HOUR) {
        setIndex--;
      }
    }

    blink = 1 - blink;

    if (digitalRead(DOWN_BTN_PIN) == 1) {
      blink = 1;
      switch (setIndex) {
      case SET_HOUR:
        hour == 23 ? (hour = 0) : hour++;
        break;
      case SET_MINUTE:
        minute == 59 ? (minute = 0) : minute++;
        break;
      case SET_YEAR:
        year == 99 ? (year = 0) : year++;
        break;
      case SET_MONTH:
        month == 12 ? (month = 1) : month++;
        break;
      case SET_DAY:
        day == 31 ? (day = 1) : day++;
        break;
      default:
        break;
      }
    }

    if (digitalRead(UP_BTN_PIN) == 1) {
      blink = 1;
      switch (setIndex) {
      case SET_HOUR:
        hour == 0 ? (hour = 23) : hour--;
        break;
      case SET_MINUTE:
        minute == 0 ? (minute = 59) : minute--;
        break;
      case SET_YEAR:
        year == 0 ? (year = 99) : year--;
        break;
      case SET_MONTH:
        month == 1 ? (month = 12) : month--;
        break;
      case SET_DAY:
        day == 1 ? (day = 31) : day--;
        break;
      default:
        break;
      }
    }

    display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
    display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
    display.setFont(&DSEG7_Classic_Bold_53);

    display.setCursor(5, 80);
    if (setIndex == SET_HOUR) { // blink hour digits
      display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
    }
    if (hour < 10) {
      display.print("0");
    }
    display.print(hour);

    display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
    display.print(":");

    display.setCursor(108, 80);
    if (setIndex == SET_MINUTE) { // blink minute digits
      display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
    }
    if (minute < 10) {
      display.print("0");
    }
    display.print(minute);

    display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(45, 150);
    if (setIndex == SET_YEAR) { // blink minute digits
      display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
    }
    display.print(2000 + year);

    display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
    display.print("/");

    if (setIndex == SET_MONTH) { // blink minute digits
      display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
    }
    if (month < 10) {
      display.print("0");
    }
    display.print(month);

    display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
    display.print("/");

    if (setIndex == SET_DAY) { // blink minute digits
      display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
    }
    if (day < 10) {
      display.print("0");
    }
    display.print(day);
    display.display(true); // partial refresh
  }

  tmElements_t tm;
  tm.Month  = month;
  tm.Day    = day;
  tm.Year   = y2kYearToTm(year);
  tm.Hour   = hour;
  tm.Minute = minute;
  tm.Second = 0;

  RTC.set(tm);

  showMenu(menuIndex, true);
}

void Watchy::showAccelerometer() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);

  Accel acc;

  long previousMillis = 0;
  long interval       = 200;

  guiState = APP_STATE;

  pinMode(BACK_BTN_PIN, INPUT);

  while (1) {

    unsigned long currentMillis = millis();

    if (digitalRead(BACK_BTN_PIN) == 1) {
      break;
    }

    if (currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;
      // Get acceleration data
      bool res          = sensor.getAccel(acc);
      uint8_t direction = sensor.getDirection();
      display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
      display.setCursor(0, 30);
      if (res == false) {
        display.println("getAccel FAIL");
      } else {
        display.print("  X:");
        display.println(acc.x);
        display.print("  Y:");
        display.println(acc.y);
        display.print("  Z:");
        display.println(acc.z);

        display.setCursor(30, 130);
        switch (direction) {
        case DIRECTION_DISP_DOWN:
          display.println("FACE DOWN");
          break;
        case DIRECTION_DISP_UP:
          display.println("FACE UP");
          break;
        case DIRECTION_BOTTOM_EDGE:
          display.println("BOTTOM EDGE");
          break;
        case DIRECTION_TOP_EDGE:
          display.println("TOP EDGE");
          break;
        case DIRECTION_RIGHT_EDGE:
          display.println("RIGHT EDGE");
          break;
        case DIRECTION_LEFT_EDGE:
          display.println("LEFT EDGE");
          break;
        default:
          display.println("ERROR!!!");
          break;
        }
      }
      display.display(true); // full refresh
    }
  }

  showMenu(menuIndex, true);
}

void Watchy::showWatchFace(bool partialRefresh) {
  display.setFullWindow();
  drawWatchFace();
  display.display(partialRefresh); // partial refresh
  guiState = WATCHFACE_STATE;
}

void Watchy::drawWatchFace() {
  display.setFont(&DSEG7_Classic_Bold_53);
  display.setCursor(5, 53 + 60);
  if (currentTime.Hour < 10) {
    display.print("0");
  }
  display.print(currentTime.Hour);
  display.print(":");
  if (currentTime.Minute < 10) {
    display.print("0");
  }
  display.println(currentTime.Minute);
}

weatherData Watchy::getWeatherData() {
  return getWeatherData(settings.cityID, settings.weatherUnit,
                        settings.weatherLang, settings.weatherURL,
                        settings.weatherAPIKey, settings.weatherUpdateInterval);
}

weatherData Watchy::getWeatherData(String cityID, String units, String lang, String url, String apiKey, uint8_t updateInterval) {
  currentWeather.isMetric = units == String("metric");
  // SvKo added
  currentWeather.code = CODE_NO_ERROR;
  
  if (weatherIntervalCounter < 0) { //-1 on first run, set to updateInterval
    weatherIntervalCounter = updateInterval;
  }
  if (weatherIntervalCounter >=
      updateInterval) { // only update if WEATHER_UPDATE_INTERVAL has elapsed
                        // i.e. 30 minutes
	String localIP;
	String gatewayIP;
	String macAdress;
    if (connectWiFi(localIP, gatewayIP, macAdress)) {
      HTTPClient http; // Use Weather API for live data if WiFi is connected
      http.setConnectTimeout(3000); // 3 second max timeout
	  // API documentation: https://openweathermap.org/current
      String weatherQueryURL = url + String("?id=") + cityID + String("&units=") + units +
                               String("&lang=") + lang + String("&appid=") +
                               apiKey;
      http.begin(weatherQueryURL.c_str());
      int httpResponseCode = http.GET();
      if (httpResponseCode == 200) {
        String payload             = http.getString();
        JSONVar responseObject     = JSON.parse(payload);
        currentWeather.temperature = int(responseObject["main"]["temp"]);
        currentWeather.weatherConditionCode =
            int(responseObject["weather"][0]["id"]);
        // SvKo
		strcpy(currentWeather.weatherDescription, (const char*)responseObject["weather"][0]["main"]);
        // sync NTP during weather API call and use timezone of city
        syncNTP(long(responseObject["timezone"]));
		// SvKo: added
		strcpy(currentWeather.name, (const char*)responseObject["name"]);
		String cityString = Normalize2ASCII(String(currentWeather.name));
		strcpy(currentWeather.name, cityString.c_str());
		
		currentWeather.offset = long(responseObject["timezone"]);		
      } else {
        // http error
	    // SvKo: added
	    currentWeather.code = CODE_HTTP_ERROR;
      }
      http.end();
      // turn off radios
	  // SvKo changed
	  WiFi.mode(WIFI_OFF);
	  btStop();
    } else { /* // No WiFi, use internal temperature sensor
      uint8_t temperature = sensor.readTemperature(); // celsius
      if (!currentWeather.isMetric) {
        temperature = temperature * 9. / 5. + 32.; // fahrenheit
      }
      currentWeather.temperature          = temperature; */ // SvKo commented out
	  currentWeather.code = CODE_COMM_ERROR;
      // SvKo changed
      currentWeather.weatherConditionCode = 0;
    }
    weatherIntervalCounter = 0;
  } else {
    weatherIntervalCounter++;
  }
  return currentWeather;
}

// SvKo added
weatherData Watchy::getWeatherDataExt(double latitude, double longitude) {
  return getWeatherDataByLocation(latitude, longitude, settings.weatherUnit,
								  settings.weatherLang, settings.weatherURL,
								  settings.weatherAPIKey, settings.weatherUpdateInterval);
}

weatherData Watchy::getWeatherDataByLocation(double latitude, double longitude, String units, String lang, String url, String apiKey, uint8_t updateInterval) {
  currentWeather.isMetric = units == String("metric");
  // SvKo added
  currentWeather.code = CODE_NO_ERROR;

  if (weatherIntervalCounter < 0) { //-1 on first run, set to updateInterval
    weatherIntervalCounter = updateInterval;
  }
  if (weatherIntervalCounter >=
      updateInterval) { // only update if WEATHER_UPDATE_INTERVAL has elapsed
                        // i.e. 30 minutes
	String localIP;
	String gatewayIP;
	String macAdress;
    if (connectWiFi(localIP, gatewayIP, macAdress)) {
      HTTPClient http; // Use Weather API for live data if WiFi is connected
      http.setConnectTimeout(3000); // 3 second max timeout
	  // API documentation: https://openweathermap.org/current
      String weatherQueryURL = url + String("?lat=") + String(latitude) + String("&lon=") + String(longitude) + String("&units=") + units +
                               String("&lang=") + lang + String("&appid=") +
                               apiKey;
      http.begin(weatherQueryURL.c_str());
      int httpResponseCode = http.GET();
      if (httpResponseCode == 200) {
        String payload             = http.getString();
        JSONVar responseObject     = JSON.parse(payload);
        currentWeather.temperature = int(responseObject["main"]["temp"]);
        currentWeather.weatherConditionCode =
            int(responseObject["weather"][0]["id"]);
        // SvKo
		strcpy(currentWeather.weatherDescription, (const char*)responseObject["weather"][0]["main"]);
		strcpy(currentWeather.name, (const char*)responseObject["name"]);
        // sync NTP during weather API call and use timezone of city
        syncNTP(long(responseObject["timezone"]));

		// SvKo: added
		currentWeather.offset = long(responseObject["timezone"]);
		
		// SvKo: added
		String cityString = Normalize2ASCII(String(currentWeather.name));
		strcpy(currentWeather.name, cityString.c_str());
      } else {
        // http error
	    // SvKo: added
	    currentWeather.code = CODE_HTTP_ERROR;
      }
      http.end();
      
	  // turn off radios
	  // SvKo changes
	  WiFi.mode(WIFI_OFF);
	  btStop();
    } else { /* // No WiFi, use internal temperature sensor
      uint8_t temperature = sensor.readTemperature(); // celsius
      if (!currentWeather.isMetric) {
        temperature = temperature * 9. / 5. + 32.; // fahrenheit
      }
      currentWeather.temperature = temperature; */ // SvKo commented out
	  currentWeather.code = CODE_COMM_ERROR;
      // SvKo changed
      currentWeather.weatherConditionCode = 0;
    }
    weatherIntervalCounter = 0;
  } else {
    weatherIntervalCounter++;
  }
  return currentWeather;
}

// SvKo: added
locationData Watchy::getLocationData() {
  return getLocationData(settings.geoipURL, settings.locationUpdateInterval);
}

locationData Watchy::getLocationData(String url, uint8_t updateInterval) {
  // SvKo added
  currentLocation.code = CODE_NO_ERROR;
  
  if (locationIntervalCounter < 0) { //-1 on first run, set to updateInterval
    locationIntervalCounter = updateInterval;
  }
  if (locationIntervalCounter >=
      updateInterval) { // only update if UPDATE_INTERVAL has elapsed
                        // i.e. 30 minutes
	String localIP;
	String gatewayIP;
	String macAdress;
    if (connectWiFi(localIP, gatewayIP, macAdress)) {
      HTTPClient http; // Use location API if WiFi is connected
      http.setConnectTimeout(3000); // 3 second max timeout
      String locationQueryURL = url;
      http.begin(locationQueryURL.c_str());
      int httpResponseCode = http.GET();
      if (httpResponseCode == 200) {
        String payload             = http.getString();
        JSONVar responseObject     = JSON.parse(payload);
		strcpy(currentLocation.publicIP, (const char*)responseObject["ip"]);
        strcpy(currentLocation.city, (const char*)responseObject["city"]);
        currentLocation.latitude = double(responseObject["latitude"]);
        currentLocation.longitude = double(responseObject["longitude"]);
		currentLocation.offset = long(responseObject["timezone"]["offset"]);
		strcpy(currentLocation.localIP, localIP.c_str());
		strcpy(currentLocation.gatewayIP, gatewayIP.c_str());
		
		String cityString = Normalize2ASCII(String(currentLocation.city));
		strcpy(currentLocation.city, cityString.c_str());
      } else {
        // http error
	    // SvKo: added
	    currentLocation.code = CODE_HTTP_ERROR;
      }
      http.end();
      // turn off radios
	  // SvKo changed
	  WiFi.mode(WIFI_OFF);
	  btStop();
    } else {
	  // SvKo added
	  currentLocation.code = CODE_COMM_ERROR;
	}
    locationIntervalCounter = 0;
  } else {
    locationIntervalCounter++;
  }
  return currentLocation;
}

float Watchy::getBatteryVoltage() {
  if (RTC.rtcType == DS3231) {
    return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * 2.0f; // Battery voltage goes through a 1/2 divider.
  } else {
    return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * 2.0f;
  }
}

uint16_t Watchy::_readRegister(uint8_t address, uint8_t reg, uint8_t *data,
                               uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)address, (uint8_t)len);
  uint8_t i = 0;
  while (Wire.available()) {
    data[i++] = Wire.read();
  }
  return 0;
}

uint16_t Watchy::_writeRegister(uint8_t address, uint8_t reg, uint8_t *data,
                                uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data, len);
  return (0 != Wire.endTransmission());
}

void Watchy::_bmaConfig() {

  if (sensor.begin(_readRegister, _writeRegister, delay) == false) {
    // fail to init BMA
    return;
  }

  // Accel parameter structure
  Acfg cfg;
  /*!
      Output data rate in Hz, Optional parameters:
          - BMA4_OUTPUT_DATA_RATE_0_78HZ
          - BMA4_OUTPUT_DATA_RATE_1_56HZ
          - BMA4_OUTPUT_DATA_RATE_3_12HZ
          - BMA4_OUTPUT_DATA_RATE_6_25HZ
          - BMA4_OUTPUT_DATA_RATE_12_5HZ
          - BMA4_OUTPUT_DATA_RATE_25HZ
          - BMA4_OUTPUT_DATA_RATE_50HZ
          - BMA4_OUTPUT_DATA_RATE_100HZ
          - BMA4_OUTPUT_DATA_RATE_200HZ
          - BMA4_OUTPUT_DATA_RATE_400HZ
          - BMA4_OUTPUT_DATA_RATE_800HZ
          - BMA4_OUTPUT_DATA_RATE_1600HZ
  */
  cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
  /*!
      G-range, Optional parameters:
          - BMA4_ACCEL_RANGE_2G
          - BMA4_ACCEL_RANGE_4G
          - BMA4_ACCEL_RANGE_8G
          - BMA4_ACCEL_RANGE_16G
  */
  cfg.range = BMA4_ACCEL_RANGE_2G;
  /*!
      Bandwidth parameter, determines filter configuration, Optional parameters:
          - BMA4_ACCEL_OSR4_AVG1
          - BMA4_ACCEL_OSR2_AVG2
          - BMA4_ACCEL_NORMAL_AVG4
          - BMA4_ACCEL_CIC_AVG8
          - BMA4_ACCEL_RES_AVG16
          - BMA4_ACCEL_RES_AVG32
          - BMA4_ACCEL_RES_AVG64
          - BMA4_ACCEL_RES_AVG128
  */
  cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

  /*! Filter performance mode , Optional parameters:
      - BMA4_CIC_AVG_MODE
      - BMA4_CONTINUOUS_MODE
  */
  cfg.perf_mode = BMA4_CONTINUOUS_MODE;

  // Configure the BMA423 accelerometer
  sensor.setAccelConfig(cfg);

  // Enable BMA423 accelerometer
  // Warning : Need to use feature, you must first enable the accelerometer
  // Warning : Need to use feature, you must first enable the accelerometer
  sensor.enableAccel();

  struct bma4_int_pin_config config;
  config.edge_ctrl = BMA4_LEVEL_TRIGGER;
  config.lvl       = BMA4_ACTIVE_HIGH;
  config.od        = BMA4_PUSH_PULL;
  config.output_en = BMA4_OUTPUT_ENABLE;
  config.input_en  = BMA4_INPUT_DISABLE;
  // The correct trigger interrupt needs to be configured as needed
  sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

  struct bma423_axes_remap remap_data;
  remap_data.x_axis      = 1;
  remap_data.x_axis_sign = 0xFF;
  remap_data.y_axis      = 0;
  remap_data.y_axis_sign = 0xFF;
  remap_data.z_axis      = 2;
  remap_data.z_axis_sign = 0xFF;
  // Need to raise the wrist function, need to set the correct axis
  sensor.setRemapAxes(&remap_data);

  // Enable BMA423 isStepCounter feature
  sensor.enableFeature(BMA423_STEP_CNTR, true);
  // Enable BMA423 isTilt feature
  sensor.enableFeature(BMA423_TILT, true);
  // Enable BMA423 isDoubleClick feature
  sensor.enableFeature(BMA423_WAKEUP, true);

  // Reset steps
  sensor.resetStepCounter();

  // Turn on feature interrupt
  sensor.enableStepCountInterrupt();
  sensor.enableTiltInterrupt();
  // It corresponds to isDoubleClick interrupt
  sensor.enableWakeupInterrupt();
}

void Watchy::setupWifi() {
  display.epd2.setBusyCallback(0); // temporarily disable lightsleep on busy
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  wifiManager.setTimeout(WIFI_AP_TIMEOUT);
  wifiManager.setAPCallback(_configModeCallback);
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  if (!wifiManager.autoConnect(WIFI_AP_SSID)) { // WiFi setup failed
    display.println("Setup failed &");
    display.println("timed out!");
  } else {
    display.println("Connected to");
    display.println(WiFi.SSID());
  }
  display.display(false); // full refresh
  // turn off radios
  WiFi.mode(WIFI_OFF);
  btStop();
  display.epd2.setBusyCallback(displayBusyCallback); // enable lightsleep on
                                                     // busy
  guiState = APP_STATE;
}

void Watchy::_configModeCallback(WiFiManager *myWiFiManager) {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Connect to");
  display.print("SSID: ");
  display.println(WIFI_AP_SSID);
  display.print("IP: ");
  display.println(WiFi.softAPIP());
  display.display(false); // full refresh
}

bool Watchy::connectWiFi(String &hostIP, String &gatewayIP, String &macAdress) {
  if (WL_CONNECT_FAILED ==
      WiFi.begin()) { // WiFi not setup, you can also use hard coded credentials
                      // with WiFi.begin(SSID,PASS);
    WIFI_CONFIGURED = false;
  } else {
    if (WL_CONNECTED == WiFi.waitForConnectResult()) { // attempt to connect for 10s
      WIFI_CONFIGURED = true;
	  // SvKo added
	  hostIP = WiFi.localIP().toString();
	  gatewayIP = WiFi.gatewayIP().toString();
	  macAdress = WiFi.macAddress();
    } else { // connection failed, time out
      WIFI_CONFIGURED = false;
      // turn off radios
      WiFi.mode(WIFI_OFF);
      btStop();
    }
  }
  return WIFI_CONFIGURED;
}

void Watchy::showUpdateFW() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Please visit");
  display.println("watchy.sqfmi.com");
  display.println("with a Bluetooth");
  display.println("enabled device");
  display.println(" ");
  display.println("Press menu button");
  display.println("again when ready");
  display.println(" ");
  display.println("Keep USB powered");
  display.display(false); // full refresh

  guiState = FW_UPDATE_STATE;
}

void Watchy::updateFWBegin() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Bluetooth Started");
  display.println(" ");
  display.println("Watchy BLE OTA");
  display.println(" ");
  display.println("Waiting for");
  display.println("connection...");
  display.display(false); // full refresh

  BLE_OTA BT;
  BT.begin("Watchy BLE OTA");
  int prevStatus = -1;
  int currentStatus;

  while (1) {
    currentStatus = BT.updateStatus();
    if (prevStatus != currentStatus || prevStatus == 1) {
      if (currentStatus == 0) {
        display.setFullWindow();
        display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("BLE Connected!");
        display.println(" ");
        display.println("Waiting for");
        display.println("upload...");
        display.display(false); // full refresh
      }
      if (currentStatus == 1) {
        display.setFullWindow();
        display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("Downloading");
        display.println("firmware:");
        display.println(" ");
        display.print(BT.howManyBytes());
        display.println(" bytes");
        display.display(true); // partial refresh
      }
      if (currentStatus == 2) {
        display.setFullWindow();
        display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("Download");
        display.println("completed!");
        display.println(" ");
        display.println("Rebooting...");
        display.display(false); // full refresh

        delay(2000);
        esp_restart();
      }
      if (currentStatus == 4) {
        display.setFullWindow();
        display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("BLE Disconnected!");
        display.println(" ");
        display.println("exiting...");
        display.display(false); // full refresh
        delay(1000);
        break;
      }
      prevStatus = currentStatus;
    }
    delay(100);
  }

  // turn off radios
  WiFi.mode(WIFI_OFF);
  btStop();
  showMenu(menuIndex, false);
}

void Watchy::showSyncNTP() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Syncing NTP... ");
  display.display(true); // full refresh

  String localIP;
  String gatewayIP;
  String macAdress;
  if (connectWiFi(localIP, gatewayIP, macAdress)) {
    if (syncNTP()) {
      display.println("NTP Sync Success\n");
      display.println("Current Time Is:");

      RTC.read(currentTime);

      display.print(tmYearToCalendar(currentTime.Year));
      display.print("/");
      display.print(currentTime.Month);
      display.print("/");
      display.print(currentTime.Day);
      display.print(" - ");

      if (currentTime.Hour < 10) {
        display.print("0");
      }
      display.print(currentTime.Hour);
      display.print(":");
      if (currentTime.Minute < 10) {
        display.print("0");
      }
      display.println(currentTime.Minute);
    } else {
      display.println("NTP Sync Failed");
    }
  } else {
    display.println("WiFi Not Configured");
  }
  display.display(true); // full refresh
  delay(3000);
  showMenu(menuIndex, true);
}

// SvKo added
void Watchy::bondBLE() {
  display.setFullWindow();
  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Bluetooth Started");
  display.println(" ");
  display.println("Watchy BLE");
  display.println(" ");
  display.println("Waiting for");
  display.println("connection...");
  display.display(true); 

  BLE_Bond BT;
  BT.begin("Watchy");
  int prevStatus = BOND_STATUS_UNDEFINED;
  int currentStatus;
  int count = 0;

  while (1) {
    currentStatus = BT.updateStatus();
    if (prevStatus != currentStatus) {
	  display.setFullWindow();
	  display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
	  display.setFont(&FreeMonoBold9pt7b);
	  display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
	  display.setCursor(0, 30);

      if (currentStatus == BOND_STATUS_CONNECTED) {
        display.println("BLE Connected!");
        display.println(" ");
        display.println("Waiting for");
        display.println("bonding...");
        display.display(true);
      } else
      if (currentStatus == BOND_STATUS_BONDED) {
        display.println("BLE Bonded!");
        display.println(" ");
        display.println("Waiting for");
        display.println("disconnect...");
        display.display(true); 
      } else
      if (currentStatus == BOND_STATUS_FAILED) {
        display.println("BLE Bonding");
        display.println("failed!");
        display.println("Exiting...");
        display.display(true); 
        delay(1000);
        break;
      } else
      if (currentStatus == BOND_STATUS_DISCONNECTED) {
        display.println("BLE Disconnected!");
        display.println(" ");
        display.println("Exiting...");
        display.display(true); 
        delay(1000);
        break;
      } else
      if (currentStatus == BOND_STATUS_GATTCONNECT) {
        display.println("BLE GATT");
        display.println("Connect!");
        display.display(true); 
      } else
      if (currentStatus == BOND_STATUS_GATTDISCONNECT) {
        display.println("BLE GATT");
        display.println("Disconnected!");
        display.println("Exiting...");
        display.display(true); 
        delay(1000);
        break;
      } else
      if (currentStatus == BOND_STATUS_SECURITYEVT) {
        display.println("BLE Security");
        display.println("Event!");
        display.display(true); 
      } else
      if (currentStatus == BOND_STATUS_AUTHCOMPLETE) {
        display.println("BLE Auth.");
        display.println("Complete!");
        display.display(true); 
      } else
      if (currentStatus == BOND_STATUS_UPDATECONNPARAMS) {
        display.println("BLE Update");
        display.println("Conn Params!");
        display.display(true); 
      } 
	  
      prevStatus = currentStatus;
    }

	count++;
	if (count > 300)	// stop after 30sec
		break;
    delay(100);
  }

  // turn off radios
  WiFi.mode(WIFI_OFF);
  btStop();
  showMenu(menuIndex, false);
}

bool Watchy::syncNTP() { // NTP sync - call after connecting to WiFi and
                         // remember to turn it back off
  return syncNTP(settings.gmtOffset,
                 settings.ntpServer.c_str());
}

bool Watchy::syncNTP(long gmt) {
  return syncNTP(gmt, settings.ntpServer.c_str());
}

bool Watchy::syncNTP(long gmt, String ntpServer) {
  // NTP sync - call after connecting to
  // WiFi and remember to turn it back off
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, ntpServer.c_str(), gmt);
  timeClient.begin();
  if (!timeClient.forceUpdate()) {
    return false; // NTP sync failed
  }
  tmElements_t tm;
  breakTime((time_t)timeClient.getEpochTime(), tm);
  RTC.set(tm);
  return true;
}

// SvKo
void Watchy::showAlert(singleAlert alert, int index, int amount) {
	guiState = ALERT_STATE;

	display.setFullWindow();
	display.fillScreen(darkMode ? GxEPD_BLACK : GxEPD_WHITE);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(darkMode ? GxEPD_WHITE : GxEPD_BLACK);
	display.setCursor(0, 20);

	String _string = (const char*)alert.timeStamp;
	display.println(_string.substring(0, 16));
	display.println(alert.appName);
	display.println(alert.title);
	display.println(alert.body);

	display.display(true); // full refresh
}

// SvKo added
alertData Watchy::getAlertData(bool _darkMode) {
	String localIP;
	String gatewayIP;
	String macAdress;
	// SvKo alerts
	JSONVar alerts;
	
	darkMode = _darkMode;
    
	// SvKo added
	currentAlerts.code = CODE_NO_ERROR;

	if (connectWiFi(localIP, gatewayIP, macAdress)) {
		HTTPClient http; // Use location API if WiFi is connected
		http.setConnectTimeout(3000); // 3 second max timeout
		String locationQueryURL = "http://" + gatewayIP + ":8080/alert?MAC=" + macAdress;
		http.begin(locationQueryURL.c_str());
		// SvKo added
		// http.addHeader("MAC", macAdress.c_str());
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
			// SvKo alerts
			// currentAlerts.alerts = JSON.parse(payload);
			alerts = JSON.parse(payload);
			alertIndex = -1;
			
			// SvKo alerts
			// currentAlerts.count = currentAlerts.alerts["data"].length();
			int alertNo = alerts["data"].length();
			if (alertNo > ALERT_MAX_NO) {
				alertNo = ALERT_MAX_NO;
			}
			// SvKo alerts
			currentAlerts.count = alertNo;
			
			for (int i = 0; i < currentAlerts.count; i++) {
				// SvKo alerts
				// JSONVar alert = currentAlerts.alerts["data"][i];
				JSONVar alert = alerts["data"][i];
				
				String _string = (const char*)alert["appName"];
				if (_string.length() < NAME_LEN)
					strcpy(allAlerts[i].appName, _string.c_str());
				else
					strcpy(allAlerts[i].appName, (_string.substring(0, NAME_LEN-1)).c_str());

				_string = (const char*)alert["title"];
				if (_string.length() < TITLE_LEN)
					strcpy(allAlerts[i].title, _string.c_str());
				else
					strcpy(allAlerts[i].title, (_string.substring(0, TITLE_LEN-1)).c_str());

				_string = (const char*)alert["body"];
				// SvKo
				// _string = Normalize2ASCII(_string);
				if ((_string != null) && (_string.length() > 0)) {
					if (_string.length() < BODY_LEN)
						strcpy(allAlerts[i].body, _string.c_str());
					else
						strcpy(allAlerts[i].body, (_string.substring(0, BODY_LEN-1)).c_str());
				} else {
					strcpy(allAlerts[i].body, "");
				}
				
				allAlerts[i].dismissed = false;
				
				_string = (const char*)(alert["timestamp"]);
				_string.replace("T", " ");
				int index = _string.lastIndexOf(".");
				_string = _string.substring(0, index);
				strcpy(allAlerts[i].timeStamp, _string.c_str());

				allAlerts[i].id = int(alert["id"]);
			}
			
			int newNo = currentAlerts.count;
			int newMin = 0;
			int newMax = 0;
			if  (newNo > 0) {
				newMin = allAlerts[0].id;
				newMax = allAlerts[newNo - 1].id;
				
				String log = String(newMin) + " " + String(newMax) + " " + String(oldMin) + " " + String(oldMax);
				strcpy(currentAlerts.log, log.c_str());

				if ((oldNo != newNo) || (oldMin != newMin) || (oldMax != newMax)) {
					float VBAT = getBatteryVoltage();
					if	(VBAT > 3.80) {
						vibMotor(200, 3);
					}
				}
			}
			
		} else {
			// http error
			// SvKo: added
			currentAlerts.code = CODE_HTTP_ERROR;
		}
		strcpy(currentAlerts.log, String(httpResponseCode).c_str());
		http.end();

		// turn off radios
		WiFi.mode(WIFI_OFF);
		btStop();
		
	} else {
		// SvKo: added
		currentAlerts.code = CODE_COMM_ERROR;
	}

	return currentAlerts;
}


accelData Watchy::getAccelData() {
  Accel acc;

  bool res = sensor.getAccel(acc);
  if (res == true) {
	  int16_t oldAccelX = currentAccel.accelX;
	  int16_t oldAccelY = currentAccel.accelY;
	  int16_t oldAccelZ = currentAccel.accelZ;

	  currentAccel.direction = sensor.getDirection();
	  currentAccel.accelX = acc.x;
	  currentAccel.accelY = acc.y;
	  currentAccel.accelZ = acc.z;
	  currentAccel.stepCounter = sensor.getCounter();
	  currentAccel.move = ((abs(oldAccelX - currentAccel.accelX) > MAX_ACCEL_QUIET) || (abs(oldAccelY - currentAccel.accelY) > MAX_ACCEL_QUIET) || (abs(oldAccelZ - currentAccel.accelZ) > MAX_ACCEL_QUIET));
	  
	  quietMode = !currentAccel.move;
	  
	  currentAccel.code = CODE_NO_ERROR;
  } else {
	  currentAccel.code = CODE_DATA_ERROR;
	  currentAccel.move = true;
  }
	  
	return currentAccel;
}

// SvKo
String Watchy::Unicode2ASCII(String source) {
	int loop = 0;
	int index;
	do {
		index = source.indexOf("\\u");
		if (index != -1) {
			loop++;
			switch (loop) {
			 case 1: source.replace("\\u00c0", "A"); break;
			 case 2: source.replace("\\u00c1", "A"); break;
			 case 3: source.replace("\\u00c2", "A"); break;
			 case 4: source.replace("\\u00c3", "A"); break;
			 case 5: source.replace("\\u00c4", "A"); break;
			 case 6: source.replace("\\u00c5", "A"); break;
			 case 7: source.replace("\\u00c6", "A"); break;
			 case 8: source.replace("\\u00c7", "C"); break;
			 case 9: source.replace("\\u00c8", "E"); break;
			 case 10: source.replace("\\u00c9", "E"); break;
			 case 11: source.replace("\\u00ca", "E"); break;
			 case 12: source.replace("\\u00cb", "E"); break;
			 case 13: source.replace("\\u00cc", "I"); break;
			 case 14: source.replace("\\u00cd", "I"); break;
			 case 15: source.replace("\\u00ce", "I"); break;
			 case 16: source.replace("\\u00cf", "I"); break;
			 case 17: source.replace("\\u00d0", "D"); break;
			 case 18: source.replace("\\u00d1", "N"); break;
			 case 19: source.replace("\\u00d2", "O"); break;
			 case 20: source.replace("\\u00d3", "O"); break;
			 case 21: source.replace("\\u00d4", "O"); break;
			 case 22: source.replace("\\u00d5", "O"); break;
			 case 23: source.replace("\\u00d6", "O"); break;
			 case 24: source.replace("\\u00d7", "x"); break;
			 case 25: source.replace("\\u00d8", "O"); break;
			 case 26: source.replace("\\u00d9", "U"); break;
			 case 27: source.replace("\\u00da", "U"); break;
			 case 28: source.replace("\\u00db", "U"); break;
			 case 29: source.replace("\\u00dc", "U"); break;
			 case 30: source.replace("\\u00dd", "Y"); break;
			 case 31: source.replace("\\u00df", "ss"); break;
			 case 32: source.replace("\\u00e0", "a"); break;
			 case 33: source.replace("\\u00e1", "a"); break;
			 case 34: source.replace("\\u00e2", "a"); break;
			 case 35: source.replace("\\u00e3", "a"); break;
			 case 36: source.replace("\\u00e4", "a"); break;
			 case 37: source.replace("\\u00e5", "a"); break;
			 case 38: source.replace("\\u00e6", "ae"); break;
			 case 39: source.replace("\\u00e7", "c"); break;
			 case 40: source.replace("\\u00e8", "e"); break;
			 case 41: source.replace("\\u00e9", "e"); break;
			 case 42: source.replace("\\u00ea", "e"); break;
			 case 43: source.replace("\\u00eb", "e"); break;
			 case 44: source.replace("\\u00ec", "i"); break;
			 case 45: source.replace("\\u00ed", "i"); break;
			 case 46: source.replace("\\u00ee", "i"); break;
			 case 47: source.replace("\\u00ef", "i"); break;
			 case 48: source.replace("\\u00f0", "o"); break;
			 case 49: source.replace("\\u00f1", "n"); break;
			 case 50: source.replace("\\u00f2", "o"); break;
			 case 51: source.replace("\\u00f3", "o"); break;
			 case 52: source.replace("\\u00f4", "o"); break;
			 case 53: source.replace("\\u00f5", "o"); break;
			 case 54: source.replace("\\u00f6", "o"); break;
			 case 55: source.replace("\\u00f8", "o"); break;
			 case 56: source.replace("\\u00f9", "u"); break;
			 case 57: source.replace("\\u00fa", "u"); break;
			 case 58: source.replace("\\u00fb", "u"); break;
			 case 59: source.replace("\\u00fc", "u"); break;
			 case 60: source.replace("\\u00fd", "y"); break;
			 case 61: source.replace("\\u00ff", "y"); break;
			 case 62: source.replace("\\u0100", "A"); break;
			 case 63: source.replace("\\u0101", "a"); break;
			 case 64: source.replace("\\u0102", "A"); break;
			 case 65: source.replace("\\u0103", "a"); break;
			 case 66: source.replace("\\u0104", "A"); break;
			 case 67: source.replace("\\u0105", "a"); break;
			 case 68: source.replace("\\u0106", "C"); break;
			 case 69: source.replace("\\u0107", "c"); break;
			 case 70: source.replace("\\u0108", "C"); break;
			 case 71: source.replace("\\u0109", "c"); break;
			 case 72: source.replace("\\u010a", "C"); break;
			 case 73: source.replace("\\u010b", "c"); break;
			 case 74: source.replace("\\u010c", "C"); break;
			 case 75: source.replace("\\u010d", "c"); break;
			 case 76: source.replace("\\u010e", "D"); break;
			 case 77: source.replace("\\u010f", "d"); break;
			 case 78: source.replace("\\u0110", "D"); break;
			 case 79: source.replace("\\u0111", "d"); break;
			 case 80: source.replace("\\u0112", "E"); break;
			 case 81: source.replace("\\u0113", "e"); break;
			 case 82: source.replace("\\u0114", "E"); break;
			 case 83: source.replace("\\u0115", "e"); break;
			 case 84: source.replace("\\u0116", "E"); break;
			 case 85: source.replace("\\u0117", "e"); break;
			 case 86: source.replace("\\u0118", "E"); break;
			 case 87: source.replace("\\u0119", "e"); break;
			 case 88: source.replace("\\u011a", "E"); break;
			 case 89: source.replace("\\u011b", "e"); break;
			 case 90: source.replace("\\u011c", "G"); break;
			 case 91: source.replace("\\u011d", "g"); break;
			 case 92: source.replace("\\u011e", "G"); break;
			 case 93: source.replace("\\u011f", "g"); break;
			 case 94: source.replace("\\u0120", "G"); break;
			 case 95: source.replace("\\u0121", "g"); break;
			 case 96: source.replace("\\u0122", "G"); break;
			 case 97: source.replace("\\u0123", "g"); break;
			 case 98: source.replace("\\u0124", "H"); break;
			 case 99: source.replace("\\u0125", "h"); break;
			 case 100: source.replace("\\u0126", "H"); break;
			 case 101: source.replace("\\u0127", "h"); break;
			 case 102: source.replace("\\u0128", "I"); break;
			 case 103: source.replace("\\u0129", "i"); break;
			 case 104: source.replace("\\u012a", "I"); break;
			 case 105: source.replace("\\u012b", "i"); break;
			 case 106: source.replace("\\u012c", "I"); break;
			 case 107: source.replace("\\u012d", "i"); break;
			 case 108: source.replace("\\u012e", "I"); break;
			 case 109: source.replace("\\u012f", "i"); break;
			 case 110: source.replace("\\u0130", "I"); break;
			 case 111: source.replace("\\u0131", "i"); break;
			 case 112: source.replace("\\u0132", "IJ"); break;
			 case 113: source.replace("\\u0133", "ij"); break;
			 case 114: source.replace("\\u0134", "J"); break;
			 case 115: source.replace("\\u0135", "j"); break;
			 case 116: source.replace("\\u0136", "K"); break;
			 case 117: source.replace("\\u0137", "k"); break;
			 case 118: source.replace("\\u0138", "k"); break;
			 case 119: source.replace("\\u0139", "L"); break;
			 case 120: source.replace("\\u013a", "l"); break;
			 case 121: source.replace("\\u013b", "L"); break;
			 case 122: source.replace("\\u013c", "l"); break;
			 case 123: source.replace("\\u013d", "L"); break;
			 case 124: source.replace("\\u013e", "l"); break;
			 case 125: source.replace("\\u013f", "L"); break;
			 case 126: source.replace("\\u0140", "l"); break;
			 case 127: source.replace("\\u0141", "L"); break;
			 case 128: source.replace("\\u0142", "l"); break;
			 case 129: source.replace("\\u0143", "N"); break;
			 case 130: source.replace("\\u0144", "n"); break;
			 case 131: source.replace("\\u0145", "N"); break;
			 case 132: source.replace("\\u0146", "n"); break;
			 case 133: source.replace("\\u0147", "N"); break;
			 case 134: source.replace("\\u0148", "n"); break;
			 case 135: source.replace("\\u014c", "O"); break;
			 case 136: source.replace("\\u014d", "o"); break;
			 case 137: source.replace("\\u014e", "O"); break;
			 case 138: source.replace("\\u014f", "o"); break;
			 case 139: source.replace("\\u0150", "O"); break;
			 case 140: source.replace("\\u0151", "o"); break;
			 case 141: source.replace("\\u0152", "OE"); break;
			 case 142: source.replace("\\u0153", "oe"); break;
			 case 143: source.replace("\\u0154", "R"); break;
			 case 144: source.replace("\\u0155", "r"); break;
			 case 145: source.replace("\\u0156", "R"); break;
			 case 146: source.replace("\\u0157", "r"); break;
			 case 147: source.replace("\\u0158", "R"); break;
			 case 148: source.replace("\\u0159", "r"); break;
			 case 149: source.replace("\\u015a", "S"); break;
			 case 150: source.replace("\\u015b", "s"); break;
			 case 151: source.replace("\\u015c", "S"); break;
			 case 152: source.replace("\\u015d", "s"); break;
			 case 153: source.replace("\\u015e", "S"); break;
			 case 154: source.replace("\\u015f", "s"); break;
			 case 155: source.replace("\\u0160", "S"); break;
			 case 156: source.replace("\\u0161", "s"); break;
			 case 157: source.replace("\\u0162", "T"); break;
			 case 158: source.replace("\\u0163", "t"); break;
			 case 159: source.replace("\\u0164", "T"); break;
			 case 160: source.replace("\\u0165", "t"); break;
			 case 161: source.replace("\\u0166", "T"); break;
			 case 162: source.replace("\\u0167", "t"); break;
			 case 163: source.replace("\\u0168", "U"); break;
			 case 164: source.replace("\\u0169", "u"); break;
			 case 165: source.replace("\\u016a", "U"); break;
			 case 166: source.replace("\\u016b", "u"); break;
			 case 167: source.replace("\\u016c", "U"); break;
			 case 168: source.replace("\\u016d", "u"); break;
			 case 169: source.replace("\\u016e", "U"); break;
			 case 170: source.replace("\\u016f", "u"); break;
			 case 171: source.replace("\\u0170", "U"); break;
			 case 172: source.replace("\\u0171", "u"); break;
			 case 173: source.replace("\\u0172", "U"); break;
			 case 174: source.replace("\\u0173", "u"); break;
			 case 175: source.replace("\\u0174", "W"); break;
			 case 176: source.replace("\\u0175", "w"); break;
			 case 177: source.replace("\\u0176", "Y"); break;
			 case 178: source.replace("\\u0177", "y"); break;
			 case 179: source.replace("\\u0178", "Y"); break;
			 case 180: source.replace("\\u0179", "Z"); break;
			 case 181: source.replace("\\u017a", "z"); break;
			 case 182: source.replace("\\u017b", "Z"); break;
			 case 183: source.replace("\\u017c", "z"); break;
			 case 184: source.replace("\\u017d", "Z"); break;
			 case 185: source.replace("\\u017e", "z"); break;
			}
		}
	} while (index != -1);
	return source;
}

// SvKo
String Watchy::Normalize2ASCII(String source) {
	source.replace('À', 'A');
	source.replace('Á', 'A');
	source.replace('Â', 'A');
	source.replace('Ã', 'A');
	source.replace("Ä", "Ae");
	source.replace('Å', 'A');
	source.replace('Æ', 'A');
	source.replace('Ç', 'C');
	source.replace('È', 'E');
	source.replace('É', 'E');
	source.replace('Ê', 'E');
	source.replace('Ë', 'E');
	source.replace('Ì', 'I');
	source.replace('Í', 'I');
	source.replace('Î', 'I');
	source.replace('Ï', 'I');
	source.replace('Ð', 'D');
	source.replace('Ñ', 'N');
	source.replace('Ò', 'O');
	source.replace('Ó', 'O');
	source.replace('Ô', 'O');
	source.replace('Õ', 'O');
	source.replace("Ö", "Oe");
	source.replace('×', 'x');
	source.replace('Ø', 'O');
	source.replace('Ù', 'U');
	source.replace('Ú', 'U');
	source.replace('Û', 'U');
	source.replace("Ü", "Ue");
	source.replace('Ý', 'Y');
	source.replace("ß", "ss");
	source.replace('à', 'a');
	source.replace('á', 'a');
	source.replace('â', 'a');
	source.replace('ã', 'a');
	source.replace("ä", "ae");
	source.replace('å', 'a');
	source.replace("æ", "ae");
	source.replace('ç', 'c');
	source.replace('è', 'e');
	source.replace('é', 'e');
	source.replace('ê', 'e');
	source.replace('ë', 'e');
	source.replace('ì', 'i');
	source.replace('í', 'i');
	source.replace('î', 'i');
	source.replace('ï', 'i');
	source.replace('ð', 'o');
	source.replace('ñ', 'n');
	source.replace('ò', 'o');
	source.replace('ó', 'o');
	source.replace('ô', 'o');
	source.replace('õ', 'o');
	source.replace("ö", "oe");
	source.replace('ø', 'o');
	source.replace('ù', 'u');
	source.replace('ú', 'u');
	source.replace('û', 'u');
	source.replace("ü", "ue");
	source.replace('ý', 'y');
	source.replace('ÿ', 'y');
	source.replace('Ā', 'A');
	source.replace('ā', 'a');
	source.replace('Ă', 'A');
	source.replace('ă', 'a');
	source.replace('Ą', 'A');
	source.replace('ą', 'a');
	source.replace('Ć', 'C');
	source.replace('ć', 'c');
	source.replace('Ĉ', 'C');
	source.replace('ĉ', 'c');
	source.replace('Ċ', 'C');
	source.replace('ċ', 'c');
	source.replace('Č', 'C');
	source.replace('č', 'c');
	source.replace('Ď', 'D');
	source.replace('ď', 'd');
	source.replace('Đ', 'D');
	source.replace('đ', 'd');
	source.replace('Ē', 'E');
	source.replace('ē', 'e');
	source.replace('Ĕ', 'E');
	source.replace('ĕ', 'e');
	source.replace('Ė', 'E');
	source.replace('ė', 'e');
	source.replace('Ę', 'E');
	source.replace('ę', 'e');
	source.replace('Ě', 'E');
	source.replace('ě', 'e');
	source.replace('Ĝ', 'G');
	source.replace('ĝ', 'g');
	source.replace('Ğ', 'G');
	source.replace('ğ', 'g');
	source.replace('Ġ', 'G');
	source.replace('ġ', 'g');
	source.replace('Ģ', 'G');
	source.replace('ģ', 'g');
	source.replace('Ĥ', 'H');
	source.replace('ĥ', 'h');
	source.replace('Ħ', 'H');
	source.replace('ħ', 'h');
	source.replace('Ĩ', 'I');
	source.replace('ĩ', 'i');
	source.replace('Ī', 'I');
	source.replace('ī', 'i');
	source.replace('Ĭ', 'I');
	source.replace('ĭ', 'i');
	source.replace('Į', 'I');
	source.replace('į', 'i');
	source.replace('İ', 'I');
	source.replace('ı', 'i');
	source.replace("Ĳ", "IJ");
	source.replace("ĳ", "ij");
	source.replace('Ĵ', 'J');
	source.replace('ĵ', 'j');
	source.replace('Ķ', 'K');
	source.replace('ķ', 'k');
	source.replace('ĸ', 'k');
	source.replace('Ĺ', 'L');
	source.replace('ĺ', 'l');
	source.replace('Ļ', 'L');
	source.replace('ļ', 'l');
	source.replace('Ľ', 'L');
	source.replace('ľ', 'l');
	source.replace('Ŀ', 'L');
	source.replace('ŀ', 'l');
	source.replace('Ł', 'L');
	source.replace('ł', 'l');
	source.replace('Ń', 'N');
	source.replace('ń', 'n');
	source.replace('Ņ', 'N');
	source.replace('ņ', 'n');
	source.replace('Ň', 'N');
	source.replace('ň', 'n');
	source.replace('Ō', 'O');
	source.replace('ō', 'o');
	source.replace('Ŏ', 'O');
	source.replace('ŏ', 'o');
	source.replace('Ő', 'O');
	source.replace('ő', 'o');
	source.replace("Œ", "OE");
	source.replace("œ", "oe");
	source.replace('Ŕ', 'R');
	source.replace('ŕ', 'r');
	source.replace('Ŗ', 'R');
	source.replace('ŗ', 'r');
	source.replace('Ř', 'R');
	source.replace('ř', 'r');
	source.replace('Ś', 'S');
	source.replace('ś', 's');
	source.replace('Ŝ', 'S');
	source.replace('ŝ', 's');
	source.replace('Ş', 'S');
	source.replace('ş', 's');
	source.replace('Š', 'S');
	source.replace('š', 's');
	source.replace('Ţ', 'T');
	source.replace('ţ', 't');
	source.replace('Ť', 'T');
	source.replace('ť', 't');
	source.replace('Ŧ', 'T');
	source.replace('ŧ', 't');
	source.replace('Ũ', 'U');
	source.replace('ũ', 'u');
	source.replace('Ū', 'U');
	source.replace('ū', 'u');
	source.replace('Ŭ', 'U');
	source.replace('ŭ', 'u');
	source.replace('Ů', 'U');
	source.replace('ů', 'u');
	source.replace('Ű', 'U');
	source.replace('ű', 'u');
	source.replace('Ų', 'U');
	source.replace('ų', 'u');
	source.replace('Ŵ', 'W');
	source.replace('ŵ', 'w');
	source.replace('Ŷ', 'Y');
	source.replace('ŷ', 'y');
	source.replace('Ÿ', 'Y');
	source.replace('Ź', 'Z');
	source.replace('ź', 'z');
	source.replace('Ż', 'Z');
	source.replace('ż', 'z');
	source.replace('Ž', 'Z');
	source.replace('ž', 'z');
	
	return source;
}