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
// Wakeup Timer Period Calculation
//        Register_Value = (Period(ms)*32768)/1000
//        Example1: If period target is 1000ms (1 second)
//                  Calculation: (1000 * 32768) / 1000 = 32768 = 0x00008000
//        Example2: If period target is 2000ms (2 second)
//                  Calculation: (2000 * 32768) / 1000 = 65536 = 0x00010000
#define SET_WUT_PERIOD_ARRAY_10MS(arr)      SET_U8_ARRAY(arr, 0x00, 0x00, 0x01, 0x47)  //10ms
#define SET_WUT_PERIOD_ARRAY_50MS(arr)      SET_U8_ARRAY(arr, 0x00, 0x00, 0x06, 0x66)  //50ms
#define SET_WUT_PERIOD_ARRAY_100MS(arr)     SET_U8_ARRAY(arr, 0x00, 0x00, 0x0C, 0xCC)  //100ms
#define SET_WUT_PERIOD_ARRAY_500MS(arr)     SET_U8_ARRAY(arr, 0x00, 0x00, 0x40, 0x00)  //500ms
#define SET_WUT_PERIOD_ARRAY_1S(arr)        SET_U8_ARRAY(arr, 0x00, 0x00, 0x80, 0x00)  //1000ms
#define SET_WUT_PERIOD_ARRAY_2S(arr)        SET_U8_ARRAY(arr, 0x00, 0x01, 0x00, 0x00)  //2000ms
#define SET_WUT_PERIOD_ARRAY_4S(arr)        SET_U8_ARRAY(arr, 0x00, 0x02, 0x00, 0x00)  //4000ms
#define WUT_PERIOD_SEL_DEFAULT              4       // default period: '4' = 1s
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
xdata_u8           wut_period_sel;      // WUT period index, 0~6 (see XTAPP_WUT_Config)
xdata_u8           wut_periodic_en;     // 1 = periodic mode, 0 = one-shot mode
xdata_u16          wut_evt_cnt;         // number of WUT expiry (TMRTHD) events since last start (wraps after 65535)
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
 * @brief  Print the compiled RF configuration (crystal, cap IO, data rate, deviation,
 *         frequency, TX power, FIFO/interrupt setup, syncword) from the included
 *         SPI_ER41xx_config_*.h to UART.
 * @param  None
 * @retval None
 */
