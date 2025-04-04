/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    araduino.cpp
 * @brief   I2C wrapper functions for Arduino
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#include "hal/hal.h"
#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include "hal/arduino/arduino_hal.h"

static char const*
_GetErrorString ( int  error, int  scope, char*  str, int  strLen ) {
  char  buf [ 30 ];
  char const*  err;
  switch ( error ) {
  case 1:
    err = "Data too long to fit in transmit buffer";
    break;
  case 2:
    err = "Received NACK on transmit of address";
    break;
  case 3:
    err = "Received NACK on transmit of data";
    break;
  case 4:
    err = "Other error";
    break;
  case 5:
    err = "Timeout";
    break;
  default:
    sprintf ( buf, "Unkown error %d", error );
    err = buf;
  }
  snprintf ( str, strLen, "Arduino Wire Error: %s", err );
  return str;
}

static void
_Delay ( uint32_t ms ) {
  delay ( ms );
}

static int
_I2CRead ( void*  ifce, uint8_t  slAddr, uint8_t*  wrData, int  wrSize, uint8_t*  rdData, int  rdSize ) {
  ( void) ifce;
  Wire . beginTransmission( slAddr );
  if ( wrSize ) {
    Wire . write( wrData, wrSize );
    Wire . endTransmission ( false );
    delay ( 10 );
  }
  Wire . requestFrom ( slAddr, rdSize );

  int i = 0;
  while ( Wire . available ( ) && i < rdSize ) { // slave may send less than requested
    //for ( int i = 0; i < rdSize; i++ ) {
    rdData [ i++ ] = Wire . read ( );
    //}
  }
  int  errorCode = Wire . endTransmission ( );
  if ( errorCode )
    return HAL_SetError ( errorCode, aesArduino, _GetErrorString );
  return ecSuccess;
}


static int
_I2CWrite( void*  ifce, uint8_t  slAddr, uint8_t*  wrData1, int  wrSize1, uint8_t*  wrData2, int  wrSize2 ) {
  ( void) ifce;
  Wire . beginTransmission ( slAddr );
  if ( wrSize1 )
    Wire . write ( wrData1, wrSize1 );
  if ( wrSize2 )
    Wire . write ( wrData2, wrSize2 );
  int  errorCode = Wire . endTransmission ( );
  if ( errorCode )
    return HAL_SetError ( errorCode, aesArduino, _GetErrorString );
  return ecSuccess;
}



int
HAL_Init ( Interface_t*  hal ) {
  Wire . begin ( );
  hal -> i2cRead     = _I2CRead;
  hal -> i2cWrite    = _I2CWrite;
  hal -> msSleep     = _Delay;
  hal -> reset       = NULL;
  // Allow UART interface to settle - otherwise startup information 
  //  will not be received by Arduino IDE
  _Delay ( 2500 );
  return ecSuccess;
}

int
HAL_Deinit      ( Interface_t*  hal ) {
  return ecSuccess;
}

void
HAL_HandleError ( int  errorCode, void const*  contextV ) {
  char const* context = ( char const* ) contextV;
  int  error, scope;
  char  msg [ 200 ];
  if ( errorCode ) {
    Serial . print ( "Error " );
    Serial . print ( errorCode );
    Serial . print ( " received during " );
    Serial . println ( ( char const* ) context );
    Serial . print ( "  " );
    Serial . println ( HAL_GetErrorInfo (  &error, &scope, msg, 200 ) );
  }
  while ( 1 );
}

