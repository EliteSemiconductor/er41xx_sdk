/******************************************************************************/
/*
 * @file     AppUart.h
 * @version  V1.0.0
 * @brief    UART driver declarations for NANO100 (Cortex-M0)
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/
/* NOTE: named AppUart.* (not Uart.*) because on a case-insensitive
   filesystem "Uart.h"/"Uart.c" collide with Nano100Lib's own
   StdDriver "uart.h"/"uart.c" (both are on this project's build/include
   path) -- with matching filenames, the identical-by-default include
   guard would make the SDK's uart.h "win" and silently discard this
   file's declarations. */

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __APP_UART_H__
#define __APP_UART_H__

/* Includes -----------------------------------------------------------------*/

/* Definition & Macro -------------------------------------------------------*/

/* Typedef ------------------------------------------------------------------*/
typedef enum {
    UART_BAUD_9600,
    UART_BAUD_19200,
    UART_BAUD_38400,
    UART_BAUD_57600,
    UART_BAUD_115200,
} uart_baud_t;

/* Extend Variables ---------------------------------------------------------*/
/* Function -----------------------------------------------------------------*/
/* NOTE: function names are kept identical to the N76E003 source (UART0_*)
   so RF_App.c needs no change, even though the physical peripheral driven
   here is NANO100's UART1 (PB4/PB5), per ER41XX_EZ_StdTRx_Transparent. */
void    InitialUART0_Timer1(uart_baud_t baud);
void    Send_Data_To_UART0(uint8_t c);
uint8_t UART0_IsDataReady(void);
uint8_t Receive_Data_From_UART0(void);
void    UART0_SendStr(const char *s);
void    UART0_SendHex8(uint8_t v);
void    UART0_SendDec3(uint16_t v);
void    UART0_SendStrDec3(const char *s, uint16_t v, uint8_t new_line);
void    UART0_SendStrHex8(const char *s, uint8_t v, uint8_t new_line);

#endif /* __APP_UART_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