void dump_rf_config(void)
{
    /* addr is unused here; only the register bytes (b0..b3, LSB first) matter */
    t_cfg_reg xo_cfg     = { XO_CONFIG_1 };      // XTAL Cap IO
    t_cfg_reg freq_cfg   = { SYNTH_CAL_CONFIG_0 };
    uint8_t   rate_bytes[4] = GLB_DATA_RATE;
    uint8_t   dev_bytes[4]  = GLB_DEVIATION;
//    t_cfg_reg pa0_cfg    = { PA1G_CONFIG_0 };
//    t_cfg_reg pa1_cfg    = { PA1G_CONFIG_1 };
//    t_cfg_reg pa2_cfg    = { PA1G_CONFIG_2 };
    t_cfg_reg pwr_cfg    = { ANCTL_CONFIG_7 };    // TX power level
    t_cfg_reg fifo_cfg   = { FIFO_FEA_REG };      // FIFO data order
    t_cfg_reg int_cfg    = { INT_EN_REG };
    t_cfg_reg txmode_cfg = { MAC_TX_CFG };        // Tx FIFO mode
    uint32_t u32Val;

    printf("===== RF Config from Toolkit =====\r\n");

    printf("Crystal Hz     : %lu Hz\r\n", (unsigned long)CRYSTAL_HZ);

    /* CAP IO occupies bit6:0 of XO_CONFIG_1 byte1, bit7 is a separate enable flag */
    printf("Cap IO         : %u\r\n", (unsigned)(xo_cfg.reg_data[1] & 0x7F));

    u32Val = MAKE_U32(rate_bytes[3], rate_bytes[2], rate_bytes[1], rate_bytes[0]);
    u32Val &= ~(1UL << 24);   // bit24 is the extended-rate flag, not part of the value
    printf("Data Rate      : %lu bps\r\n", (unsigned long)u32Val);

    u32Val = MAKE_U32(dev_bytes[3], dev_bytes[2], dev_bytes[1], dev_bytes[0]);
    printf("Deviation      : %lu Hz\r\n", (unsigned long)u32Val);

    u32Val = MAKE_U32(freq_cfg.reg_data[3], freq_cfg.reg_data[2], freq_cfg.reg_data[1], freq_cfg.reg_data[0]);
    printf("Frequency      : %lu Hz\r\n", (unsigned long)u32Val);

//    printf("TX Power PA    : PA1G_CFG0=0x%02X%02X%02X%02X PA1G_CFG1=0x%02X%02X%02X%02X PA1G_CFG2=0x%02X%02X%02X%02X\r\n",
//           pa0_cfg.reg_data[3], pa0_cfg.reg_data[2], pa0_cfg.reg_data[1], pa0_cfg.reg_data[0],
//           pa1_cfg.reg_data[3], pa1_cfg.reg_data[2], pa1_cfg.reg_data[1], pa1_cfg.reg_data[0],
//           pa2_cfg.reg_data[3], pa2_cfg.reg_data[2], pa2_cfg.reg_data[1], pa2_cfg.reg_data[0]);

    printf("TX Power Level : %u\r\n", (unsigned)((pwr_cfg.reg_data[3]>>1) & 0x7F));

    printf("FIFO Order     : 0x%02X (MSB_INV=%u)\r\n",
           fifo_cfg.reg_data[0], (unsigned)(fifo_cfg.reg_data[0] & 0x01));

    printf("INT Ena Mask   : 0x%02X%02X%02X%02X (RX=%u TX=%u)\r\n",
           int_cfg.reg_data[3], int_cfg.reg_data[2], int_cfg.reg_data[1], int_cfg.reg_data[0],
           (unsigned)(IS_INT_ST_RX(int_cfg.reg_data) ? 1 : 0),
           (unsigned)(IS_INT_ST_TX(int_cfg.reg_data) ? 1 : 0));

    printf("TX FIFO Mode   : 0x%02X\r\n", txmode_cfg.reg_data[0]);

    printf("RX FIFO Mode   : N/A (RF RX not used in this example)\r\n");

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
 * @brief  Get the display name of a WUT period index
 * @param  sel: period index, 0~6
 * @retval Period name string
 */
const char *XTAPP_WUT_PeriodName(uint8_t sel)
{
    switch(sel)
    {
        case 0:  return "10ms";
        case 1:  return "50ms";
        case 2:  return "100ms";
        case 3:  return "500ms";
        case 4:  return "1s";
        case 5:  return "2s";
        case 6:  return "4s";
        default: return "?";
    }
}
/**
 * @brief  (Re)start the wakeup timer with the current period and mode,
 *         and reset the event counter
 * @param  None
 * @retval None
 */
void XTAPP_WUT_Start(void)
{
    uint8_t  u8Array[4];

    if(rf_err_mode == 1)
        return;

    switch(wut_period_sel)
    {
        case 0:  SET_WUT_PERIOD_ARRAY_10MS(u8Array);   break;
        case 1:  SET_WUT_PERIOD_ARRAY_50MS(u8Array);   break;
        case 2:  SET_WUT_PERIOD_ARRAY_100MS(u8Array);  break;
        case 3:  SET_WUT_PERIOD_ARRAY_500MS(u8Array);  break;
        case 4:  SET_WUT_PERIOD_ARRAY_1S(u8Array);     break;
        case 5:  SET_WUT_PERIOD_ARRAY_2S(u8Array);     break;
        default: SET_WUT_PERIOD_ARRAY_4S(u8Array);     break;
    }
    wut_evt_cnt = 0;

    printf("WUT starting : period = %s%s", XTAPP_WUT_PeriodName(wut_period_sel),
           wut_periodic_en ? ", mode = Periodic\r\n" : ", mode = One-shot\r\n");

    if(TRx_SetWUTMR_Timer(u8Array, wut_periodic_en) != TRx_STATUS_SUCCESS)
    {
        printf("WUT start fail\r\n");
        return;
    }
}
/**
 * @brief  Select WUT period and mode, then restart the wakeup timer
 * @param  sel:         period index, 0 = 10ms, 1 = 50ms, 2 = 100ms, 3 = 500ms,
 *                                    4 = 1s,   5 = 2s,   6 = 4s
 *         periodic_en: 1 = periodic mode, 0 = one-shot mode
 * @retval None
 */
void XTAPP_WUT_Config(uint8_t sel, uint8_t periodic_en)
{
    if(sel > 6)
        return;
    wut_period_sel  = sel;
    wut_periodic_en = periodic_en ? 1 : 0;
    XTAPP_WUT_Start();
}
/**
 * @brief  Stop the wakeup timer (both periodic and one-shot mode)
 * @param  None
 * @retval None
 */
void XTAPP_WUT_Stop(void)
{
    uint8_t  u8Array[4];
    uint8_t  try_cnt = 100;

    // Clear the enable / periodic flags in 0x8000 to disable the wakeup timer,
    // then wait for the timer hardware to become ready (0x8004 = 0)
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    TRx_WRITEREG(0x8000, u8Array);
    do
    {
        TRx_READREG(0x8004, u8Array);
        if(COMPARE_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00))
        {
            printf("WUT stop\r\n");
            return;
        }
        Delay_ms(1);
    } while(--try_cnt);

    printf("WUT stop fail\r\n");
}
/**
 * @brief  Initialize RF transceiver and enable the WUT expiry (TMRTHD) interrupt.
 *         The chip stays in active mode; call XTAPP_WUT_Start() to start the timer.
 * @param  None
 * @retval 0: pass, 1: fail
 */
