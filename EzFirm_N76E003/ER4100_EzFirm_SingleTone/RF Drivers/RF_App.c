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
/* Typedef ------------------------------------------------------------------*/
typedef enum{
    state_st_is_idle,
    state_to_lauch_st,
    state_st_is_launching,
    state_to_stop_st
} rf_st_state;
/* Variables ----------------------------------------------------------------*/
xdata_u8 rf_err_mode;
rf_st_state rf_single_tone_state;
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
            rf_single_tone_state = state_to_lauch_st; //default trigger Single Tone
            rf_err_mode = 0;
        }
    }
    
    return rf_err_mode;
}
/**
 * @brief  RF main scan loop: poll NIRQ and TX trigger button
 * @param  None
 * @retval None
 */
void XTAPP_Scan(void)
{
    if(rf_err_mode == 1)
        return;
    /* Single Tone trigger/Stop */
    if(P05 == 0)
    {
        if(rf_single_tone_state == state_st_is_idle)
            rf_single_tone_state = state_to_lauch_st;
        else
            rf_single_tone_state = state_to_stop_st;
        
        while(P05 == 0);   // Wait for button release
    }
    
    if(rf_single_tone_state == state_to_lauch_st)
    {
        UART0_SendStr("Start Single Tone!\r\n");
        rf_single_tone_state = state_st_is_launching;
        TRx_SingleTone_Trigger();
    }
    else if(rf_single_tone_state == state_to_stop_st)
    {
        UART0_SendStr("Stop Single Tone!\r\n");
        rf_single_tone_state = state_st_is_idle;
        TRx_SingleTone_Disable();
    }
}
