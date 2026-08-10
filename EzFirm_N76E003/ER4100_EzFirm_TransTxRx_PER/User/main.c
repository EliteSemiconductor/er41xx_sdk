/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 TransTxRx application on N76E003
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

/* Function -----------------------------------------------------------------*/
/**
 * @brief  Initialize GPIO pins
 * @param  None
 * @retval None
 */
void GPIO_Init(void)
{
    P05_PushPull_Mode;              // P0.5 => TX trigger button
}
/**
 * @brief  Main entry
 * @param  None
 * @retval None
 */
void main(void)
{
    InitialUART0_Timer1(UART_BAUD_115200);
    GPIO_Init();
    if(XTAPP_Init()==0)
        UART0_SendStr("TransTxRx_PER : init done\r\n");
    else
        UART0_SendStr("TransTxRx_PER : init fail\r\n");
    while (1)
    {
        XTAPP_Scan();
    }
}
