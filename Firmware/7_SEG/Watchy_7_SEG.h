#ifndef WATCHY_7_SEG_H
#define WATCHY_7_SEG_H

#include <Watchy.h>
#include "Seven_Segment10pt7b.h"
#include "DSEG7_Classic_Regular_15.h"
#include "DSEG7_Classic_Bold_25.h"
#include "DSEG7_Classic_Regular_39.h"
#include "icons.h"

class Watchy7SEG : public Watchy{
    using Watchy::Watchy;
    public:
        void drawWatchFace();
        void drawTime();
        void drawDate();
        bool drawSteps(accelData currentAccel);
        void drawWeather(locationData currentLocation);
        void drawBattery();
        locationData getLocation();
        void drawAlerts(String gatewayIP, String macAdress);
        void drawStackTrace();
};

#endif