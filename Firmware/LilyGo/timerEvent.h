/**
 * @file timerEvent.h
 *
 */

#ifndef TIMER_EVENT_H
#define TIMER_EVENT_H

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
void timerEventWatchface();
void timerEventBrightness(uint seconds);
static void timerBrightness_cb(void* arg);
static void timerWatchface_cb(void* arg);
void startBrightnessTimer(uint seconds);
void stopBrightnessTimer();
void displayWakeUp();
void BacklightOn();
void BacklightOff();

/**********************
 *      MACROS
 **********************/

#endif /*TIMER_EVENT_H*/
