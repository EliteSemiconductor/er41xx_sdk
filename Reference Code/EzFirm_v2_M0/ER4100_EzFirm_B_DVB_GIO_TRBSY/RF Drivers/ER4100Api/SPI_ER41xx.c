/**************************************************************************/
/**
 * @file     SPI_ER41xx.c
 * @version  V4.0.5 for other crystal
 * @brief    ER41xx series SPI driver file
 *
 * @copyright (C) COPYRIGHT 2020 ESMT Technology Corp. All rights reserved.
 *****************************************************************************/
#include "Common.h"
#include "compiler_compat.h"
#include "SPI_ER41xx.h"
/* Private define ------------------------------------------------------------*/
#define TimerOutCnt 5000
/* Private macro ------------------------------------------------------------*/
#define IS_TABLE_END(e) \
     ((e).addr == END_REG_INT && \
     (e).reg_data[0] == END_REG_CHAR && \
     (e).reg_data[1] == END_REG_CHAR && \
     (e).reg_data[2] == END_REG_CHAR && \
     (e).reg_data[3] == END_REG_CHAR)
/* Private struct ---------------------------------------------------------*/
typedef struct {
    uint16_t addr;
    uint8_t reg_data[4];
    //reg_data[0] is bit7:0 of uint32_t, lowest byte of register data
    //reg_data[1] is bit15:8 of uint32_t
    //reg_data[2] is bit23:16 of uint32_t
    //reg_data[3] is bit31:24 of uint32_t, highest byte of register data
}t_reg_array;

typedef struct {
    uint8_t freq[4];
	//freq[0] is bit7:0 of uint32_t, lowest byte of frequency 
    //freq[1] is bit15:8 of uint32_t
    //freq[2] is bit23:16 of uint32_t
    //freq[3] is bit31:24 of uint32_t, highest byte of frequency
	
    uint8_t reg_1998[4];
    uint8_t reg_199C[4];
    uint8_t reg_19A0[4];
    uint8_t reg_1994[4];
		//reg_19xx[0] is bit7:0 of uint32_t, lowest byte of register data
    //reg_19xx[1] is bit15:8 of uint32_t
    //reg_19xx[2] is bit23:16 of uint32_t
    //reg_19xx[3] is bit31:24 of uint32_t, highest byte of register data
}ch_pll_item_t;
/* Private variables ---------------------------------------------------------*/
//   Configuration  (default value)
#define Crystal_Hz CRYSTAL_HZ
#define HW_Clk_MHz 40
#define Crystal_MHz 38.4
xdata_u8 gTRx_DataRate[4] = GLB_DATA_RATE;
xdata_u8 gTRx_FreqDev[4] = GLB_DEVIATION;

_D_CODE const ch_pll_item_t channel_pll_table[] = CHANNEL_PLL_TABLE_ARRAY;
_D_CODE const t_reg_array Initial_Reg_Array[] = RF_REG_INIT_ARRAY;
_D_CODE const t_reg_array TRx_Config_Array[] = RF_REG_CONFIG_ARRAY;
_D_CODE const t_reg_array TRx_ConfigEx_Array[] = RF_REG_CONFIG_EX_ARRAY;
/* Private function prototypes -----------------------------------------------*/
uint8_t * Set_U8_Array(uint8_t* src_arr, 
                    uint8_t b3, 
                    uint8_t b2, 
                    uint8_t b1, 
                    uint8_t b0)
{
    SET_U8_ARRAY(src_arr, b3, b2, b1, b0);
    return src_arr;
}

