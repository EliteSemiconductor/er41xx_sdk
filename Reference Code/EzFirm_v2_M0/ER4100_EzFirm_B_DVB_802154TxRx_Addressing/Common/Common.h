/******************************************************************************/
/*
 * @file     Common.h
 * @version  V1.0.0
 * @brief    Common type definitions and includes for NANO100 (Cortex-M0) platform
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
#include <stdint.h>
#include <stdio.h>
/* Definition & Macro -------------------------------------------------------*/
/* 8051 memory-space qualifiers used by the ezgen-generated RF Drivers files
   (SPI_ER41xx.c/.h/_config.h, RF_App.c) have no meaning on Cortex-M0's flat
   address space, so they are compiled away here instead of editing those
   files, keeping them byte-for-byte identical to the ezgen output.
   NOTE: xdata_u8/16/32 map to plain (non-volatile) types -- on 8051 "xdata"
   only ever meant "external RAM data space", not C's volatile semantics, so
   this is the more accurate mapping. It also avoids ARMCC treating
   volatile-qualified buffers as incompatible with the const uint8_t* the
   TRx_WRITEREG()/TRx_Write()/TRx_WriteAddr() chain expects them to be
   read-only through. */
#define code
#define xdata
#define xdata_u8             uint8_t
#define xdata_u16            uint16_t
#define xdata_u32            uint32_t

/* MCU include --------------------------------------------------------------*/
#include "Nano100Series.h"

/* Common include ------------------------------------------------------------*/
#include "compiler_compat.h"

#include "Delay.h"
#include "AppUart.h"

/* RF include ----------------------------------------------------------------*/
#include "SPI_ER41xx.h"
#include "RF_Hal.h"
#include "RF_App.h"

#endif /* __COMMON_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
