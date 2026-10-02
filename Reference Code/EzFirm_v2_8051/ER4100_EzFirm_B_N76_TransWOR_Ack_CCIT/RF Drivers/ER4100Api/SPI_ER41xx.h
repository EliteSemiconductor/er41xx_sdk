/**************************************************************************/
/**
 * @file     SPI_ER41xx.h
 * @version  V4.0.5 for other crystal
 * @brief    ER41xx series SPI driver header file
 *
 * @copyright (C) COPYRIGHT 2020 ESMT Technology Corp. All rights reserved.
 *****************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_ER41XX_H__
#define __SPI_ER41XX_H__
#define SDK_EZ_8BIT
/* Includes ------------------------------------------------------------------*/
#include "SPI_ER41xx_config.h"
/* Definition & Macro --------------------------------------------------------*/    
#define BASIC_API_VER             "V4.05.10"
/* Extend RF APIs */
#undef EXTERN_RF_APIS_PCR
#define EXTERN_RF_APIS_WUT
#undef EXTERN_RF_APIS_SINGLE_TONE
#undef EXTERN_RF_APIS_PN9    
#undef EXTERN_RF_APIS_FORMAT
#undef EXTERN_RF_APIS_CH
#undef EXTERN_RF_APIS_CCA
#undef EXTERN_RF_APIS_GPIO
/* Type convert */
#define MAKE_U16(msb, lsb) \
    ( ((uint16_t)(msb) << 8) | ((uint16_t)(lsb) & 0xFF) )
    
#define MAX_VAL(a, b)        (((a) > (b)) ? (a) : (b))
#define MAX3_VAL(a, b, c)    MAX_VAL(MAX_VAL(a, b), c)
#define MAX4_VAL(a, b, c, d) MAX_VAL(MAX3_VAL(a, b, c), d)
    
#define MAKE_U32(b3, b2, b1, b0) \
    ( ((uint32_t)(b3) << 24) | \
      ((uint32_t)(b2) << 16) | \
      ((uint32_t)(b1) << 8)  | \
      ((uint32_t)(b0) & 0xFF) )

/* Utility to access array */
#define SET_U8_ARRAY(arr, b3, b2, b1, b0)     \
    do {                                      \
        (arr)[3] = (uint8_t)(b3);             \
        (arr)[2] = (uint8_t)(b2);             \
        (arr)[1] = (uint8_t)(b1);             \
        (arr)[0] = (uint8_t)(b0);             \
    } while (0)
    
#define AND_U8_ARRAY(arr, b3, b2, b1, b0)     \
    do {                                      \
        (arr)[3] &= (uint8_t)(b3);             \
        (arr)[2] &= (uint8_t)(b2);             \
        (arr)[1] &= (uint8_t)(b1);             \
        (arr)[0] &= (uint8_t)(b0);             \
    } while (0)
    
#define OR_U8_ARRAY(arr, b3, b2, b1, b0)     \
    do {                                      \
        (arr)[3] |= (uint8_t)(b3);             \
        (arr)[2] |= (uint8_t)(b2);             \
        (arr)[1] |= (uint8_t)(b1);             \
        (arr)[0] |= (uint8_t)(b0);             \
    } while (0)
    
#define COMPARE_U8_ARRAY(arr, b3, b2, b1, b0) ( \
    ((arr)[3] == (uint8_t)(b3)) && \
    ((arr)[2] == (uint8_t)(b2)) && \
    ((arr)[1] == (uint8_t)(b1)) && \
    ((arr)[0] == (uint8_t)(b0))    \
)
    
#define TRX_WRITEREG_U32(addr, b3, b2, b1, b0)   \
    do {                                        \
        uint8_t _buf[4];                        \
        SET_U8_ARRAY(_buf, b3, b2, b1, b0);     \
        TRx_WRITEREG((addr), _buf);             \
    } while (0)

