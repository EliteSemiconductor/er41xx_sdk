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

/* WOR parameter */
// (1) WOR_CNT
//        The duration of RX enable is WOR_CNT*WOR_CNT_UNIT
//        The value range of WOR_CNT_UNIT should be following
//                      0   @200us
//                      1   @400us 
//                      2   @1ms
//                      3   @20ms
#define WOR_CNT_UNIT            3 
#define WOR_CNT                 4
// (2) Wakeup Timer Period Calculation
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
#define WUT_PERIOD_SEL(arr)                 SET_WUT_PERIORD_ARRAY_1S(arr)
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
xdata_u8           rf_err_mode;
t_xtapp_config_t   xdata gtXtAppConfigInfo;
xdata_u8           gXtBuffer[RF_BUF_SIZE];

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
    printf("0x%04X : 0x%02X%02X%02X%02X\r\n", addr, u8Array[3], u8Array[2], u8Array[1], u8Array[0]);
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
        printf("RX(%03u):rssi: -%03u", len, rssi);
    }
    else
    {
        printf("TX(%03u):", len);
    }

    for(ii = 0; ii < len; ii++)
    {
        if((ii & 0x0F) == 0)    // new line every 16 bytes
        {
            printf("\r\n%03u    ", ii);
        }
        printf("%02X ", buf[ii]);
    }

    printf("\r\n");
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
    t_cfg_reg xo_cfg     = { XO_CONFIG_1 };      // XTAL Cap IO
    t_cfg_reg freq_cfg   = { SYNTH_CAL_CONFIG_0 };
    uint8_t   rate_bytes[4] = GLB_DATA_RATE;
    uint8_t   dev_bytes[4]  = GLB_DEVIATION;
    t_cfg_reg pwr_cfg    = { ANCTL_CONFIG_7 };    // TX power level
    t_cfg_reg fifo_cfg   = { FIFO_FEA_REG };      // FIFO data order
    t_cfg_reg int_cfg    = { INT_EN_REG };
    t_cfg_reg txmode_cfg = { MAC_TX_CFG };        // Tx FIFO mode
    uint32_t u32Val;

    printf("===== RF Config from Toolkit =====\r\n");
    printf("Crystal Hz     : %lu Hz\r\n", (unsigned long)CRYSTAL_HZ);
    printf("Cap IO         : %u\r\n", (unsigned)(xo_cfg.reg_data[1] & 0x7F));

    u32Val = MAKE_U32(rate_bytes[3], rate_bytes[2], rate_bytes[1], rate_bytes[0]);
    u32Val &= ~(1UL << 24);
    printf("Data Rate      : %lu bps\r\n", (unsigned long)u32Val);

    u32Val = MAKE_U32(dev_bytes[3], dev_bytes[2], dev_bytes[1], dev_bytes[0]);
    printf("Deviation      : %lu Hz\r\n", (unsigned long)u32Val);

    u32Val = MAKE_U32(freq_cfg.reg_data[3], freq_cfg.reg_data[2], freq_cfg.reg_data[1], freq_cfg.reg_data[0]);
    printf("Frequency      : %lu Hz\r\n", (unsigned long)u32Val);

    printf("TX Power Level : %u\r\n", (unsigned)((pwr_cfg.reg_data[3] >> 1) & 0x7F));
    printf("FIFO Order     : 0x%02X (MSB_INV=%u)\r\n",
           fifo_cfg.reg_data[0], (unsigned)(fifo_cfg.reg_data[0] & 0x01));
    printf("INT Ena Mask   : 0x%02X%02X%02X%02X (RX=%u TX=%u)\r\n",
           int_cfg.reg_data[3], int_cfg.reg_data[2], int_cfg.reg_data[1], int_cfg.reg_data[0],
           (unsigned)(IS_INT_ST_RX(int_cfg.reg_data) ? 1 : 0),
           (unsigned)(IS_INT_ST_TX(int_cfg.reg_data) ? 1 : 0));
    printf("TX FIFO Mode   : 0x%02X\r\n", txmode_cfg.reg_data[0]);
    printf("RX FIFO Mode   : 0x%02X (set by gtXtAppConfigInfo.rx_mode in FW)\r\n", gtXtAppConfigInfo.rx_mode);
    printf("Syncword       : 0x%08lX\r\n", (unsigned long)PREDEFINED_RX_SYNCWORD);
    printf("\r\n");
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
 * @brief  Configure WOR (Wake-On-Radio) timer and enter deep sleep
 *         The transceiver will periodically wake up (per WUT_PERIOD_SEL interval),
 *         open a short RX window (WOR_CNT * WOR_CNT_UNIT), then go back to sleep.
 * @param  None
 * @retval None
 */
