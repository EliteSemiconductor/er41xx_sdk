/******************************************************************************/
/*
 * @file     AppUart.c
 * @version  V1.0.0
 * @brief    UART driver for NANO100 (Cortex-M0), backed by hardware UART1
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "Common.h"

/* Definition & Macro -------------------------------------------------------*/
#define ENABLE_UART (1)
/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/

/* Function prototypes ------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/**
 * @brief  Initialize UART1 (module clock / pin-mux already done by SYS_Init)
 * @param  baud: baud rate selection (uart_baud_t)
 * @retval None
 */
void InitialUART0_Timer1(uart_baud_t baud)
{
    #if ENABLE_UART == 1
    uint32_t baudrate;

    switch (baud) {
        case UART_BAUD_9600:   baudrate = 9600;   break;
        case UART_BAUD_19200:  baudrate = 19200;  break;
        case UART_BAUD_38400:  baudrate = 38400;  break;
        case UART_BAUD_57600:  baudrate = 57600;  break;
        case UART_BAUD_115200: baudrate = 115200; break;
        default:               baudrate = 115200; break;
    }

    UART_Open(UART1, baudrate);
    /* Set RX Trigger Level = 1 byte */
    UART1->TLCTL = (UART1->TLCTL & ~UART_TLCTL_RFITL_Msk) | UART_TLCTL_RFITL_1BYTE;
    /* Set Timeout time 0x3E bit-time */
    UART_SetTimeoutCnt(UART1, 0x3E);
    #endif
}
/**
 * @brief  Check whether UART1 has a received byte waiting (non-blocking)
 * @param  None
 * @retval 1: a byte is available, 0: RX FIFO empty
 */
uint8_t UART0_IsDataReady(void)
{
    #if ENABLE_UART == 1
    return (UART_GET_RX_EMPTY(UART1) ? 0 : 1);
    #else
    return 0;
    #endif
}
/**
 * @brief  Receive one byte from UART1 (blocking)
 * @param  None
 * @retval Received byte
 */
uint8_t Receive_Data_From_UART0(void)
{
    #if ENABLE_UART == 1
    while (UART_GET_RX_EMPTY(UART1));
    return (uint8_t)UART_READ(UART1);
    #else
    return 0;
    #endif
}
/**
 * @brief  Send one byte via UART1 (blocking)
 * @param  c: byte to send
 * @retval None
 */
void Send_Data_To_UART0(uint8_t c)
{
    #if ENABLE_UART == 1
    while (UART_IS_TX_FULL(UART1));
    UART_WRITE(UART1, c);
    #endif
}
/**
 * @brief  Send null-terminated string via UART1
 * @param  s: pointer to string
 * @retval None
 */
void UART0_SendStr(const char *s)
{
    while (*s)
    {
        Send_Data_To_UART0((uint8_t)*s++);
    }
}
/**
 * @brief  Send one byte as 2-digit hex string (e.g. 0xFF -> "FF")
 * @param  v: byte value
 * @retval None
 */
void UART0_SendHex8(uint8_t v)
{
    uint8_t hi = (v >> 4) & 0x0F;
    uint8_t lo = v & 0x0F;
    Send_Data_To_UART0(hi < 10 ? ('0' + hi) : ('A' + hi - 10));
    Send_Data_To_UART0(lo < 10 ? ('0' + lo) : ('A' + lo - 10));
}
/**
 * @brief  Send value as 3-digit decimal string (output capped at 3 digits, 0~999)
 * @param  v: value to send (uint16_t); values above 999 will display incorrectly
 * @retval None
 */
void UART0_SendDec3(uint16_t v)
{
    uint8_t d;

    d = 0;
    while (v >= 100) { v -= 100; d++; }
    Send_Data_To_UART0('0' + d);

    d = 0;
    while (v >= 10) { v -= 10; d++; }
    Send_Data_To_UART0('0' + d);

    Send_Data_To_UART0('0' + (uint8_t)v);
}
/**
 * @brief  Send string followed by 3-digit decimal value
 * @param  s:        prefix string
 *         v:        value to print
 *         new_line: 1 = append "\r\n"
 * @retval None
 */
void UART0_SendStrDec3(const char *s, uint16_t v, uint8_t new_line)
{
    UART0_SendStr(s);
    UART0_SendDec3(v);
    if (new_line)
        UART0_SendStr("\r\n");
}
/**
 * @brief  Send string followed by 2-digit hex value
 * @param  s:        prefix string
 *         v:        value to print
 *         new_line: 1 = append "\r\n"
 * @retval None
 */
void UART0_SendStrHex8(const char *s, uint8_t v, uint8_t new_line)
{
    UART0_SendStr(s);
    UART0_SendHex8(v);
    if (new_line)
        UART0_SendStr("\r\n");
}