/* Access OpCode */
#define COMM_NOP                  0x00
#define COMM_SETINT               0x0B
#define COMM_CLEARSTATUS          0x0C
#define COMM_GETSTATUSINT         0x0D
#define COMM_CHANGEMODE           0x0E
#define COMM_RSTFIFO              0x18
#define COMM_TXTrigger            0x20
#define COMM_RXTrigger            0x21
#define COMM_WRITEREG             0x90
#define COMM_READREG              0x91
#define COMM_TXFIFO               0xC0
#define COMM_RXFIFO               0xD0

/* Interrupt event operation */
#define INT_ST_RX_BIT                 (0)
#define INT_ST_RXERR_BIT              (1)
#define INT_ST_TX_BIT                 (2)
#define INT_ST_TXERR_BIT              (3)
#define INT_ST_TMRTHD_BIT             (4)
#define INT_ST_WAKEUP_BIT             (5)
#define INT_ST_NFC_BIT                (6)
#define INT_ST_TX_FIFO_AEMPTY_BIT     (7)
#define INT_ST_RX_FIFO_AFULL_BIT      (8)
#define INT_ST_RX_SYNCW_BIT           (9)
// Query (IS_)
#define IS_INT_ST_RX(arr)             ((arr)[0] & 0x01)
#define IS_INT_ST_RXERR(arr)          ((arr)[0] & 0x02)
#define IS_INT_ST_TX(arr)             ((arr)[0] & 0x04)
#define IS_INT_ST_TXERR(arr)          ((arr)[0] & 0x08)
#define IS_INT_ST_TMRTHD(arr)         ((arr)[0] & 0x10)

#define IS_INT_ST_WAKEUP(arr)         ((arr)[1] & 0x01)
#define IS_INT_ST_NFC(arr)            ((arr)[1] & 0x02)
#define IS_INT_ST_TX_FIFO_AEMPTY(arr) ((arr)[1] & 0x04)
#define IS_INT_ST_RX_FIFO_AFULL(arr)  ((arr)[1] & 0x08)
#define IS_INT_ST_RX_SYNCW(arr)       ((arr)[1] & 0x10)
#define IS_INT_ST_ANY_RX(arr)          \
    ( IS_INT_ST_RX(arr)            ||  \
      IS_INT_ST_RXERR(arr)         ||  \
      IS_INT_ST_RX_FIFO_AFULL(arr) ||  \
      IS_INT_ST_RX_SYNCW(arr) )
#define IS_INT_ST_ANY(arr) \
    ( ((arr)[0] | (arr)[1] | (arr)[2] | (arr)[3]) != 0 )
// Clear 
#define CLR_INT_ST_RX(arr)             do { (arr)[0] &= (uint8_t)~0x01; } while (0)
#define CLR_INT_ST_RXERR(arr)          do { (arr)[0] &= (uint8_t)~0x02; } while (0)
#define CLR_INT_ST_TX(arr)             do { (arr)[0] &= (uint8_t)~0x04; } while (0)
#define CLR_INT_ST_TXERR(arr)          do { (arr)[0] &= (uint8_t)~0x08; } while (0)
#define CLR_INT_ST_TMRTHD(arr)         do { (arr)[0] &= (uint8_t)~0x10; } while (0)
#define CLR_INT_ST_WAKEUP(arr)         do { (arr)[1] &= (uint8_t)~0x01; } while (0)
#define CLR_INT_ST_NFC(arr)            do { (arr)[1] &= (uint8_t)~0x02; } while (0)
#define CLR_INT_ST_TX_FIFO_AEMPTY(arr) do { (arr)[1] &= (uint8_t)~0x04; } while (0)
#define CLR_INT_ST_RX_FIFO_AFULL(arr)  do { (arr)[1] &= (uint8_t)~0x08; } while (0)
#define CLR_INT_ST_RX_SYNCW(arr)       do { (arr)[1] &= (uint8_t)~0x10; } while (0)
#define CLEAR_INT_ST_ALL(arr)          \
    do {                               \
        (arr)[0] = 0;                  \
        (arr)[1] = 0;                  \
        (arr)[2] = 0;                  \
        (arr)[3] = 0;                  \
    } while (0)
