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
lv_timer_t timerEventWatchface(void);
esp_timer_handle_t timerEventBrightness(void);
static void timerBrightness_cb(void* arg);
void startBrightnessTimer(esp_timer_handle_t brightness_timer);
void stopBrightnessTimer(esp_timer_handle_t brightness_timer);

/**********************
 *      MACROS
 **********************/

#endif /*TIMER_EVENT_H*/
