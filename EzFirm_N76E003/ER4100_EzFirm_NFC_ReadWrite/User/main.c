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
        UART0_SendStr("NFC_ReadWrite : init done\r\n");
    else
        UART0_SendStr("NFC_ReadWrite : init fail\r\n");
    
    /* MCU read write NFC memory block */
    UART0_SendStr("-----------------------------------------------------------------\r\n");
    UART0_SendStr("Task1 - Read and Write by MCU\r\n");
    XTAPP_ReadWriteTest_MCU_Side(); 
    
    UART0_SendStr("\r\n-----------------------------------------------------------------\r\n");
    UART0_SendStr("Task2 - Read and write by phone, waiting for field from phone\r\n");
    while (1)
    {
        /* wait for access from reader/phone */
        XTAPP_ReadWriteTest_Phone_Side();
    }
}