/**
 * @brief  Send NOP command to transceiver.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_NOP(void)
{
    uint8_t u8Array[4];
    if (!(TRx_Write(COMM_NOP, u8Array, 0)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Write a 4-byte value to a register address of DUT.
 * @param  Addr:    16-bit register address (uint16_t)
 *         TRxData: 4-byte data buffer to write (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_WRITEREG(uint16_t Addr, const uint8_t *TRxData)
{
    //TRxData[0] is bit7:0 of uint32_t
    //TRxData[1] is bit15:8 of uint32_t
    //TRxData[2] is bit23:16 of uint32_t
    //TRxData[3] is bit31:24 of uint32_t
    if (!(TRx_WriteAddr(COMM_WRITEREG, TRxData, Addr)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Read a 4-byte value from a register address of DUT.
 * @param  Addr:    16-bit register address (uint16_t)
 *         TRxData: 4-byte buffer to store the read value (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_READREG(uint16_t Addr, uint8_t *TRxData)
{
    //TRxData[0] is bit7:0 of uint32_t
    //TRxData[1] is bit15:8 of uint32_t
    //TRxData[2] is bit23:16 of uint32_t
    //TRxData[3] is bit31:24 of uint32_t
    if (!(TRx_ReadAddr(COMM_READREG, TRxData, Addr)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Enable transceiver interrupts using a 4-byte mask array.
 * @param  IntMask: 4-byte interrupt enable mask array (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_EnableInt(uint8_t *IntMask)
{
    if (!(TRx_Write(COMM_SETINT, IntMask, 4)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Clear DUT's interrupt flags.
 * @param  TRxData: 4-byte interrupt status array indicating which flags to clear (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_ClearIntFlag(uint8_t *TRxData)
{
    if (!(TRx_Write(COMM_CLEARSTATUS, TRxData, 4)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Get transceiver interrupt status.
 * @param  TRxData: 4-byte buffer to store interrupt status (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GetIntStatus(uint8_t *TRxData)
{
    if (!(TRx_Read(COMM_GETSTATUSINT, TRxData, 4)))
    {
        return TRx_STATUS_SUCCESS;
    }
    return TRx_STATUS_FAIL;
}
#ifdef EXTERN_RF_APIS_GPIO
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
                                  uint8_t mode, uint8_t ds, uint8_t smt, uint8_t pull_ctrl, uint8_t dbg_num)
{
    uint8_t u8Array[4];

    /* b3: bit27:24 DBG_NUM */
    /* b2: bit20:19 PULL_CTRL, bit18 SMT, bit17:16 DS */
    /* b1: bit10:8 MODE */
    /* b0: bit7 GIO_INV, bit3:0 GIO_SEL */
    SET_U8_ARRAY(u8Array,
                 (dbg_num & 0x0F),
                 ((pull_ctrl & 0x03) << 3) | ((smt & 0x01) << 2) | (ds & 0x03),
                 (mode & 0x07),
                 ((gio_inv & 0x01) << 7) | (gio_sel & 0x0F));
    return TRx_WRITEREG(addr, u8Array);
}

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
STATUS_TRx TRx_GPIO0_Sel(uint8_t enable, TRx_GPIOSel_EnumDef gio_sel)
{
    uint8_t u8Array[4];

    if (enable)
        SET_U8_ARRAY(u8Array, 0x00, 0x16, 0x01, gio_sel);
    else
        SET_U8_ARRAY(u8Array, 0x00, 0x16, 0x00, 0x00);
    return TRx_WRITEREG(0x4000, u8Array);
}

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
STATUS_TRx TRx_GPIO1_Sel(uint8_t enable, TRx_GPIOSel_EnumDef gio_sel)
{
    uint8_t u8Array[4];

    if (enable)
        SET_U8_ARRAY(u8Array, 0x00, 0x16, 0x01, gio_sel);
    else
        SET_U8_ARRAY(u8Array, 0x00, 0x16, 0x00, 0x00);
    return TRx_WRITEREG(0x4004, u8Array);
}
#endif
/**
 * @brief  Set DUT into power saving (power down) mode.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PowerDownMode(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0, 0, 0, 3);
    if (!(TRx_Write(COMM_CHANGEMODE, u8Array, 1)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

#ifdef EXTERN_RF_APIS_PCR
/* PCR management Application APIs */
/**
 * @brief  Enable the PCRMU function for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_Enable(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0, 0, 0, 4);
    TRx_WRITEREG(0x0020, u8Array);
    SET_U8_ARRAY(u8Array, 0, 0, 0, 0);
    TRx_WRITEREG(0x0028, u8Array);
    SET_U8_ARRAY(u8Array, 0xff, 0xff, 0xff, 0xff);
    TRx_WRITEREG(0x1a00, u8Array);
    return TRx_WRITEREG(0x1a08, u8Array);
}

/**
 * @brief  Disable the PCRMU function for mode change.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PCRMU_Disable(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0xff, 0xff, 0xff, 0xff);
    TRx_WRITEREG(0x0020, u8Array);
    SET_U8_ARRAY(u8Array, 0, 0, 0, 0);
    TRx_WRITEREG(0x1a00, u8Array);
    return TRx_WRITEREG(0x1a08, u8Array);
}
#endif

#ifdef EXTERN_RF_APIS_SINGLE_TONE
/* Single tone (continoue wave) Application APIs */
/**
 * @brief  Trigger single tone (continuous wave) transmission.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SingleTone_Trigger(void)
{
	uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
	TRx_WRITEREG(0x0000, u8Array);	
	TRx_READREG(0x1e04, u8Array);
    OR_U8_ARRAY(u8Array, 0x01, 0x40, 0x00, 0x01);
	return TRx_WRITEREG(0x1e04, u8Array);
}

/**
 * @brief  Disable TX single tone (continuous wave) mode.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SingleTone_Disable(void)
{
	uint8_t u8Array[4];
	uint16_t Cnt = 0;

	TRx_READREG(0x1e04, u8Array);
    AND_U8_ARRAY(u8Array, 0xFE, 0xBF, 0xFF, 0xFE);
	TRx_WRITEREG(0x1e04, u8Array);		
	do
	{
		TRx_READREG(0x1efc, u8Array);
		Cnt++;
	}while(!COMPARE_U8_ARRAY(u8Array, 0x00, 0x02, 0x00, 0x01) && Cnt<TimerOutCnt);
	if(Cnt>=TimerOutCnt)
		return TRx_STATUS_TO_FAIL;

    // check the value of 0x1988 is greater than 100000
    TRx_READREG(0x1988, u8Array);
    if ((u8Array[2] > 0x01) || 
        (u8Array[2] == 0x01 && (u8Array[1] > 0x86 || (u8Array[1] == 0x86 && u8Array[0] >= 0xA0))))
    {
        // Condition is established ==> set bit23 of 0x1E04
        TRx_READREG(0x1e04, u8Array);
        u8Array[2] |= 0x80; // 0x80 = (1 << 7),mapping to bit 23 of uint32_t
        TRx_WRITEREG(0x1e04, u8Array);
    }
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x07);
	return TRx_WRITEREG(0x0000, u8Array);
}
#endif

#ifdef EXTERN_RF_APIS_PN9
/* PN9 Application APIs */
/**
 * @brief  Enable PRBS9 pseudo-random bit sequence continuous transmission.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_PRBS9_Trigger(void)
{
    uint8_t u8Array[4];

    // 1. Reset 0x0000 to all zeros
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    TRx_WRITEREG(trx, 0x0000, u8Array);	

    // 2. Load PRBS9 configuration registers (0x1E40 - 0x1E4C)
    // 0x07a137bc -> {0xBC, 0x37, 0xA1, 0x07}
    SET_U8_ARRAY(u8Array, 0x07, 0xA1, 0x37, 0xBC);
    TRx_WRITEREG(trx, 0x1e40, u8Array);

    // 0xb710d793 -> {0x93, 0xD7, 0x10, 0xB7}
    SET_U8_ARRAY(u8Array, 0xB7, 0x10, 0xD7, 0x93);
    TRx_WRITEREG(trx, 0x1e44, u8Array);

    // 0x8aab5add -> {0xDD, 0x5A, 0xAB, 0x8A}
    SET_U8_ARRAY(u8Array, 0x8A, 0xAB, 0x5A, 0xDD);
    TRx_WRITEREG(trx, 0x1e48, u8Array);

    // 0x39b85666 -> {0x66, 0x56, 0xB8, 0x39}
    SET_U8_ARRAY(u8Array, 0x39, 0xB8, 0x56, 0x66);
    TRx_WRITEREG(trx, 0x1e4c, u8Array);

    // 3. Update 0x1E04 with mask 0x00407F01
    // Read current value first
    TRx_READREG(trx, 0x1e04, u8Array);
    // Apply Bitwise OR: u32Data |= 0x00407F01
    OR_U8_ARRAY(u8Array, 0x00, 0x40, 0x7F, 0x01);
    
    return TRx_WRITEREG(trx, 0x1e04, u8Array);
}
#endif

#ifdef EXTERN_RF_APIS_FORMAT
/**
 * @brief  Set RX sync word for frame start detection.
 * @param  SyncWord: 4-byte sync word array (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_SyncWord(uint8_t *SyncWord)
{
    /* 
     [SyncWord] expected format: {byte0, byte1, byte2, byte3} 
     Example: To observe "0x12345678" (binary: 00010010...1000) on-air 
     via signal analyzer (MSB first), the internal Syncword_u32 must be 
     bit-reversed to 0x1E6A2C48. This corresponds to 
     the byte array: {0x48, 0x2C, 0x6A, 0x1E} 
    */
	return TRx_WRITEREG(0xb100, SyncWord);
}
#endif

