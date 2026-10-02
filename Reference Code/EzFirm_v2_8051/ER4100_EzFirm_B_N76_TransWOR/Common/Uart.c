/******************************************************************************/
/*
 * @file     Uart.c
 * @version  V1.0.0
 * @brief    UART0 driver for N76E003
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
 * @brief  Initialize UART0 with Timer1 baud rate generator
 * @param  baud: baud rate selection (uart_baud_t)
 * @retval None
 */
void InitialUART0_Timer1(uart_baud_t baud)
{
    #if ENABLE_UART == 1
    P06_Quasi_Mode;
    P07_Quasi_Mode;

    SCON = 0x50;
    TMOD |= 0x20;

    set_SMOD;
    set_T1M;
    clr_BRCK;

#ifdef FOSC_160000
    switch (baud) {
        case UART_BAUD_9600:   TH1 = 0x97; break;
        case UART_BAUD_19200:  TH1 = 0xCB; break;
        case UART_BAUD_38400:  TH1 = 0xE5; break;
        case UART_BAUD_57600:  TH1 = 0xEE; break;
        case UART_BAUD_115200: TH1 = 0xF7; break;
        default:               TH1 = 0xF7; break;
    }
#endif

#ifdef FOSC_166000
    switch (baud) {
        case UART_BAUD_9600:   TH1 = 0x94; break;
        case UART_BAUD_19200:  TH1 = 0xCA; break;
        case UART_BAUD_38400:  TH1 = 0xE5; break;
        case UART_BAUD_57600:  TH1 = 0xEE; break;
        case UART_BAUD_115200: TH1 = 0xF7; break;
        default:               TH1 = 0xF7; break;
    }
#endif

    set_TR1;
    set_TI; // For printf, TI must be set to 1
    #endif
}
/**
 * @brief  Receive one byte from UART0 (blocking)
 * @param  None
 * @retval Received byte
 */
uint8_t Receive_Data_From_UART0(void)
{
    #if ENABLE_UART == 1
    uint8_t c;
    while (!RI);
    c = SBUF;
    RI = 0;
    return c;
    #endif
}
/**
 * @brief  Send one byte via UART0 (blocking)
 * @param  c: byte to send
 * @retval None
 */
void Send_Data_To_UART0(uint8_t c)
{
    #if ENABLE_UART == 1
    TI = 0;
    SBUF = c;
    while (TI == 0);
    #endif
}
/**
 * @brief  Send null-terminated string via UART0
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
 * @brief  Send one byte as 2-digit hex string (e.g. 0xFF → "FF")
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
 * @brief  Send a 32-bit value in 8-digit hex format
 * @param  v: value to send
 * @retval None
 */
void UART0_SendU32Hex8(uint32_t v)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        uint8_t nibble = (uint8_t)((v >> (28 - (i * 4))) & 0x0F);
        Send_Data_To_UART0(nibble < 10 ? ('0' + nibble) : ('A' + nibble - 10));
    }
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
 * @brief  Send a 32-bit value in decimal format
 * @param  v: value to send
 * @retval None
 */
void UART0_SendU32Dec(uint32_t v)
{
    uint8_t digits[10];
    uint8_t count = 0;
    uint8_t i;

    if (v == 0)
    {
        Send_Data_To_UART0('0');
        return;
    }

    while (v > 0)
    {
        digits[count++] = (uint8_t)(v % 10);
        v /= 10;
    }

    for (i = count; i > 0; i--)
    {
        Send_Data_To_UART0('0' + digits[i - 1]);
    }
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
/**
 * @brief  Send string followed by 32-bit decimal value
 * @param  s:        prefix string
 *         v:        value to print
 *         new_line: 1 = append "\r\n"
 * @retval None
 */
void UART0_SendStrU32Dec(const char *s, uint32_t v, uint8_t new_line)
{
    UART0_SendStr(s);
    UART0_SendU32Dec(v);
    if (new_line)
        UART0_SendStr("\r\n");
}
