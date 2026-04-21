/**
 * @file syncNTP.h
 *
 */

#ifndef SYNC_NTP_H
#define SYNC_NTP_H

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
bool syncNTP(long gmt, const char *ntpServer);
void setupNTPSync();
void timeavailable(struct timeval *t);

/**********************
 *      MACROS
 **********************/

#endif /*SYNC_NTP_H*/