/**
 * @brief  Reset DUT's TX FIFO content.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_TX_FIFOReset(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0, 0, 0, 1);
    if (!(TRx_Write(COMM_RSTFIFO, u8Array, 4)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Reset DUT's RX FIFO content.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_FIFOReset(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0, 0, 0x01, 0);
    if (!(TRx_Write(COMM_RSTFIFO, u8Array, 4)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Set DUT's TX FIFO almost-empty and RX FIFO almost-full thresholds.
 * @param  TxThreshold:  TX FIFO almost-empty threshold in bytes (uint8_t, max 128)
 *         uRxThreshold: RX FIFO almost-full threshold in bytes (uint8_t, max 128)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_FIFOTHD(uint8_t TxThreshold, uint8_t uRxThreshold)
{
    uint8_t u8Array[4];

    // Boundary check for thresholds
    if(TxThreshold > 128 || uRxThreshold > 128)
    {
        return TRx_STATUS_FAIL;
    }

    // Mapping logic: ((128 - uRxThreshold) << 16) | TxThreshold 
    SET_U8_ARRAY(u8Array, 0x00, (uint8_t)(128 - uRxThreshold), 0x00, TxThreshold);

    return TRx_WRITEREG(0xa0a4, u8Array);
}

/**
 * @brief  Read up to READSIZE bytes from DUT's RX FIFO.
 * @param  READSIZE: maximum number of bytes to read (uint16_t)
 *         rxpbuff:  pointer to buffer to store received data (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_FIFOTHD(uint16_t READSIZE, uint8_t *rxpbuff)
{
	if(!(TRx_Read(COMM_RXFIFO, rxpbuff, READSIZE)))
		return TRx_STATUS_SUCCESS;
	else
		return TRx_STATUS_FAIL;
}

/**
 * @brief  Write data into DUT's TX FIFO.
 * @param  WRITESIZE: number of bytes to write (uint16_t, max 128)
 *         txpbuff:   pointer to TX data buffer (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_TX_FIFO(uint16_t WRITESIZE, uint8_t *txpbuff)
{
    if (WRITESIZE > 128)
        return TRx_STATUS_FAIL;

    if (!(TRx_Write(COMM_TXFIFO, txpbuff, WRITESIZE)))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Read all available data from DUT's RX FIFO (length read from register 0xA004).
 * @param  rxpbuff: pointer to buffer to store received data (uint8_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_FIFO(uint8_t *rxpbuff)
{
    uint8_t u8Array[4];

    TRx_READREG(0xa004, u8Array);
    if (!(TRx_Read(COMM_RXFIFO, rxpbuff, MAKE_U16(u8Array[3], u8Array[2]))))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Transmit data from DUT's TX FIFO.
 * @param  DataLen_byte: number of bytes to transmit (uint16_t, max 2047)
 *         CCAEn:        1 = enable CCA mode before TX, 0 = disable (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_TX_Trigger(uint16_t DataLen_byte, uint8_t CCAEn)
{
    uint8_t u8Array[4];
    // uint8_t *p = (uint8_t*)&u32Data;
    //  P[3] is u32Data[31:24]
    //  P[2] is u32Data[23:16]
    //  P[1] is u32Data[15:8]
    //  P[0] is u32Data[7:0]

    if (DataLen_byte > 2047)
        return TRx_STATUS_FAIL;

    TRx_READREG(0x10A8, u8Array);
    TRx_WRITEREG(0x1E1C, u8Array);

    /*(DataLen_byte << 16):
       [0] = flag
       [1] = 0x00
       [2] = len LSB
       [3] = len MSB
    */
    if (CCAEn)
    {
        SET_U8_ARRAY(u8Array, 0, 0x01, 0x5F, 0x03);
        TRx_WRITEREG(0xB19C, u8Array);

        /* TX Trigger ??? flag = 0x02 (enable CCA) */
        u8Array[0] = 0x02;
    }
    else
    {
        /* flag = 0x01 (disable CCA) */
        u8Array[0] = 0x01;
    }

    u8Array[1] = 0;
    u8Array[2] = (uint8_t)(DataLen_byte & 0xFF); // len LSB
    u8Array[3] = (uint8_t)(DataLen_byte >> 8);   // len MSB,?? 16-bit shift,??

    if (!TRx_Write(COMM_TXTrigger, u8Array, 4))
        return TRx_STATUS_SUCCESS;
    else
        return TRx_STATUS_FAIL;
}