#define INT_MASK_CLR_BIT(arr, bit)           \
    do {                                     \
        (arr)[(bit) >> 3] &= (uint8_t)~(1U << ((bit) & 0x07)); \
    } while (0)    
// Set (SET_) 
#define SET_INT_ST_RX(arr)              do { (arr)[0] |= 0x01; } while (0)
#define SET_INT_ST_RXERR(arr)           do { (arr)[0] |= 0x02; } while (0)
#define SET_INT_ST_TX(arr)              do { (arr)[0] |= 0x04; } while (0)
#define SET_INT_ST_TXERR(arr)           do { (arr)[0] |= 0x08; } while (0)
#define SET_INT_ST_TMRTHD(arr)          do { (arr)[0] |= 0x10; } while (0)
// bit5~bit7 reserved
#define SET_INT_ST_WAKEUP(arr)          do { (arr)[1] |= 0x01; } while (0)
#define SET_INT_ST_NFC(arr)             do { (arr)[1] |= 0x02; } while (0)
#define SET_INT_ST_TX_FIFO_AEMPTY(arr)  do { (arr)[1] |= 0x04; } while (0)
#define SET_INT_ST_RX_FIFO_AFULL(arr)   do { (arr)[1] |= 0x08; } while (0)
#define SET_INT_ST_RX_SYNCW(arr)        do { (arr)[1] |= 0x10; } while (0)
// bit13~bit15 reserved 
#define INT_MASK_SET_BIT(arr, bit)           \
    do {                                     \
        (arr)[(bit) >> 3] |= (uint8_t)(1U << ((bit) & 0x07)); \
    } while (0)

    
/* RF Config */
// Modulation type
#define TX_BPSK_MODE              0x0    //!<	TRx Tx BPSK modulation
#define TX_FSK_MODE               0x3    //!<	TRx Tx FSK modulation
#define TX_GFSK_MODE              0x4    //!<	TRx Tx FSK modulation
// TX MAC Type
#define TX_TRANSPARENT            0x0    //!<	Transparent mode without preamble/SyncWord
#define TX_TRANSPARENTwithPRESYNC 0x3    //!<	Transparent mode with preamble/SyncWord
#define TX_802154withPRESYNC      0xF    //!<	802.15.4 mode with preamble/SyncWord
// RX MAC Type
#define RX_802154                 0    //!<	802.15.4 mode with preamble/SyncWord
#define RX_TRANSPARENT            1    //!<	Transparent mode with preamble/SyncWord
// TX CCA RSSI Threshold
#define TX_CCA_RSSI_60dBmTHD      0xE1F    //!<	-60dBm RSSI threshold in TX CCA mode
#define TX_CCA_RSSI_80dBmTHD      0xD7F    //!<	-80dBm RSSI threshold in TX CCA mode
//FIFO size
#define XT_MAX_FIFO_SIZE          128
/* Typedef -----------------------------------------------------------*/
typedef enum
{
    OUTPUT_OFF = 0,
    OUTPUT_ON,
} ONOFF_OUTPUT;

typedef enum
{
    AGC_MODE_DIG    = 1,
    AGC_MODE_RF_DIG = 2,
    AGC_MODE_DEBUG  = 3,
} AGC_MODE;

typedef enum
{
    GAIN_SRC_ANA = 0,
    GAIN_SRC_BBP = 1,
} GAIN_SRC;

typedef enum
{
    TRx_STATUS_SUCCESS = 0,
    TRx_STATUS_TO_FAIL,
    TRx_STATUS_FAIL,
} STATUS_TRx;

