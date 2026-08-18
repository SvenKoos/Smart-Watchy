# Smart Watchy - smart watch functionality on Watchy and LilyGo

## Functional principles
Smart Watchy and companion are app using WiFi to communicate to each other.

Smart messages are transferred ones per minute from mobile app to Watchy.

### Smart messages from mobile device on Watchy
New messages detected on mobile device and filtered by companion app configuration are forwarded to Watchy and indicated by icon on wtach face.

Use the right up and down buttons on Watchy to open and change between the messages; use left up button to get back to watch face.
Double-tap on LilyGo watch face to open and change between the messages; Double-tap to leave the message view back to watch face.

The list of messages is limited to last 20 entries.

A single message is shortend to the size of the watch display (no scrolling on Watchy).

## Mobile app: ESP-Alerts-for-Android

The companion app is based on the Hackster.io and Hackaday.io projects [Read Phone Notifications using ESP], the ESP-Alerts-for-Arduino project by mitchwongho and the embed-http-web-server-in-android by Yayo-Arellano.

The companion app provides the following features:
- scan the messages to the message center of the Android OS
- run as service in the background
- make the relevant message sources configurable for message scan
- configure the Watchy as message consumer
							
### Build and Deployment
Build the mobile app with Android Studio or use the signed .apk from release.zip and deploy it to your mobile device via Android Studio or sideloading.

Change the mobile app settings for battery usage to Unlimited to enable running in background.

### Configuration
Configure the behavior of mobile app in the Settings section of the app (optional).
- Run as a service
- Start at boot
- Flip display vertically
- Remote MAC address: WiFi MAC address of the Watchy or LilyGo to limit the access of the companion app on mobile for certain watch device in standard format (xx:xx:xx:xx:xx:xx)

### Setup mobile hotspot
Use a secure password for tethering.

## Smart Watchy firmware for Watchy

The firmware is based on Watchy package 1.4.x, the 7-segment watch face and adds several features:
- dynamic discovery of location based on IP address
- incl. weather report for discovered location
- smart messages from connected mobile on Watchy
- quiet mode for inactivity phases

### Build and Deployment
Change the OpenWeather API key in settings.h to your own.

Build the firmware with Arduino IDE and deploy it to your Watchy.

### Configuration
- WiFi MAC address of the Watchy is shown in the About view (to optionally limit the access to companion app).
- Connect the Watchy to the hotspot of your mobile device with Wifi Manager available from menu.

## Features under development

### Android Smart Lock / Extended Unlock support
The Watchy should be used as Trusted device for Android Smart Lock / Extended Unlock functionality.

The Watchy must be paired / bonded to the mobile device (new item in Watchy menu) and registered as trusted device in Extended Unlock.

Watchy is advertising via BLE to support Extended Unlock.
In most of the cases this is not sufficient to fulfill the requirements for Extended Unlock (constant connection via BLE, active GATT service).
Working with WiFi and BLE in parallel and constant way (for BLE) on ESP32 creates several issues with acceptable powermanagement.

Further development of the trusted device feature will be done on LilyGo.

## Hints
Location discovery: If mobile device is in roaming zone, IP address is still received from home mobile service provider, which results in home weather report on watch face.

Quiet mode: Exit by pressing the left up button.

# Smart Watchy goes LilyGo
The Smart Watchy firmware is now available on LilyGo.
It is compatible with the existing companion mobile app for Android.
The firmware directory contains appropriate directories for Watchy and LilyGo versions.

## Smart Watchy firmware for LilyGo
Smart Watchy firmware for LilyGo is running on LilyGo T-Watch-S3 and T-Watch-S3 Plus.
- The UI is changed from button-based usage to gesture-based handling.
- The menu is still provided using the one and only LilyGo button.
The firmware is based on LilyGo library 0.1.0 sharing the same features as firmware for Watchy but adding:
- time-based one-time password (TOTP) compatible with Microsoft Entra.ID and Google
- remaining battery capacity daily chart in the menü
- step counter daily chart in the menü
- Support for Google Geolocation API to discover the location

### Build and Deployment
Change additionally the TOTP secret in settings.h to your own.

# Smart Watchy goes LoRa WAN on LilyGo
Smart Watchy on LiLygo is using LoRa WAN to
- forward messages from mobile message center on Android received with Smart Watchy on LilyGo T-Watch to T-Echo Lite,
- send messages from LoRa Messenger menu item of Smart Watchy on LilyGo to T-Echo Lite.

LoRa transmitting is encrypted and uses a sender identifier (LoRa magic) to ensure information security.
Firmware for T-Echo Lite is added to the repository.

# Smart Watchy on LilyGo gets Daily Agenda Watchface
Daily calendar entries are transferred as messages through message center to Smart Watchy on LilyGo as JSON paylod containing the subject (Topic), start time (Start) and end time (End).
```JSON  
{"Topic":"Project NOVA - EIS - Reference Group#2","Start":"2026-08-18T11:00:00.0000000","End":"2026-08-18T11:45:00.0000000"}
```

Application name and title depend on the way to transfer the agenda items to the message center of Android.
- JSON structure, message application name and title can be adepted in code (alertData.cpp, deviceEvent.cpp, config.h) to any specific solution.

## Transfer agenda items to message center (Microsoft way)
- use Power Automate to create the periodic cloud-based flow, read daily agenda from Outlook 365 and transfer the items to Teams
  - recurrency
```Power Automate
 {
  "type": "Recurrence",
  "recurrence": {
    "frequency": "Day",
    "interval": "1",
    "timeZone": "W. Europe Standard Time",
    "schedule": {
      "hours": [
        "7"
      ],
      "minutes": [
        0
      ]
    }
  }
}
```
  - retrieve calendar view of the events (V3)
```Power Automate
{
  "type": "OpenApiConnection",
  "inputs": {
    "parameters": {
      "calendarId": "your_calendar_id",
      "startDateTimeUtc": "@startOfDay(utcNow())",
      "endDateTimeUtc": "@addDays(startOfDay(utcNow()), 1)"
    },
    "host": {
      "apiId": "/providers/Microsoft.PowerApps/apis/shared_office365",
      "connection": "shared_office365",
      "operationId": "GetEventsCalendarViewV3"
    }
  },
  "runAfter": {}
}
```
 - Select
```Power Automate
{
  "type": "Select",
  "inputs": {
    "from": "@outputs('Kalenderansicht_der_Termine_abrufen_(V3)')?['body/value']",
    "select": {
      "Topic": "@item()?['subject']",
      "Start": "@item()?['start']",
      "End": "@item()?['end']"
    }
  },
  "runAfter": {
    "Kalenderansicht_der_Termine_abrufen_(V3)": [
      "Succeeded"
    ]
  }
}
```
  - For each
```Power Automate
{
  "type": "Foreach",
  "foreach": "@outputs('Auswählen')['body']",
  "actions": {
    "Nachricht_in_einem_Chat_oder_Kanal_veröffentlichen": {
      "type": "OpenApiConnection",
      "inputs": {
        "parameters": {
          "poster": "Flow bot",
          "location": "Chat with Flow bot",
          "body/recipient": "your_email_address",
          "body/messageBody": "<p class=\"editor-paragraph\">@{items('For_each')}</p>"
        },
        "host": {
          "apiId": "/providers/Microsoft.PowerApps/apis/shared_teams",
          "connection": "shared_teams",
          "operationId": "PostMessageToConversation"
        }
      }
    }
  },
  "runAfter": {
    "Auswählen": [
      "Succeeded"
    ]
  }
}
```