/**
 * @brief  Start DUT's RX and wait until receiver is ready.
 * @param  DataLen_byte:   expected RX data length in bytes (uint16_t, max 2047)
 *         Mode802154En:   1 = 802.15.4 mode (validate protocol), 0 = transparent (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_Trigger(uint16_t DataLen_byte, uint8_t Mode802154En)
{
    uint8_t u8Array[4];
    xdata_u16 Cnt = 0;

    if (DataLen_byte > 2047)
        return TRx_STATUS_FAIL;

    u8Array[3] = (uint8_t)((DataLen_byte >> 7) & 0xFF); // bits 31..24
    u8Array[2] = (uint8_t)((DataLen_byte << 1) & 0xFF); // bits 23..16
    u8Array[1] = 0;                                     //(uint8_t)((DataLen_byte << 7) & 0x80);   // bit 15
    u8Array[0] = Mode802154En ? 0x11 : 0x21;            // enable or ignore checksum
    TRx_Write(COMM_RXTrigger, u8Array, 4);

    do
    {
        TRx_READREG(0x1394, u8Array);
        Cnt++;
    } while (!(u8Array[0] & 0x01) && Cnt < TimerOutCnt);

    if (Cnt >= TimerOutCnt)
        return TRx_STATUS_TO_FAIL;
    else
        return TRx_STATUS_SUCCESS;
}

/**
 * @brief  Turn off DUT's RX.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RxOff(void)
{
    uint8_t u8Array[4];
    xdata_u16 Cnt = 0;

    SET_U8_ARRAY(u8Array, 0, 0, 0, 0);
    TRx_Write(COMM_RXTrigger, u8Array, 4);
    do
    {
        TRx_READREG(0xa018, u8Array);
        Cnt++;
    } while ((u8Array[0] & 0x01) && Cnt < TimerOutCnt);

    if (Cnt >= TimerOutCnt)
        return TRx_STATUS_TO_FAIL;
    else
        return TRx_STATUS_SUCCESS;
}

/**
 * @brief  Get DUT's RSSI value of the last received packet.
 * @param  RSSI_dBm: pointer to store the raw RSSI result (uint16_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GetRSSI_Data(uint16_t *RSSI_dBm)
{
    uint8_t u8Array[4];

    if (!(TRx_READREG(0xb1fc, u8Array)))
    {
        *RSSI_dBm = MAKE_U16(u8Array[1], u8Array[0]);
        return TRx_STATUS_SUCCESS;
    }
    else
        return TRx_STATUS_FAIL;
}
#ifdef EXTERN_RF_APIS_CCA
/**
 * @brief  Trigger a CCA RSSI measurement and return the result.
 * @param  u16RSSI_dBm: pointer to store the 16-bit raw RSSI result (uint16_t*)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1 (timeout), TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_GetRSSI_CCA(uint16_t *u16RSSI_dBm)
{
    uint8_t u8Array[4];
    uint8_t Cnt = 0;

    /* Enable CCA RX mode: write 0x00015F03 to 0xB19C */
    SET_U8_ARRAY(u8Array, 0x00, 0x01, 0x5F, 0x03);
    TRx_WRITEREG(0xb19c, u8Array);

    /* Trigger RX: write 0x00000201 via COMM_RXTrigger opcode */
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x02, 0x01);
    TRx_Write(COMM_RXTrigger, u8Array, 4);

    /* Poll 0xB1FC until bit31 (byte[3] bit7) clears, or timeout */
    do
    {
        Delay_ms(1);
        TRx_READREG(0xb1fc, u8Array);
        Cnt++;
    } while ((u8Array[3] & 0x80) && Cnt < 100);

    /* Extract 16-bit RSSI result from bytes [1:0] */
    *u16RSSI_dBm = MAKE_U16(u8Array[1], u8Array[0]);

    /* Disable CCA RX mode: write 0x00015F00 to 0xB19C */
    SET_U8_ARRAY(u8Array, 0x00, 0x01, 0x5F, 0x00);
    TRx_WRITEREG(0xb19c, u8Array);

    TRx_RxOff();

    /* Poll 0xA018 until all bits clear */
    do
    {
        TRx_READREG(0xa018, u8Array);
    } while (IS_INT_ST_ANY(u8Array));

    /* Reset MAC: set then clear low nibble of reg 0x0010 */
    TRx_READREG(0x0010, u8Array);
    u8Array[0] |= 0x0F;
    TRx_WRITEREG(0x0010, u8Array);
    TRx_READREG(0x0010, u8Array);
    u8Array[0] &= 0xF0;
    TRx_WRITEREG(0x0010, u8Array);

    if (Cnt >= 100)
        return TRx_STATUS_TO_FAIL;
    else
        return TRx_STATUS_SUCCESS;
}
/**
 * @brief  Configure CCA (Clear Channel Assessment) RSSI threshold.
 * @param  u16CCA_RSSITHD: 12-bit RSSI threshold value (uint16_t)
 *           e.g. TX_CCA_RSSI_60dBmTHD (0xE1F) or TX_CCA_RSSI_80dBmTHD (0xD7F)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_CCA_Config(uint16_t u16CCA_RSSITHD)
{
    uint8_t u8Array[4];
    /* Register value: [byte3:byte2] = threshold, [byte1:byte0] = 0x0500 */
    SET_U8_ARRAY(u8Array, (uint8_t)(u16CCA_RSSITHD >> 8), (uint8_t)(u16CCA_RSSITHD & 0xFF), 0x05, 0x00);
    return TRx_WRITEREG(0xa040, u8Array);
}
#endif
#ifdef EXTERN_RF_APIS_WUT
/* WOR Application APIs */
/**
 * @brief  Enable RX wake-up timer (WOR mode); DUT enters power saving after timeout.
 * @param  TOTcnt:      timeout count multiplier (uint16_t, max 0x3F)
 *         TOTcnt_unit: timeout unit — 0:200us, 1:400us, 2:1ms, 3:20ms (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_WUTMR_Enable(uint16_t TOTcnt, uint8_t TOTcnt_unit)
{
	uint8_t u8Array[4];
	if(TOTcnt > 0x3f)
		return TRx_STATUS_FAIL;
	SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x01);
	TRx_WRITEREG(0x002c, u8Array);
    //regA014[6:5] is unit of RX duration,
    // 0: unit is 200us
    // 1: unit is 400us
    // 2: unit is 1ms    
    // 3: unit is 20ms  
	if(TOTcnt)
        SET_U8_ARRAY(u8Array, 0x00, (uint8_t)(TOTcnt - 1), 0x04, 0x11 | (TOTcnt_unit << 5));
    else
        SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x04, 0x71);

    return TRx_WRITEREG(0xA014, u8Array);
}

/**
 * @brief  Disable RX wake-up timer (WOR mode).
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_RX_WUTMR_Disable()
{
	uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    TRx_WRITEREG(0x002C, u8Array);
    return TRx_WRITEREG(0xA014, u8Array);
}

/**
 * @brief  Configure the Wake-up Timer (WUTMR) value and operation mode.
 * @param  u8TimerValArr: pre-calculated 4-byte timer value, Little-Endian (uint8_t*)
 *           Formula: RegisterValue = (Time_ms * 32768) / 1000
 *           Example 1 - 1000ms: (1000*32768)/1000 = 32768 = 0x00008000 → {0x00,0x80,0x00,0x00}
 *           Example 2 -  100ms: (100*32768)/1000  = 3277  = 0x00000CCD → {0xCD,0x0C,0x00,0x00}
 *         PeriodicEn: 1 = periodic mode, 0 = one-shot mode (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL if hardware polling times out
 */
