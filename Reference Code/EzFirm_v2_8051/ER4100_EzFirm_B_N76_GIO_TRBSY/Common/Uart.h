/******************************************************************************/
/*
 * @file     Uart.h
 * @version  V1.0.0
 * @brief    UART0 driver declarations for N76E003
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __UART_H__
#define __UART_H__

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
void    InitialUART0_Timer1(uart_baud_t baud);
uint8_t UART0_IsDataReady(void);
void    Send_Data_To_UART0(uint8_t c);
uint8_t Receive_Data_From_UART0(void);
void    UART0_SendStr(const char *s);
void    UART0_SendHex8(uint8_t v);
void    UART0_SendU32Hex8(uint32_t v);
void    UART0_SendDec3(uint16_t v);
void    UART0_SendU32Dec(uint32_t v);
void    UART0_SendStrDec3(const char *s, uint16_t v, uint8_t new_line);
void    UART0_SendStrHex8(const char *s, uint8_t v, uint8_t new_line);
void    UART0_SendStrU32Dec(const char *s, uint32_t v, uint8_t new_line);

#endif /* __UART_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
