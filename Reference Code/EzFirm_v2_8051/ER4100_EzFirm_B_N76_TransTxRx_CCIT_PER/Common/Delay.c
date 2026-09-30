/******************************************************************************/
/*
 * @file     Delay.c
 * @brief    Delay utility functions for N76E003 @ 16MHz (1T mode)
 *
 * @copyright (C) COPYRIGHT 2020 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "Common.h"
#include <intrins.h>

/* Definition & Macro -------------------------------------------------------*/

/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
bit BIT_TMP;

/* Function prototypes ------------------------------------------------------*/

/* Functions ----------------------------------------------------------------*/
void Delay_1us(unsigned int n)
{
    while (n--) {
        DELAY_1US();
    }
}

void Delay_10us(unsigned int n)
{
    while (n--) {
        DELAY_10US();
    }
}

void Delay_100us(unsigned int n)
{
    unsigned char i;
    while (n--) {
        i = 10;
        while (i--) {
            DELAY_10US();   // 10us x 10 = 100us
        }
    }
}

void Delay_ms(unsigned int n)
{
    unsigned char i;
    while (n--) {
        i = 100;
        while (i--) {
            DELAY_10US();   // 10us x 100 = 1ms
        }
    }
}

/*** (C) COPYRIGHT 2020 ESMT Technology Corp. ***/