void XTAPP_WOR_Trigger(void)
{
    uint8_t  u8Array[4];
    // Set Period
    TRx_RX_WUTMR_Enable(WOR_CNT, WOR_CNT_UNIT);
    WUT_PERIOD_SEL(u8Array);
    TRx_SetWUTMR_Timer(u8Array, 1);
    // Enable RX
    TRx_RX_FIFOReset();
    TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));
    // Enter deep sleep to start WOR flow
    TRx_PowerDownMode();
}
/**
 * @brief  Disable WOR (Wake-On-Radio) timer to stop periodic wakeup
 *         Call this when a valid RX packet is received and WOR is no longer needed,
 *         or before switching to normal continuous RX / TX mode.
 * @param  None
 * @retval None
 */
void XTAPP_WOR_Disable(void)
{
    TRx_RX_WUTMR_Disable();//Set duration of RX window in WOR flow (CNT*20ms)
    TRx_SetWUTMR_Timer(0,0);
}
/**
 * @brief  Initialize RF transceiver and start RX
 * @param  None
 * @retval 0: pass, 1: fail
 */
uint8_t XTAPP_Init(void)
{
    gtXtAppConfigInfo.rx_mode = RX_TRANSPARENT;
    gtXtAppConfigInfo.max_rx_size = RF_BUF_SIZE;
    rf_err_mode = 1;
    
    TRx_IoConfig();
    
    if( XTAPP_Strobe() == 0 )   // Check RF chip present
    {
        TRx_SW_Reset();
        
        if(TRx_Init() == TRx_STATUS_SUCCESS)   // Default initialization
        {
            printf("Waiting 3000 ms for ER4100 to stabilize before the first WOR trigger...\r\n");
            Delay_ms(3000); // The ER4100 needs a stable warm-up period before the first WOR trigger.
            XTAPP_WOR_Trigger();
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
 * @brief  Read received data length and FIFO contents
 * @param  data_len: pointer to store received data length
 * @retval None
 */
void XTAPP_ReceiveCheck(uint16_t* data_len)
{
    uint8_t  u8Array[4];
    TRx_READREG(0xA004, u8Array);
    *data_len = MAKE_U16(u8Array[3], u8Array[2]);
    TRx_RX_FIFO(gXtBuffer);
}

/**
 * @brief  RF IRQ handler: process interrupt events from transceiver
 * @param  None
 * @retval None
 */
void XTAPP_IrqHdlr(void)
{
    uint8_t  u8Array[4];
    TRx_GetIntStatus(u8Array);
    if(IS_INT_ST_ANY(u8Array))
    {
        TRx_ClearIntFlag(u8Array);
        if(IS_INT_ST_ANY_RX(u8Array))
        {   /* RX event */
            xdata_u16 len;
            xdata_u16 u16Rssi;
            XTAPP_WOR_Disable();
            XTAPP_ReceiveCheck(&len);
            TRx_GetRSSI_Data(&u16Rssi);
            u16Rssi = (4095 - u16Rssi) / 8;

            dump_buffer(gXtBuffer, len, 1, u16Rssi);
        }
        else
        {
            TRx_RX_FIFOReset();
        }
        XTAPP_WOR_Trigger();    //restart WOR
    }
}
/**
 * @brief  RF main scan loop: poll NIRQ
 * @param  None
 * @retval None
 */
void XTAPP_Scan(void)
{
    if(rf_err_mode == 1)
        return;
    /* IRQ handler: process RX received / WOR wakeup events */
    if(RF_NIRQ_VAL() == 0)
    {
        XTAPP_IrqHdlr();
    }
}
