/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 PowerSaving_WUT application on N76E003
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
    {
        UART0_SendStr("PowerSaving_WUT : init done\r\n");
        UART0_SendStr("\r\n******************************************\r\n");
        UART0_SendStr("PowerSaving with Wakeup Timer Demo\r\n");
        UART0_SendStr("Set wakeup period by WUT_PERIOD_SEL(arr)\r\n");
        UART0_SendStr("******************************************\r\n");
    }
    else
        UART0_SendStr("PowerSaving_WUT : init fail\r\n");

    while (1)
    {
        XTAPP_Scan();
    }
}
