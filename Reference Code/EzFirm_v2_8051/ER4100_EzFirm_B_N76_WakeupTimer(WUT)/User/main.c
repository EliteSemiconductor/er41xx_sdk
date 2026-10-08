/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 WakeupTimer application on N76E003
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
 * @brief  Print the demo usage (UART key-in commands)
 * @param  None
 * @retval None
 */
void Demo_Help_Msg(void)
{
    UART0_SendStr("\r\n*******************************\r\n");
    UART0_SendStr("WakeupTimer Demo\r\n");
    UART0_SendStr("Key in WUT period:\r\n");
    UART0_SendStr("* '0' = 10ms, Periodic mode\r\n");
    UART0_SendStr("* '1' = 50ms, Periodic mode\r\n");
    UART0_SendStr("* '2' = 100ms, Periodic mode\r\n");
    UART0_SendStr("* '3' = 500ms, Periodic mode\r\n");
    UART0_SendStr("* '4' = 1s, Periodic mode\r\n");
    UART0_SendStr("* '5' = 1s, One-shot mode\r\n");
    UART0_SendStr("* '6' = 2s, One-shot mode\r\n");
    UART0_SendStr("* '7' = 4s, One-shot mode\r\n");
    UART0_SendStr("* 'p' = Stop periodic trigger\r\n");
    UART0_SendStr("*******************************\r\n");
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
        UART0_SendStr("WakeupTimer : init done\r\n");
        Demo_Help_Msg();
    }
    else
        UART0_SendStr("WakeupTimer : init fail\r\n");

    while (1)
    {
        /* Select WUT period / mode by UART input character */
        if (RI)     // UART0 received a byte
        {
            uint8_t c = Receive_Data_From_UART0();

            switch (c)
            {
                case '0': XTAPP_WUT_Config(0, 1); break;   // 10ms,  Periodic mode
                case '1': XTAPP_WUT_Config(1, 1); break;   // 50ms,  Periodic mode
                case '2': XTAPP_WUT_Config(2, 1); break;   // 100ms, Periodic mode
                case '3': XTAPP_WUT_Config(3, 1); break;   // 500ms, Periodic mode
                case '4': XTAPP_WUT_Config(4, 1); break;   // 1s,    Periodic mode
                case '5': XTAPP_WUT_Config(4, 0); break;   // 1s,    One-shot mode
                case '6': XTAPP_WUT_Config(5, 0); break;   // 2s,    One-shot mode
                case '7': XTAPP_WUT_Config(6, 0); break;   // 4s,    One-shot mode
                case 'p': 
                    XTAPP_WUT_Stop();
                    Demo_Help_Msg();
                    break;   // stop periodic trigger
                default: break;
            }
        }
        /* RF Task Process */
        XTAPP_Scan();
    }
}
