#include "Watchy_7_SEG.h"

#define DARKMODE false

const uint8_t BATTERY_SEGMENT_WIDTH = 7;
const uint8_t BATTERY_SEGMENT_HEIGHT = 11;
const uint8_t BATTERY_SEGMENT_SPACING = 9;
const uint8_t WEATHER_ICON_WIDTH = 48;
const uint8_t WEATHER_ICON_HEIGHT = 32;

void Watchy7SEG::drawWatchFace(){
    // SvKo added
    locationData currentLocation = getLocation();
    accelData currentAccel = getAccel();

    display.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

    drawTime();
    drawDate();
    
    // SvKo changed
    bool move = drawSteps(currentAccel);
    if (move == true) {
      drawWeather(currentLocation);

      //SvKo added
      drawAlerts();

      display.drawBitmap(125, 75, WIFI_CONFIGURED ? wifi : wifioff, 26, 18, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }

    drawBattery();

    // SvKo Removed
/*
    if(BLE_CONFIGURED){
        display.drawBitmap(100, 75, bluetooth, 13, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }
*/
}

void Watchy7SEG::drawTime(){
    display.setFont(&DSEG7_Classic_Bold_53);
    display.setCursor(5, 53+5);
    int displayHour;
    if(HOUR_12_24==12){
      displayHour = ((currentTime.Hour+11)%12)+1;
    } else {
      displayHour = currentTime.Hour;
    }
    if(displayHour < 10){
        display.print("0");
    }
    display.print(displayHour);
    display.print(":");
    if(currentTime.Minute < 10){
        display.print("0");
    }
    display.println(currentTime.Minute);
}

void Watchy7SEG::drawDate(){
    display.setFont(&Seven_Segment10pt7b);

    int16_t  x1, y1;
    uint16_t w, h;

    String dayOfWeek = dayStr(currentTime.Wday);
    display.getTextBounds(dayOfWeek, 5, 85, &x1, &y1, &w, &h);
    if(currentTime.Wday == 4){
        w = w - 5;
    }
    display.setCursor(85 - w, 85);
    display.println(dayOfWeek);

    String month = monthShortStr(currentTime.Month);
    display.getTextBounds(month, 60, 110, &x1, &y1, &w, &h);
    display.setCursor(85 - w, 110);
    display.println(month);

    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 120);
    if(currentTime.Day < 10){
    display.print("0");
    }
    display.println(currentTime.Day);

    // SvKo: removed
    // display.setCursor(5, 150);
    // display.println(tmYearToCalendar(currentTime.Year));// offset from 1970, since year is stored in uint8_t
}

bool Watchy7SEG::drawSteps(accelData currentAccel){
    // reset step counter at midnight
    if (currentTime.Hour == 0 && currentTime.Minute == 0){
      sensor.resetStepCounter();
    }

    // SvKo changed
    // uint32_t stepCount = sensor.getCounter();
    uint32_t stepCount = currentAccel.stepCounter;
    display.drawBitmap(10, 165, steps, 19, 23, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // SvKo: added
    display.setFont(&DSEG7_Classic_Bold_25);
    
    display.setCursor(35, 190);
    display.println(stepCount);
    
    display.setCursor(5, 150);
    display.setFont(&Seven_Segment10pt7b);
		  
    // SvKo added
    if (currentAccel.move == false) {
      display.println("Quiet Mode");

      return false;
    } else {
      return true;
      // display.println(currentAccel.log);
    }
}

void Watchy7SEG::drawBattery(){
    display.drawBitmap(157, 73, battery, 37, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.fillRect(162, 78, 27, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);//clear battery segments
    int8_t batteryLevel = 0;
    float VBAT = getBatteryVoltage();

    if(VBAT > 4.1){
        batteryLevel = 3;
    }
    else if(VBAT > 3.95 && VBAT <= 4.1){
        batteryLevel = 2;
    }
    else if(VBAT > 3.80 && VBAT <= 3.95){
        batteryLevel = 1;
    }
    else if(VBAT <= 3.80){
        batteryLevel = 0;
    }

    for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
        display.fillRect(162 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }
}

void Watchy7SEG::drawWeather(locationData currentLocation){
    // SvKo changed
    weatherData currentWeather;

    // SvKo: added
    display.setCursor(5, 150);
    display.setFont(&Seven_Segment10pt7b);

    if (currentLocation.code == 200) {
      String name;
      int maxNameLength = 10;
      if (strlen(currentLocation.city) > maxNameLength)
      {
        name = String(currentLocation.city, maxNameLength - 1);
        name = name + String(".");
      }
      else
        name = currentLocation.city;
      display.println(name);

      currentWeather = getWeatherDataExt(currentLocation.latitude, currentLocation.longitude);
    }
    else {
      currentWeather = getWeatherData();
    }

    if (currentWeather.code == 200) {

      int8_t temperature = currentWeather.temperature;
      int16_t weatherConditionCode = currentWeather.weatherConditionCode;

      display.setFont(&DSEG7_Classic_Regular_39);
      int16_t  x1, y1;
      uint16_t w, h;
      display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
      if(159 - w - x1 > 87){
          display.setCursor(159 - w - x1, 150);
      } else {
          display.setFont(&DSEG7_Classic_Bold_25);
          display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
          display.setCursor(159 - w - x1, 136);
      }
      display.println(temperature);
      display.drawBitmap(165, 110, currentWeather.isMetric ? celsius : fahrenheit, 26, 20, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
      
      const unsigned char* weatherIcon;

      //https://openweathermap.org/weather-conditions
      if(weatherConditionCode > 801){//Cloudy
      weatherIcon = cloudy;
      }else if(weatherConditionCode == 801){//Few Clouds
      weatherIcon = cloudsun;
      }else if(weatherConditionCode == 800){//Clear
      weatherIcon = sunny;
      }else if(weatherConditionCode >=700){//Atmosphere
      weatherIcon = atmosphere;
      }else if(weatherConditionCode >=600){//Snow
      weatherIcon = snow;
      }else if(weatherConditionCode >=500){//Rain
      weatherIcon = rain;
      }else if(weatherConditionCode >=300){//Drizzle
      weatherIcon = drizzle;
      }else if(weatherConditionCode >=200){//Thunderstorm
      weatherIcon = thunderstorm;
      }else
      return;
      display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }
}

// SvKo: added
locationData Watchy7SEG::getLocation(){
    return getLocationData();
}

// SvKo: added
void Watchy7SEG::drawAlerts() {
  alertData currentAlerts;
  currentAlerts = getAlertData(DARKMODE);

  if (currentAlerts.code == 200) {
    display.setFont(&Seven_Segment10pt7b);
    int noAlerts = 0;
    noAlerts = currentAlerts.alerts["data"].length();
    if (noAlerts > 0) {
      // display.drawBitmap(7, 132, alert, 24, 24, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
      display.drawBitmap(95, 72, alert, 24, 24, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

      // display.setFont(&DSEG7_Classic_Bold_25);
      // display.setCursor(35, 155);
      // display.println(currentAlerts.log);
    } 
  }
}

// SvKo: added
accelData Watchy7SEG::getAccel(){
    return getAccelData();
}