typedef enum
{
    TR_BSY = 0,      //!<	TX /RX in progress (WTR)
    TR_FSH,          //!<  TX /RX completion (VPOAK)
    TX_PKT,          //!<  TX packet in progress (preamble + sync word + data) (TMEO)
    TX_DATA,         //!<  TX data in progress (data) (EOAC)
    TX_PKT_O,        //!<  TX packet steaming output (FMTDO)
    RX_CD,           //!<  RX carrier detection (CD)
    RX_SYNC,         //!<  RX data in progress (data) (FSYNC)
    FIFO_EXH,        //!<  FIFO TX almost empty or RX amost full (FPF)
    TX_DIO_DATA,     //!<  TX direct mode's data (TXD)
    TX_DIO_CLK,      //!<  TX direct mode's clock (DCK)
    RX_DIO_DATA,     //!<  RX direct mode's data (RXD)
    RX_DIO_CLK,      //!<  RX direct mode's clock (RCK)
    EVT_WAKEUP,      //!<  Wakeup timer's IRQ (Sleep INT)
    TX_EN,           //!<  TX to usage of external antenna switch
    NFC_FIELD_OK,    //!<  NFC field detect
    NFC_BUSY,        //!<  NFC operation in progress
} TRx_GPIOSel_EnumDef;

/**
 * GPIOx_CFG_REG (addr 0x4000: GPIO0, 0x4004: GPIO1) bit layout:
 *   bit3:0   GPIOx_GIO_SEL   - GIO selection (see TRx_GPIOSel_EnumDef)
 *   bit7     GPIOx_GIO_INV   - GIO inverse
 *   bit10:8  GPIOx_MODE      - 0:Default 1:GIO 2-3:Reserved 4:RX RF 5:RX BBP 6:NFC 7:Debug
 *   bit17:16 GPIOx_DS        - Driving strength: 0:4.5mA 1:9mA 2:13.5mA 3:18mA
 *   bit18    GPIOx_SMT       - Schmitt trigger: 1=Enable 0=Disable
 *   bit20:19 GPIOx_PULL_CTRL - Pull control: 0=Pull up 1=Pull down 2=No pull
 *   bit27:24 GPIOx_DBG_NUM   - Debug bus bit selected for output, valid when MODE = Debug mode
 */

/* Function --------------------------------------------------------*/
/**
 * @brief  Initialize the DUT according to the Init_array.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_Init(void);

/**
 * @brief  Send NOP command to transceiver.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_NOP(void);

/**
 * @brief  Write a 4-byte value to a register address of DUT.
 * @param  Addr:    16-bit register address (uint16_t)
 *         TRxData: 4-byte data buffer to write (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_WRITEREG(uint16_t Addr, uint8_t* TRxData );

/**
 * @brief  Read a 4-byte value from a register address of DUT.
 * @param  Addr:    16-bit register address (uint16_t)
 *         TRxData: 4-byte buffer to store the read value (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_READREG(uint16_t Addr, uint8_t* TRxData );

/**
 * @brief  Transmit data from DUT's TX FIFO.
 * @param  DataLen_byte: number of bytes to transmit (uint16_t, max 2047)
 *         CCAEn:        1 = enable CCA mode before TX, 0 = disable (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_TX_Trigger(uint16_t DataLen_byte, uint8_t CCAEn);

/**
 * @brief  Trigger single tone (continuous wave) transmission.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SingleTone_Trigger(void);

/**
 * @brief  Disable TX single tone (continuous wave) mode.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SingleTone_Disable(void);

/**
 * @brief  Enable PRBS9 pseudo-random bit sequence continuous transmission.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PRBS9_Trigger(void);

/**
 * @brief  Turn off DUT's RX.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RxOff(void);

/**
 * @brief  Start DUT's RX and wait until receiver is ready.
 * @param  DataLen_byte:   expected RX data length in bytes (uint16_t, max 2047)
 *         Mode802154En:   1 = 802.15.4 mode (validate protocol), 0 = transparent (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_Trigger( uint16_t DataLen_byte, uint8_t Mode802154En );

/**
 * @brief  Enable RX wake-up timer (WOR mode); DUT enters power saving after timeout.
 * @param  TOTcnt:      timeout count multiplier (uint16_t, max 0x3F)
 *         TOTcnt_unit: timeout unit — 0:200us, 1:400us, 2:1ms, 3:20ms (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_WUTMR_Enable(uint16_t TOTcnt, uint8_t TOTcnt_unit);

/**
 * @brief  Disable RX wake-up timer (WOR mode).
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_WUTMR_Disable(void);

/**
 * @brief  Configure the Wake-up Timer (WUTMR) value and operation mode.
 * @param  u8TimerValArr: pre-calculated 4-byte timer value, Little-Endian (uint8_t*)
 *           Formula: RegisterValue = (Time_ms * 32768) / 1000
 *           Example 1 - 1000ms: (1000*32768)/1000 = 32768 = 0x00008000 → {0x00,0x80,0x00,0x00}
 *           Example 2 -  100ms: (100*32768)/1000  = 3277  = 0x00000CCD → {0xCD,0x0C,0x00,0x00}
 *         PeriodicEn: 1 = periodic mode, 0 = one-shot mode (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL if hardware polling times out
 */