STATUS_TRx TRx_SetWUTMR_Timer(uint8_t* u8TimerValArr, uint8_t PeriodicEn)
{
    /*
    u8TimerValArr Pre-calculated 4-byte timer value (Little-Endian array).
    Calculation Formula: 
    RegisterValue = (Time_ms * 32768) / 1000
    Example1: Setting 1000ms (1 second)
            1. Calculation: (1000 * 32768) / 1000 = 32768 = 0x00008000
            2. u8TimerValArr should be = {0x00, 0x80, 0x00, 0x00} // b0, b1, b2, b3

    Example2: Setting 100ms
            1. Calculation: (100 * 32768) / 1000 = 3276.8 -> Round to 3277 = 0x00000CCD
            2. u8TimerValArr should be =  {0xCD, 0x0C, 0x00, 0x00} // b0, b1, b2, b3
    */
	uint8_t u8Array[4];
    uint16_t Cnt = 0;
	
	// 1. initialize 0x8000
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    TRx_WRITEREG(0x8000, u8Array);
    // 2. wait for hardware Ready (0x8004 should be 0)
	do
	{
		TRx_READREG(0x8004, u8Array);
		Cnt++;
        Delay_ms(1);
	} while(!COMPARE_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00) && Cnt < TimerOutCnt);

	if(Cnt>=TimerOutCnt)
		return TRx_STATUS_TO_FAIL;
	else
	{
		// 3. write timer value calculated by user
        TRx_WRITEREG(0x8008, u8TimerValArr);
		
		// 4. set periodic flag in 0x8000
        if(PeriodicEn)
            SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x11); // Periodic Enable
        else
            SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x01); // One-shot Enable
            
        return TRx_WRITEREG(0x8000, u8Array);
	}
}
#endif

