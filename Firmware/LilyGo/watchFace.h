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
void drawIcons(bool isConnected);
void drawWeather();

/**********************
 *      MACROS
 **********************/

#endif /*WATCH_FACE_H*/
