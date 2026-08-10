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
    P04_PushPull_Mode;              // P0.4 => TX trigger button with wrong PAIN ID
    P05_PushPull_Mode;              // P0.5 => TX trigger button with right ID & ADDR
    P06_PushPull_Mode;              // P0.5 => TX trigger button with wrong ADDR1
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
        UART0_SendStr("802154TxRx_Addressing : init done\r\n");
    else
        UART0_SendStr("802154TxRx_Addressing : init fail\r\n");
    UART0_SendStr("******************************************\r\n");
    UART0_SendStr("* Waiting for RX or UART_CMD...\r\n");
    UART0_SendStr("* CMD: 1=Send Match packet\r\n");
    UART0_SendStr("* CMD: 2=Broadcast\r\n");
    UART0_SendStr("* CMD: 3=Send PANID mismatch packet\r\n");
    UART0_SendStr("* CMD: 4=Send 1ADDR mismatch packet\r\n");
    UART0_SendStr("******************************************\r\n");
    while (1)
    {
        XTAPP_Scan();
    }
}