/**
 * @brief  Trigger DUT software reset.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SW_Reset(void)
{
    uint8_t u8Array[4];

    SET_U8_ARRAY(u8Array, 0xff, 0xff, 0xff, 0xff);
    TRx_WRITEREG(0x0010, u8Array);
    Delay_ms(10);
    SET_U8_ARRAY(u8Array, 0, 0, 0, 0);
    return TRx_WRITEREG(0x0010, u8Array);
}

/**
 * @brief  Configure 40 MHz crystal capacitor tuning.
 * @param  Cap_I: input capacitor value (uint8_t, max 127)
 *         Cap_O: output capacitor value (uint8_t, max 127)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_XtalCap40M_Config(uint8_t Cap_I, uint8_t Cap_O)
{
    uint8_t u8Array[4];

    if (Cap_I > 127 || Cap_O > 127)
        return TRx_STATUS_FAIL;
    TRx_READREG(0x1580, u8Array);

    u8Array[2] &= 0xc0;
    u8Array[1] = 0x00;

    // fill cap : ( Cap_I << 8 ) | ( Cap_O << 15 )
    u8Array[2] |= (uint8_t)((Cap_O >> 1) & 0x3F);
    u8Array[1] |= (uint8_t)(Cap_I & 0x7F);
    u8Array[1] |= (uint8_t)((Cap_O & 0x01) << 7);

    return TRx_WRITEREG(0x1580, u8Array);
}

/**
 * @brief  Select a pre-defined channel from the channel table by index.
 * @param  ch_idx: index of the target channel in the lookup table (uint8_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SelectChannel(uint8_t ch_idx)
{
    uint8_t u8Array[4];

    // gTRx_DataRate[3]  gTRx_DataRate[2]  gTRx_DataRate[1]  gTRx_DataRate[0]
    //  bit31..24         bit23..16         bit15..8          bit7..0
    //    xdata_u8 gTRx_DataRate[] = GLB_DATA_RATE;
    //    xdata_u8 gTRx_FreqDev[]  = GLB_DEVIATION;
    // Determine gTRx_DataRate >= 0x000EA600 (960000)
    if ((gTRx_DataRate[3] != 0x00) ||
        (gTRx_DataRate[3] == 0x00 && gTRx_DataRate[2] > 0x0E) ||
        (gTRx_DataRate[3] == 0x00 && gTRx_DataRate[2] == 0x0E && gTRx_DataRate[1] > 0xA6) ||
        (gTRx_DataRate[3] == 0x00 && gTRx_DataRate[2] == 0x0E && gTRx_DataRate[1] == 0xA6 && gTRx_DataRate[0] != 0x00))
    {
        gTRx_DataRate[3] |= 0x01; // set bit24
    }

    /* These three should be written back as they were originally */
    TRx_WRITEREG(0x1980, channel_pll_table[ch_idx].freq);
    TRx_WRITEREG(0x1984, gTRx_FreqDev);
    TRx_WRITEREG(0x1988, gTRx_DataRate);
    SET_U8_ARRAY(u8Array, 0, 0, 0, 1);
    TRx_WRITEREG(0x1990, u8Array);
