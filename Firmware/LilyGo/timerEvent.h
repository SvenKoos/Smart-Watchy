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
void timerEventWatchface(void);
void timerEventBrightness(void);
static void timerBrightness_cb(void* arg);
static void timerWatchface_cb(void *arg);
void startBrightnessTimer();
void stopBrightnessTimer();
void displayWakup();
void displayGoToSleep();

/**********************
 *      MACROS
 **********************/

#endif /*TIMER_EVENT_H*/
