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
        UART0_SendStr("RSSI_Scan : init done\r\n");
    else
        UART0_SendStr("RSSI_Scan : init fail\r\n");

    UART0_SendStr("******************************************\r\n");
    UART0_SendStr("* RSSI Scan Demo\r\n");
    UART0_SendStr("* Scanning background RF energy every 1s\r\n");
    UART0_SendStr("* Output format: rssi(dBm): -XX\r\n");
    UART0_SendStr("******************************************\r\n");

    while (1)
    {
        XTAPP_Scan();
    }
}
