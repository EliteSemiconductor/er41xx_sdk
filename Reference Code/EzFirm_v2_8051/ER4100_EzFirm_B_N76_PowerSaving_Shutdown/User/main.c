/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 PowerSaving_Shutdown application on N76E003
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
    P05_PushPull_Mode;              // P0.5 => Shutdown trigger button
    
    // Set P0.4 as output => ER4100-GPIO1 is shutdown pin in GPIO-default_mode, 
    // low = shutdown
    P04_PushPull_Mode;                            // Set P0.4 as output
    P04 = 1;                               // default high: normal operation
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
        UART0_SendStr("PowerSaving_Shutdown : init done\r\n");
        UART0_SendStr("\r\n******************************************\r\n");
        UART0_SendStr("PowerSaving_Shutdown Demo\r\n");
        UART0_SendStr("Press P05 to enter Shutdown(SHD_PIN = 0)\r\n");
        UART0_SendStr("Press P05 again to wake up\r\n");
        UART0_SendStr("******************************************\r\n");
    }
    else
        UART0_SendStr("PowerSaving_Shutdown : init fail\r\n");

    while (1)
    {
        XTAPP_Scan();
    }
}