STATUS_TRx TRx_SetWUTMR_Timer(uint8_t* u8TimerValArr, uint8_t PeriodicEn);

/**
 * @brief  Get DUT's RSSI value of the last received packet.
 * @param  RSSI_dBm: pointer to store the raw RSSI result (uint16_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GetRSSI_Data(uint16_t* RSSI_dBm );

/**
 * @brief  Enable transceiver interrupts using a 4-byte mask array.
 * @param  IntMask: 4-byte interrupt enable mask array (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_EnableInt(uint8_t *IntMask );

/**
 * @brief  Clear DUT's interrupt flags.
 * @param  TRxData: 4-byte interrupt status array indicating which flags to clear (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_ClearIntFlag(uint8_t* TRxData );

/**
 * @brief  Get transceiver interrupt status.
 * @param  TRxData: 4-byte buffer to store interrupt status (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GetIntStatus(uint8_t* TRxData );

/**
 * @brief  Write DUT's GPIOx_CFG_REG with a fully-specified configuration.
 * @param  addr:      target register address, 0x4000=GPIO0, 0x4004=GPIO1 (uint16_t)
 *         gio_sel:   bit3:0   GIO selection (TRx_GPIOSel_EnumDef)
 *         gio_inv:   bit7     GIO inverse, 0/1
 *         mode:      bit10:8  0:Default 1:GIO 2-3:Reserved 4:RX RF 5:RX BBP 6:NFC 7:Debug
 *         ds:        bit17:16 Driving strength: 0:4.5mA 1:9mA 2:13.5mA 3:18mA
 *         smt:       bit18    Schmitt trigger: 1=Enable 0=Disable
 *         pull_ctrl: bit20:19 Pull control: 0=Pull up 1=Pull down 2=No pull
 *         dbg_num:   bit27:24 Debug bus bit selected for output, valid when mode = Debug mode
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GPIO_GeneralContrl(uint16_t addr, TRx_GPIOSel_EnumDef gio_sel, uint8_t gio_inv,
                                  uint8_t mode, uint8_t ds, uint8_t smt, uint8_t pull_ctrl, uint8_t dbg_num);

/**
 * @brief  Switch DUT's GPIO0 between GIO mode and default mode.
 * @param  enable:  1 switches GPIO_0_MODE to 1 (GIO mode) and outputs gio_sel;
 *                  0 switches GPIO_0_MODE back to 0 (default mode) — GPIO0's default
 *                  mode is the TCXO enable pin (default high)
 *         gio_sel: GPIO0 GIO selection (TRx_GPIOSel_EnumDef), only applied when enable = 1
 *                  TR_BSY, TR_FSH, TX_PKT, TX_DATA, TX_PKT_O, RX_CD, RX_SYNC, FIFO_EXH,
 *                  TX_DIO_DATA, TX_DIO_CLK, RX_DIO_DATA, RX_DIO_CLK, EVT_WAKEUP, TX_EN,
 *                  NFC_FIELD_OK, NFC_BUSY
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GPIO0_Sel(uint8_t enable, TRx_GPIOSel_EnumDef gio_sel);

/**
 * @brief  Switch DUT's GPIO1 between GIO mode and default mode.
 * @param  enable:  1 switches GPIO_1_MODE to 1 (GIO mode) and outputs gio_sel;
 *                  0 switches GPIO_1_MODE back to 0 (default mode) — GPIO1's default
 *                  mode is the shutdown pin (default input); the shutdown function only
 *                  takes effect once separately enabled in PCR
 *         gio_sel: GPIO1 GIO selection (TRx_GPIOSel_EnumDef), only applied when enable = 1
 *                  TR_BSY, TR_FSH, TX_PKT, TX_DATA, TX_PKT_O, RX_CD, RX_SYNC, FIFO_EXH,
 *                  TX_DIO_DATA, TX_DIO_CLK, RX_DIO_DATA, RX_DIO_CLK, EVT_WAKEUP, TX_EN,
 *                  NFC_FIELD_OK, NFC_BUSY
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GPIO1_Sel(uint8_t enable, TRx_GPIOSel_EnumDef gio_sel);

/**
 * @brief  Enable the PCRMU function (excluding NFC) for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_Enable_withoutNFC(void);

/**
 * @brief  Enable the PCRMU function for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_Enable(void);

/**
 * @brief  Disable the PCRMU function for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_Disable(void);

/**
 * @brief  Enable the shutdown function for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_ShutDown_Enable(void);

/**
 * @brief  Disable the shutdown function for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_ShutDown_Disable(void);

/**
 * @brief  Set DUT into power saving (power down) mode.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PowerDownMode(void);

/**
 * @brief  Trigger DUT software reset.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SW_Reset(void);

/**
 * @brief  Set DUT's working channel by frequency.
 * @param  u32Frequency_Hz: target frequency in Hz (uint32_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SetChannel( uint32_t u32Frequency_Hz );

/**
 * @brief  Select a pre-defined channel from the channel table by index.
 * @param  ch_idx: index of the target channel in the lookup table (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SelectChannel( uint8_t ch_idx );

/**
 * @brief  Write data into DUT's TX FIFO.
 * @param  WRITESIZE: number of bytes to write (uint16_t, max 128)
 *         txpbuff:   pointer to TX data buffer (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_TX_FIFO(uint16_t WRITESIZE, uint8_t* txpbuff );

/**
 * @brief  Read all available data from DUT's RX FIFO (length read from register 0xA004).
 * @param  rxpbuff: pointer to buffer to store received data (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_FIFO(uint8_t* rxpbuff );

/**
 * @brief  Read up to READSIZE bytes from DUT's RX FIFO.
 * @param  READSIZE: maximum number of bytes to read (uint16_t)
 *         rxpbuff:  pointer to buffer to store received data (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_FIFOTHD(uint16_t READSIZE, uint8_t* rxpbuff );

/**
 * @brief  Set DUT's TX FIFO almost-empty and RX FIFO almost-full thresholds.
 * @param  TxThreshold:  TX FIFO almost-empty threshold in bytes (uint8_t, max 128)
 *         uRxThreshold: RX FIFO almost-full threshold in bytes (uint8_t, max 128)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_FIFOTHD(uint8_t TxThreshold, uint8_t uRxThreshold );

/**
 * @brief  Reset DUT's TX FIFO content.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_TX_FIFOReset(void);

/**
 * @brief  Set RX sync word for frame start detection.
 * @param  SyncWord: 4-byte sync word array (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_SyncWord(uint8_t *SyncWord );

/**
 * @brief  Reset DUT's RX FIFO content.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_FIFOReset(void);

/**
 * @brief  Configure CCA (Clear Channel Assessment) RSSI threshold.
 * @param  u16CCA_RSSITHD: 12-bit RSSI threshold value (uint16_t)
 *           e.g. TX_CCA_RSSI_60dBmTHD (0xE1F) or TX_CCA_RSSI_80dBmTHD (0xD7F)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_CCA_Config(uint16_t u16CCA_RSSITHD);

/**
 * @brief  Trigger a CCA RSSI measurement and return the result.
 * @param  u16RSSI_dBm: pointer to store the 16-bit raw RSSI result (uint16_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1 (timeout), TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GetRSSI_CCA(uint16_t *u16RSSI_dBm);

#endif /*__SPI_ER41XX_H__*/

/*** (C) COPYRIGHT 2020 ESMT Technology Corp. ***/
