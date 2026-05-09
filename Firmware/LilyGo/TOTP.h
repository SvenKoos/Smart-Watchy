/**
 * @file TOTP.h
 *
 */

#ifndef TOTP_H
#define TOTP_H

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
void setupTOTP(String base32Secret);
String calculateTotpCode();

/**********************
 *      MACROS
 **********************/

#endif /*TOTP_H*/
