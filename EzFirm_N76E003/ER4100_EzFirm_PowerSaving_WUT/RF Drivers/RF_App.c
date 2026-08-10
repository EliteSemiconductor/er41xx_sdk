/******************************************************************************/
/*
 * @file     RF_App.c
 * @version  V1.0.0
 * @brief    RF application layer for ER4100 transceiver
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "Common.h"
/* Definition & Macro -------------------------------------------------------*/
// RF_BUF_SIZE
//     expected RX payload length passed to TRx_RX_Trigger().
//     802.15.4 mode:               this value is ignored; MAC layer reads the
//                                 PHR (Physical Header) in the received frame
//                                 to determine the actual payload length.
//     Standard transparent mode:   1 ~ 128 bytes
//     Long packet transparent mode: 1 ~ 2047 bytes
#define RF_BUF_SIZE  64

// Wakeup Timer Period Calculation
//        Register_Value = (Period(ms)*32768)/1000 
//        Example1: If period target is 1000ms (1 second)
//                  Calculation: (1000 * 32768) / 1000 = 32768 = 0x00008000
//        Example2: If period target is 2000ms (2 second)
//                  Calculation: (2000 * 32768) / 1000 = 65536 = 0x00010000
#define SET_WUT_PERIORD_ARRAY_10MS(arr)     SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x01, 0x47)  //50ms
#define SET_WUT_PERIORD_ARRAY_50MS(arr)     SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x06, 0x66)  //50ms
#define SET_WUT_PERIORD_ARRAY_100MS(arr)    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x0C, 0xCC)  //100ms
#define SET_WUT_PERIORD_ARRAY_500MS(arr)    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x40, 0x00)  //500ms
#define SET_WUT_PERIORD_ARRAY_1S(arr)       SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x80, 0x00)  //1000ms
#define SET_WUT_PERIORD_ARRAY_2S(arr)       SET_U8_ARRAY(u8Array, 0x00, 0x01, 0x00, 0x00)  //2000ms
#define SET_WUT_PERIORD_ARRAY_4S(arr)       SET_U8_ARRAY(u8Array, 0x00, 0x02, 0x00, 0x00)  //4000ms
#define WUT_PERIOD_SEL(arr)                 SET_WUT_PERIORD_ARRAY_2S(arr)
/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
xdata_u8           rf_err_mode;
/* Function prototypes ------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/**
 * @brief  Print register value to UART in hex format
 * @param  addr: 16-bit register address
 * @retval None
 */
void dump_reg(uint16_t addr)
{
    uint8_t  u8Array[4];
    TRx_READREG(addr, u8Array);
    UART0_SendStr("0x");
    UART0_SendHex8(addr >> 8);
    UART0_SendHex8(addr & 0xFF);
    UART0_SendStr(" : 0x");
    UART0_SendHex8(u8Array[3]);
    UART0_SendHex8(u8Array[2]);
    UART0_SendHex8(u8Array[1]);
    UART0_SendHex8(u8Array[0]);
    UART0_SendStr("\r\n");
}
/**
 * @brief  Print data buffer to UART in hex format
 * @param  buf:   pointer to data buffer
 *         len:   number of bytes to print
 *         rx_fg: 1 = RX packet, 0 = TX packet
 *         rssi:  RSSI value (used when rx_fg = 1)
 * @retval None
 */
void dump_buffer(uint8_t *buf, uint16_t len, uint8_t rx_fg, uint16_t rssi)
{
    uint16_t ii;

    if(rx_fg)
    {
        UART0_SendStrDec3("RX(", len, 0);
        UART0_SendStr("):");
        UART0_SendStrDec3("rssi: -", rssi, 0);
    }
    else
    {
        UART0_SendStr("TX(");
        UART0_SendDec3(len);
        UART0_SendStr("):");
    }

    for(ii = 0; ii < len; ii++)
    {
        if((ii & 0x0F) == 0)    // new line every 16 bytes
        {
            UART0_SendStr("\r\n");
            UART0_SendDec3(ii);
            UART0_SendStr("    ");
        }
        UART0_SendHex8(buf[ii]);
        Send_Data_To_UART0(' ');
    }

    UART0_SendStr("\r\n");
}
/**
 * @brief  Read chip ID to confirm RF transceiver is present
 * @param  None
 * @retval 0: pass, 1: fail
 */
uint8_t XTAPP_Strobe(void)
{
    uint8_t  u8Array[4];
    uint8_t  try_cnt = 100;
    TRx_READREG(0xF100, u8Array);
    do
    {
        if(( u8Array[3] == 0x90) && ( u8Array[2] == 0x00) && ( u8Array[1] == 0x00))
        {
            break;
        }
        else
        {
            try_cnt--;
            Delay_ms(10);
            if( ! try_cnt )
            {
                return 1;   // fail
            }
        }
        TRx_READREG(0xF100, u8Array);
    } while(1);
    return 0;   // pass
}
/**
 * @brief  Initialize RF transceiver and start RX
 * @param  None
 * @retval 0: pass, 1: fail
 */
uint8_t XTAPP_Init(void)
{
    rf_err_mode = 1;
    TRx_IoConfig();

    if( XTAPP_Strobe() == 0 )   // Check RF chip present
    {
        TRx_SW_Reset();
        if(TRx_Init() == TRx_STATUS_SUCCESS);   // Default initialization
        {
            uint8_t  u8Array[4];
            UART0_SendStr("Enter deep sleep and wakeup after WUT timing...");
            WUT_PERIOD_SEL(u8Array);
            TRx_SetWUTMR_Timer(u8Array, 0);  //start wakeup timer in oneshot mode
            TRx_PowerDownMode();    //Enter deep sleep
            rf_err_mode = 0;
        }
    }
    return rf_err_mode;
}
/**
 * @brief  Read received data length and FIFO contents
 * @brief  RF main scan loop: poll NIRQ and Power saving trigger button
 * @param  None
 * @retval None
 */
void XTAPP_Scan(void)
{
    if(rf_err_mode == 1)
        return;
    /* IRQ handler: process wakeup event */
    if(NIRQ_Value == 0)
    {
        // Read and clear interrupt flags
        uint8_t  u8Array[4];
        TRx_GetIntStatus(u8Array);
        if(IS_INT_ST_ANY(u8Array))
        {
            TRx_ClearIntFlag(u8Array);
            
            if(IS_INT_ST_TMRTHD(u8Array))
            {   /* (optional) Process Wakeup timer event, if INT_ST_TMRTHD is enable */
                UART0_SendStr("Wakeup Timer\r\n");
            }
            
            if(IS_INT_ST_WAKEUP(u8Array))
            {   /* Wakeup event */
                Delay_ms(100);
                UART0_SendStr("Wakeup, and etner deep sleep again...\r\n"); 
                WUT_PERIOD_SEL(u8Array);
                TRx_SetWUTMR_Timer(u8Array, 0);  //start wakeup timer in oneshot mode
                TRx_PowerDownMode();        //etner deep sleep again
            }
        }
    }
}
