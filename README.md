# Smart Watchy - smart watch functionality on Watchy

## Functional principles
Smart Watchy and companion are app using WiFi to communicate to each other.

Smart messages are transferred ones per minute from mobile app to Watchy.

### Smart messages from mobile device on Watchy
New messages detected on mobile device and filtered by companion app configuration are forwarded to Watchy and indicated by icon on wtach face.

Use the right up and down buttons on Watchy to open and change the messages; use left up button to get back to watch face.

The list of messages is limited to last 20 entries.

A single message is shortend to the size of the Watchy display (no scrolling).

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
- Remote MAC address: WiFi MAC address of the Watchy to limit the access of the companion app in standard format (xx:xx:xx:xx:xx:xx)

### Setup mobile hotspot
Use a secure password for tethering.

## Watchy firmware: Smart Watchy

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
- Connect the Watchy to the hotspot of your mobile device.

## Features under development

### Android Smart Lock support
The Watchy should be used as Trusted device for Android Smart Lock functionality.

The Watchy must be paired / bonded to the mobile device (new item in Watchy menu).

Watchy will advertise via BLE to support Smart Lock unlock.

## Hints
Location discovery: If mobile device is in roaming zone, IP address is still received from home mobile service provider, which results in home weather report on watch face.

Quiet mode: Exit by pressing the left up button.