#if Crystal_Hz == 38400000
    TRx_WRITEREG(0x1998, channel_pll_table[ch_idx].reg_1998);
    TRx_WRITEREG(0x199c, channel_pll_table[ch_idx].reg_199C);
    TRx_WRITEREG(0x19A0, channel_pll_table[ch_idx].reg_19A0);
    TRx_WRITEREG(0x1994, channel_pll_table[ch_idx].reg_1994);
    
    /* Write back MODEM_CLK_REG and RX_GAIN_FILTSET */
    {
        uint16_t i = 0;
        uint8_t fg_found = 0;
        while (!IS_TABLE_END(TRx_Config_Array[i])){
            
            if ((TRx_Config_Array[i].addr == 0x0004)|| 
                (TRx_Config_Array[i].addr == 0x1604))
            {
                fg_found++;
                TRx_WRITEREG(TRx_Config_Array[i].addr, TRx_Config_Array[i].reg_data);
            }
            
            if(fg_found >= 2) break;
            i++;
        }
    }
#endif
    return TRx_STATUS_SUCCESS;
}

#ifdef EXTERN_RF_APIS_CH
#define PLL_Q         18
#define PLL_ONE       (1UL << PLL_Q)
#define PLL_OFFSET    (32UL << PLL_Q)
/**
 * @brief Calculate PLL word and store into a 4-byte Little-Endian array
 * @param freq_hz Target frequency in Hz
 * @param mul     Frequency multiplier (e.g., 2, 4, 8, 16, 32)
 * @param u8Array Pointer to the destination uint8_t[4] array
 */
static void calc_pll_word(uint32_t freq_hz, uint8_t mul, uint8_t *u8Array)
{
    float xdata num = (float)freq_hz * mul * PLL_ONE;   // Q18 calculation
    uint32_t xdata xq  = num / (2UL * Crystal_Hz);      // still Q18
    uint32_t xdata pll = xq - PLL_OFFSET;                // (X - 32) in Q18
        
    // Decompose uint32_t result into Little-Endian array
    u8Array[0] = (uint8_t)(pll & 0xFF);         // Byte 0 (LSB)
    u8Array[1] = (uint8_t)((pll >> 8) & 0xFF);  // Byte 1
    u8Array[2] = (uint8_t)((pll >> 16) & 0xFF); // Byte 2
    u8Array[3] = (uint8_t)((pll >> 24) & 0xFF); // Byte 3 (MSB)
}

/**
 * @brief  Set DUT's working channel by frequency.
 * @param  u32Frequency_Hz: target frequency in Hz (uint32_t)
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_SetChannel( uint32_t u32Frequency_Hz )
{
    uint8_t u8Array[4];
    uint8_t mul = 0;
        
    if(gTRx_DataRate[3] > 0 || 
           (gTRx_DataRate[3] == 0 && gTRx_DataRate[2] > 0x0E) || 
           (gTRx_DataRate[3] == 0 && gTRx_DataRate[2] == 0x0E && gTRx_DataRate[1] >= 0xA6)) 
    {
        // excute u32gTRx_DataRate |= (1UL << 24);
        gTRx_DataRate[3] |= 0x01; 
    }
    /* These three should be written back as they were originally */
    u8Array[0] = (u32Frequency_Hz >>0)&0xFF;
    u8Array[1] = (u32Frequency_Hz >>8)&0xFF;
    u8Array[2] = (u32Frequency_Hz >>16)&0xFF;
    u8Array[3] = (u32Frequency_Hz >>24)&0xFF;
    TRx_WRITEREG(0x1980, u8Array);
    TRx_WRITEREG(0x1984, gTRx_FreqDev);
    TRx_WRITEREG(0x1988, gTRx_DataRate);
    u8Array[0] = 1;
    TRx_WRITEREG(0x1990, u8Array); //SYNTH_CAL_TRIGGER
