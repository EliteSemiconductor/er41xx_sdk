/******************************************************************************/
/*
 * @file     Delay.c
 * @brief    Delay utility functions for NANO100 (Cortex-M0), based on SysTick
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "Common.h"

/* Definition & Macro -------------------------------------------------------*/

/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/

/* Function prototypes ------------------------------------------------------*/

/* Functions ----------------------------------------------------------------*/
void Delay_10us(unsigned int n)
{
    while (n--) {
        CLK_SysTickDelay(10);
    }
}

void Delay_100us(unsigned int n)
{
    while (n--) {
        CLK_SysTickDelay(100);
    }
}

void Delay_ms(unsigned int n)
{
    while (n--) {
        CLK_SysTickDelay(1000);
    }
}

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