uint8_t XTAPP_Init(void)
{
    rf_err_mode     = 1;
    wut_period_sel  = WUT_PERIOD_SEL_DEFAULT;
    wut_periodic_en = 1;
    wut_evt_cnt     = 0;
    TRx_IoConfig();
    if( XTAPP_Strobe() == 0 )   // Check RF chip present
    {
        TRx_SW_Reset();
        if(TRx_Init() == TRx_STATUS_SUCCESS)   // Default initialization
        {
            uint8_t  u8Array[4];
            XTAPP_WUT_Stop();	//Clear timer register
            // Enable INT_ST_TMRTHD in the INT mask (0xF004), so every WUT expiry
            // raises NIRQ while the chip stays in active mode
            TRx_READREG(0xF004, u8Array);
            SET_INT_ST_TMRTHD(u8Array);
            TRx_WRITEREG(0xF004, u8Array);
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
 * @brief  RF main scan loop: poll NIRQ and report WUT expiry (TMRTHD) events
 * @param  None
 * @retval None
 */
void XTAPP_Scan(void)
{
    if(rf_err_mode == 1)
        return;
    /* IRQ handler: process WUT expiry event */
    if(RF_NIRQ_VAL() == 0)
    {
        // Read and clear interrupt flags
        uint8_t  u8Array[4];
        TRx_GetIntStatus(u8Array);
        if(IS_INT_ST_ANY(u8Array))
        {
            TRx_ClearIntFlag(u8Array);
            if(IS_INT_ST_TMRTHD(u8Array))
            {   /* Wakeup timer expired */
                wut_evt_cnt++;
                printf("WUT event : %u%s", (unsigned int)wut_evt_cnt,
                       wut_periodic_en ? ", key in 'p' to stop periodic\r\n" : "\r\n");
                if(!wut_periodic_en)
                {
                    printf("One-shot done\r\n");
                }
            }
        }
    }
}