#if (Crystal_Hz == 38400000)
        uint8_t i;   
        xdata_u32 hf_Hz, lf_Hz, RXm_Hz, TXm_Hz;
        //xdata_u32 u32Frequency_Hz = MAKE_U32(Frequency_Hz[3], Frequency_Hz[2], Frequency_Hz[1], Frequency_Hz[0]);
        xdata_u32 u32DataRate = MAKE_U32(gTRx_DataRate[3], gTRx_DataRate[2], gTRx_DataRate[1], gTRx_DataRate[0]);
        xdata_u32 u32FreqDev = MAKE_U32(gTRx_FreqDev[3], gTRx_FreqDev[2], gTRx_FreqDev[1], gTRx_FreqDev[0]);
            
        u32DataRate &= ~(1UL << 24);
        /* All calculations are in Hz, not MHz / float */
        hf_Hz = u32Frequency_Hz + u32FreqDev;   // high freq edge
        lf_Hz = u32Frequency_Hz - u32FreqDev;   // low  freq edge
        TXm_Hz = u32Frequency_Hz;
        if (u32DataRate >= 1000000UL)
            RXm_Hz = u32Frequency_Hz - (uint32_t)((Crystal_Hz / HW_Clk_MHz) * 2UL);
        else
            RXm_Hz = u32Frequency_Hz - (uint32_t)(Crystal_Hz / HW_Clk_MHz);
        /* Select MUL for expected band */
        if (u32Frequency_Hz >= 1556000000 && u32Frequency_Hz <= 2040000000) {
            // 0x0018a785
            u8Array[0] = 0x85; u8Array[1] = 0xA7; u8Array[2] = 0x18; u8Array[3] = 0x00;
            mul = 2;
        } 
        else if (u32Frequency_Hz >= 778000000 && u32Frequency_Hz <= 1020000000) {
            // 0x0016a785
            u8Array[0] = 0x85; u8Array[1] = 0xA7; u8Array[2] = 0x16; u8Array[3] = 0x00;
            mul = 4;
        } 
        else if (u32Frequency_Hz >= 389000000 && u32Frequency_Hz <= 510000000) {
            // 0x0014a785
            u8Array[0] = 0x85; u8Array[1] = 0xA7; u8Array[2] = 0x14; u8Array[3] = 0x00;
            mul = 8;
        } 
        else if (u32Frequency_Hz >= 194000000 && u32Frequency_Hz <= 255000000) {
            // 0x0012a785
            u8Array[0] = 0x85; u8Array[1] = 0xA7; u8Array[2] = 0x12; u8Array[3] = 0x00;
            mul = 16;
        } 
        else if (u32Frequency_Hz >= 97000000 && u32Frequency_Hz <= 127000000) {
            // 0x0010a785
            u8Array[0] = 0x85; u8Array[1] = 0xA7; u8Array[2] = 0x10; u8Array[3] = 0x00;
            mul = 32;
        }
        /* Calculate pll word and apply for expected frequency*/
        if (mul != 0) {
            TRx_WRITEREG(0x1380, u8Array);
                
            calc_pll_word(hf_Hz, mul, u8Array);
            TRx_WRITEREG(0x1998, u8Array);
                
            calc_pll_word(lf_Hz, mul, u8Array);
            TRx_WRITEREG(0x199C, u8Array);
                
            calc_pll_word(RXm_Hz, mul, u8Array);
            TRx_WRITEREG(0x19A0, u8Array);
                
            calc_pll_word(TXm_Hz, mul, u8Array);
            TRx_WRITEREG(0x1994, u8Array);
        }
            
        /* Write back MODEM_CLK_REG and RX_GAIN_FILTSET */
        {
            uint16_t i = 0;
            uint8_t fg_found = 0;
            while (!IS_TABLE_END(TRx_Config_Array[i])){
                if (TRx_Config_Array[i].addr == 0x0004 || TRx_Config_Array[i].addr == 0x1604)
                {
                    fg_found++;
                    TRx_WRITEREG(Load_Tool_CSR_array[i].addr, TRx_Config_Array[i].reg_data);
                }
                 if(fg_found >= 2) break;
                i++;
            }
        }
#endif  // (CRYSTAL_HZ == 38400000)

    return TRx_STATUS_SUCCESS;
}
#endif

/**
 * @brief  Write a null-terminated register array to DUT.
 * @param  arr: pointer to t_reg_array, terminated by IS_TABLE_END sentinel
 * @retval None
 */
void TRx_Write_Array(const t_reg_array *arr)
{
    uint16_t i = 0;

    while (!IS_TABLE_END(arr[i]))
    {
        TRx_WRITEREG(arr[i].addr, arr[i].reg_data);
        i++;

        if (i > 128)
            break; // too many array element
    }
}

/**
 * @brief  Initialize the DUT according to the Init_array.
 * @param  None
 * @retval TRx_STATUS_SUCCESS = 0, TRx_STATUS_TO_FAIL = 1, TRx_STATUS_FAIL = 2
 */
STATUS_TRx TRx_Init(void)
{
    uint16_t i;
    uint8_t u8Array[4];
    TRx_SW_Reset();
    //-----------------------------------
    // Default initialization
    TRx_Write_Array(Initial_Reg_Array);

    //-----------------------------------
    /*  Setup [RF Param+eter]
            TX/RX Propeties setting
            RF channel
            Data rate and modulation
    */
    TRx_Write_Array(TRx_Config_Array);
    Delay_ms(200);
    i = 0;
    do {
        TRx_READREG(0x1e34, u8Array);
        i++;
    } while ((u8Array[0] & 0x01) && i < TimerOutCnt);
    
    if(i == TimerOutCnt)
        return TRx_STATUS_TO_FAIL;
    //-----------------------------------
    /* Setup [Format]
            TRx_TX_PacketOffLoad();  //Tx packet: preamble_content, preamble_len, syncword, fifo_mode
            TRx_PAH433MHz20dBm_FSK( RF_TXPLV); //tx power level
            TRx_RX_SyncWord(); //Rx packet syncword
            TRx_FIFOMSB_Config( 1);
    */
    TRx_Write_Array(TRx_ConfigEx_Array);

    return TRx_STATUS_SUCCESS;
}

