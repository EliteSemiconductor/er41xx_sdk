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
typedef struct {
    uint16_t addr;
    uint8_t  reg_data[4];
    //reg_data[0] is bit7:0 of uint32_t, lowest byte of register data
    //reg_data[1] is bit15:8 of uint32_t
    //reg_data[2] is bit23:16 of uint32_t
    //reg_data[3] is bit31:24 of uint32_t, highest byte of register data
} t_cfg_reg;

/* Variables ----------------------------------------------------------------*/
xdata_u8 rf_err_mode;
t_xtapp_config_t   xdata gtXtAppConfigInfo;
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
 * @brief  Print the compiled RF configuration (crystal, cap IO, data rate, deviation,
 *         frequency, TX power, FIFO/interrupt setup, syncword) from the included
 *         SPI_ER41xx_config_*.h to UART.
 * @param  None
 * @retval None
 */
void dump_rf_config(void)
{
    t_cfg_reg xo_cfg     = { XO_CONFIG_1 };
    t_cfg_reg freq_cfg   = { SYNTH_CAL_CONFIG_0 };
    uint8_t   rate_bytes[4] = GLB_DATA_RATE;
    uint8_t   dev_bytes[4]  = GLB_DEVIATION;
    t_cfg_reg pwr_cfg    = { ANCTL_CONFIG_7 };
    t_cfg_reg fifo_cfg   = { FIFO_FEA_REG };
    t_cfg_reg int_cfg    = { INT_EN_REG };
    t_cfg_reg txmode_cfg = { MAC_TX_CFG };
    uint32_t u32Val;

    UART0_SendStr("===== RF Config form Toolkit=====\r\n");
    UART0_SendStr("Crystal Hz     : ");
    UART0_SendU32Dec((uint32_t)CRYSTAL_HZ);
    UART0_SendStr(" Hz\r\n");

    UART0_SendStr("Cap IO         : ");
    UART0_SendDec3((uint16_t)(xo_cfg.reg_data[1] & 0x7F));
    UART0_SendStr("\r\n");

    u32Val = MAKE_U32(rate_bytes[3], rate_bytes[2], rate_bytes[1], rate_bytes[0]);
    u32Val &= ~(1UL << 24);
    UART0_SendStr("Data Rate      : ");
    UART0_SendU32Dec(u32Val);
    UART0_SendStr(" bps\r\n");

    u32Val = MAKE_U32(dev_bytes[3], dev_bytes[2], dev_bytes[1], dev_bytes[0]);
    UART0_SendStr("Deviation      : ");
    UART0_SendU32Dec(u32Val);
    UART0_SendStr(" Hz\r\n");

    u32Val = MAKE_U32(freq_cfg.reg_data[3], freq_cfg.reg_data[2], freq_cfg.reg_data[1], freq_cfg.reg_data[0]);
    UART0_SendStr("Frequency      : ");
    UART0_SendU32Dec(u32Val);
    UART0_SendStr(" Hz\r\n");

    UART0_SendStr("TX Power Level : ");
    UART0_SendDec3((uint16_t)((pwr_cfg.reg_data[3] >> 1) & 0x7F));
    UART0_SendStr("\r\n");

    UART0_SendStr("FIFO Order     : 0x");
    UART0_SendHex8(fifo_cfg.reg_data[0]);
    UART0_SendStr(" (MSB_INV=");
    UART0_SendDec3((uint16_t)(fifo_cfg.reg_data[0] & 0x01));
    UART0_SendStr(")\r\n");

    UART0_SendStr("INT Ena Mask   : 0x");
    UART0_SendHex8(int_cfg.reg_data[3]);
    UART0_SendHex8(int_cfg.reg_data[2]);
    UART0_SendHex8(int_cfg.reg_data[1]);
    UART0_SendHex8(int_cfg.reg_data[0]);
    UART0_SendStr(" (RX=");
    UART0_SendDec3((uint16_t)(IS_INT_ST_RX(int_cfg.reg_data) ? 1 : 0));
    UART0_SendStr(" TX=");
    UART0_SendDec3((uint16_t)(IS_INT_ST_TX(int_cfg.reg_data) ? 1 : 0));
    UART0_SendStr(")\r\n");

    UART0_SendStr("TX FIFO Mode   : 0x");
    UART0_SendHex8(txmode_cfg.reg_data[3]);
    UART0_SendHex8(txmode_cfg.reg_data[2]);
    UART0_SendHex8(txmode_cfg.reg_data[1]);
    UART0_SendHex8(txmode_cfg.reg_data[0]);
    UART0_SendStr("\r\n");

    UART0_SendStr("RX FIFO Mode   : ");
    UART0_SendHex8(gtXtAppConfigInfo.rx_mode);
    UART0_SendStr(" (set by gtXtAppConfigInfo.rx_mode in FW)\r\n");
    UART0_SendStr("Syncword       : 0x");
    UART0_SendU32Hex8((uint32_t)PREDEFINED_RX_SYNCWORD);
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
        
        if(TRx_Init() == TRx_STATUS_SUCCESS)   // Default initialization
        {
            rf_err_mode = 0;
        }
    }
    
    if(!rf_err_mode)
    {
        dump_rf_config();
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
    uint16_t rssi;
    if(rf_err_mode == 1)
        return; 
    
    TRx_GetRSSI_CCA(&rssi);
    UART0_SendStrDec3("rssi(dBm): -", (4095 - rssi)/8, 1);
    Delay_ms(1000); //Delay to allow time for log readout
}
