/******************************************************************************/
/*
 * @file     Delay.h
 * @brief    Delay utility macros and function declarations
 *           for N76E003 @ 16MHz (1T mode, 1 cycle = 62.5ns)
 *
 * @copyright (C) COPYRIGHT 2020 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef _DELAY_H_
#define _DELAY_H_

/* Definition & Macro -------------------------------------------------------*/
#define NOP()    _nop_()            // 1 cycle  =  62.5ns
#define NOP5()   NOP();NOP();NOP();NOP();NOP()
#define NOP10()  NOP5();NOP5()
#define NOP20()  NOP10();NOP10()
#define NOP40()  NOP20();NOP20()

#define DELAY_1US()   NOP();NOP5();NOP10()  // inline ~1us, no call overhead
#define DELAY_2US()   DELAY_1US();DELAY_1US()
#define DELAY_4US()   DELAY_2US();DELAY_2US()
#define DELAY_8US()   DELAY_4US();DELAY_4US()
#define DELAY_10US()  NOP40();NOP40();NOP40();NOP40()              // inline ~10us, no call overhead
/* Function -----------------------------------------------------------------*/
void Delay_10us(unsigned int n);
void Delay_100us(unsigned int n);
void Delay_ms(unsigned int n);
#endif  /* _DELAY_H_ */

/*** (C) COPYRIGHT 2020 ESMT Technology Corp. ***/
