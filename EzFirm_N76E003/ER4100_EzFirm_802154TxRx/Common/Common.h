/******************************************************************************/
/*
 * @file     Common.h
 * @version  V1.0.0
 * @brief    Common type definitions and includes for N76E003 (8051) platform
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __COMMON_H__
#define __COMMON_H__

/* Type definitions (must be before any RF includes to avoid circular dependency) */
/* Typedef ------------------------------------------------------------------*/
#ifndef __UINT8_T_DEFINED
typedef unsigned char       uint8_t;
#define __UINT8_T_DEFINED
#endif
#ifndef __UINT16_T_DEFINED
typedef unsigned int        uint16_t;
#define __UINT16_T_DEFINED
#endif
#ifndef __UINT32_T_DEFINED
typedef unsigned long       uint32_t;
#define __UINT32_T_DEFINED
#endif

typedef signed char         int8_t;
typedef signed int          int16_t;
typedef signed long int     int32_t;

typedef bit                 BIT;

/* Definition & Macro -------------------------------------------------------*/
#define xdata_u8            volatile unsigned char xdata
#define xdata_u16           volatile unsigned int xdata
#define xdata_u32           volatile unsigned long int xdata

/* MCU include --------------------------------------------------------------*/
#include "N76E003.h"
#include "SFR_Macro.h"
#include "Function_define.h"

/* Common include ------------------------------------------------------------*/
#include "Delay.h"
#include "Uart.h"

/* RF include ----------------------------------------------------------------*/
#include "SPI_ER41xx.h"
#include "RF_Hal.h"
#include "RF_App.h"

#endif /* __COMMON_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
