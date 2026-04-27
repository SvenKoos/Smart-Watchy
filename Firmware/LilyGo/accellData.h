/**
 * @file accellData.h
 *
 */

#ifndef ACCELL_DATA_H
#define ACCELL_DATA_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct accellData {
  uint32_t stepCounter;
  int16_t xAccell;
  int16_t yAccell;
  int16_t zAccell;
  bool isMoved;
  char log[LOG_LEN];
  int code;
} accellData;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
accellData getAccellData();
void resetAccellData();
void setupAccellData();

/**********************
 *      MACROS
 **********************/

#endif /*ACCELL_DATA_H*/
