/**
 * @file watchFace.h
 *
 */

#ifndef WATCH_FACE_H
#define WATCH_FACE_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void watchFaceSetup();
void drawWatchFace();
void drawTime();
void drawDate();
void drawSteps();
void drawBattery();
void drawWeather();
void drawSolarArc(uint32_t sunrise_timestamp, uint32_t sunset_timestamp, uint32_t current_timestamp);
void drawAlert();
void drawAnalogClock();
void drawUVI(double uvi_val);
void drawQlockTwo();
void drawAgendaHeader(lv_obj_t *parent);
void drawAgendaWatchface(void);
static void eventGestureAgendaCB(lv_event_t *e);
static void registerAgendaEvents(lv_obj_t *cont);

/**********************
 *      MACROS
 **********************/

#endif /*WATCH_FACE_H*/
