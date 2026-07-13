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
#include "locationData.h"
#include "weatherData.h"
#include "config.h"
#include "settings.h"
#include "syncNTP.h"

extern lilygoSettings settings;

extern weatherData currentWeather;
extern locationData currentLocation;

weatherData getWeatherDataByLocation(double latitude, double longitude, String units, String lang, String url, String apiKey) {
	Serial.println("getWeatherDataByLocation Start");

	currentWeather.isMetric = units == String("metric");
	currentWeather.code = CODE_NO_ERROR;

	Serial.println("getWeatherDataByLocation Get");

	currentWeather.weatherConditionCode = 0;

	HTTPClient http;               // Use Weather API for live data if WiFi is connected
	http.setConnectTimeout(3000);  // 3 second max timeout
	http.setTimeout(4000);         // Max. 4 Sek auf die eigentlichen JSON-Daten warten
	                               // API documentation: https://openweathermap.org/current
	String weatherQueryURL = url + String("&lat=") + String(latitude, 5) + String("&lon=") + String(longitude, 5) + String("&units=") + units + String("&lang=") + lang + String("&appid=") + apiKey;
	Serial.println(weatherQueryURL);
	http.begin(weatherQueryURL.c_str());
	int httpResponseCode = http.GET();
	if (httpResponseCode == 200) {
		String payload = http.getString();
		JSONVar responseObject = JSON.parse(payload);
		Serial.println(responseObject);

		if ((!responseObject.hasOwnProperty("timezone_offset"))) {
			currentWeather.code = CODE_PARSE_ERROR;
			strncpy(currentWeather.log, "Missing fields", sizeof(currentWeather.log) - 1);
			currentWeather.log[sizeof(currentWeather.log) - 1] = '\0';

			return currentWeather;
		}

		currentWeather.temperature = int(responseObject["data"][0]["temp"]);
		currentWeather.weatherConditionCode = int(responseObject["data"][0]["weather"][0]["id"]);

		currentWeather.currentSunrise = uint32_t(responseObject["data"][0]["sunrise"]);
		currentWeather.currentSunset = uint32_t(responseObject["data"][0]["sunset"]);
		currentWeather.currentDT = uint32_t(responseObject["data"][0]["dt"]);

		const char* main = (const char*)responseObject["data"][0]["weather"][0]["main"];
		if (main != nullptr) {
			strncpy(currentWeather.weatherDescription, main, sizeof(currentWeather.weatherDescription) - 1);
			currentWeather.weatherDescription[sizeof(currentWeather.weatherDescription) - 1] = '\0';
		}

		const char* icon = (const char*)responseObject["data"][0]["weather"][0]["icon"];
		if (icon != nullptr) {
			strncpy(currentWeather.weatherIcon, icon, sizeof(currentWeather.weatherIcon) - 1);
			currentWeather.weatherIcon[sizeof(currentWeather.weatherIcon) - 1] = '\0';
		}

		// sync NTP during weather API call and use timezone of city
		syncNTP(long(responseObject["timezone_offset"]), settings.ntpServer.c_str());

		currentWeather.offset = long(responseObject["timezone_offset"]);

		currentWeather.uvi = (double)responseObject["data"][0]["uvi"];

		Serial.print("getWeatherData Weather: ");
		Serial.print(currentWeather.weatherIcon);
		Serial.println(currentWeather.weatherDescription);
	} else {
		// http error
		currentWeather.code = CODE_HTTP_ERROR;

		Serial.print("getWeatherDataByLocation Error code: ");
		Serial.println(currentWeather.code, DEC);
	}
	strncpy(currentWeather.log, String(httpResponseCode).c_str(), sizeof(currentWeather.log) - 1);
	currentWeather.log[sizeof(currentWeather.log) - 1] = '\0';
	http.end();

	return currentWeather;
}

locationData getReverseLocation(double latitude, double longitude, String url, String apiKey) {
	Serial.println("getReverseLocation Start");

	currentLocation.code = CODE_NO_ERROR;

	Serial.println("getReverseLocation Get");

	HTTPClient http;               // Use Weather API for live data if WiFi is connected
	http.setConnectTimeout(3000);  // 3 second max timeout
	http.setTimeout(4000);         // Max. 4 Sek auf die eigentlichen JSON-Daten warten
	                               // API documentation: https://openweathermap.org/current
	String locationQueryURL = url + String("?lat=") + String(latitude, 5) + String("&lon=") + String(longitude, 5) + String("&appid=") + apiKey + "&limit=1";
	Serial.println(locationQueryURL);
	http.begin(locationQueryURL.c_str());
	int httpResponseCode = http.GET();
	if (httpResponseCode == 200) {
		String payload = http.getString();
		JSONVar responseObject = JSON.parse(payload);
		Serial.println(responseObject);

		const char* main = (const char*)responseObject[0]["name"];
		if (main != nullptr) {
			strncpy(currentLocation.city, main, sizeof(currentLocation.city) - 1);
			currentLocation.city[sizeof(currentLocation.city) - 1] = '\0';

			// create short city name for display
			String name;
			int maxNameLength = 12;
			if (strlen(currentLocation.city) > maxNameLength) {
				name = String(currentLocation.city, maxNameLength - 1);
				if (name[maxNameLength - 2] != ' ') {
					name = name + String(".");
				}
			} else
				name = currentLocation.city;
			strcpy(currentLocation.cityShort, name.c_str());

		} else {
			strcpy(currentLocation.city, "");
			strcpy(currentLocation.cityShort, "");
		}

		Serial.print("getReverseLocation City: ");
		Serial.println(currentLocation.city);
	} else {
		// http error
		currentLocation.code = CODE_HTTP_ERROR;

		Serial.print("getReverseLocation Error code: ");
		Serial.println(currentWeather.code, DEC);
	}
	strncpy(currentLocation.log, String(httpResponseCode).c_str(), sizeof(currentLocation.log) - 1);
	currentLocation.log[sizeof(currentLocation.log) - 1] = '\0';
	http.end();

	return currentLocation;
}

void setupWeatherData() {
	currentWeather.weatherConditionCode = 0;
	currentWeather.offset = 0;
	currentWeather.temperature = 0;
	strcpy(currentWeather.weatherDescription, "");
	strcpy(currentWeather.weatherIcon, "");
	currentWeather.uvi = 0;
}

String Normalize2ASCII(String source) {
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
