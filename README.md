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
Smart Watchy firmware for LilyGo is running on LilyGo T-Watch-S3 and T-Watch-S3 Ultra.
The firmware is based on LilyGo library 0.1.0 sharing the same features as firmware for Watchy but adding:
- Time-based one-time password (TOTP) compatible with Microsoft Entra.ID and Google

### Build and Deployment
Change additionally the TOTP secret in settings.h to your own.
