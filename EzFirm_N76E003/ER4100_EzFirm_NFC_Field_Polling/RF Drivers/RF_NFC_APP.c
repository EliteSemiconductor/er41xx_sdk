/******************************************************************************/
/*
 * @file     RF_NFC_APP.c
 * @version  V1.0.0
 * @brief    RF application layer for ER4100 transceiver
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/
/*
 * NFC Block 255 (addr 0x93FC) — DPE Control & Status Register
 * Shadowed as the most-significant byte of Block 255 in NFC memory.
 *
 * Byte layout (little-endian, byte[3] = MSB of 32-bit register):
 *   byte[3] = DPE Control  (MCU writes via APB / SPI)
 *   byte[2] = DPE Status   (Host/reader writes via RFID interface)
 *   byte[1] = data (user)
 *   byte[0] = data (user)
 *
 * DPE Control Register (byte[3] of addr 0x93FC):
 *   bit[7:5]  -             reserved / free for AFE config
 *   bit[4]    DPE_rst       1 = hold DPE in reset state
 *   bit[3]    clear/mask    1 = clear active IRQ and keep masked
 *                               until this bit is set back to 0
 *   bit[2:0]  irq_src[2:0]  Interrupt source selection (index 0~7):
 *                               0 = IGNORED    (no interrupt)
 *                               1 = PWRGOOD    (The presence or absence of NFC field)
 *                               2 = RX_CMD     (reader sent a command)
 *                               3 = TX_REPLY   (reply transmitted)
 *                               4 = USER_CFG4
 *                               5 = USER_CFG5
 *                               6 = USER_CFG6
 *                               7 = USER_CFG7
 *
 * DPE Status Register (byte[2] of addr 0x93FC):
 *   bit[7]    USER_CFG7
 *   bit[6]    USER_CFG6
 *   bit[5]    USER_CFG5
 *   bit[4]    USER_CFG4
 *   bit[3]    TX_REPLY
 *   bit[2]    RX_CMD
 *   bit[1]    PWRGOOD
 *   bit[0]    IGNORED
 *   Each bit is set by DPE when the corresponding event occurs.
 *   Written back to 0 by host/reader to acknowledge.
 */

/* Includes -----------------------------------------------------------------*/
#include "Common.h"
/* Definition & Macro -------------------------------------------------------*/
#define ATTACHED_CNT_THR            50
#define DETACHED_CNT_THR            50

#define BUFFER_SIZE                 64
/* Typedef ----------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
xdata_u8           rf_err_mode;
xdata_u8           gUartCmd;        /* Filled by UART RX ISR; consumed by XTAPP_NFC_Process */
xdata_u8           gXtBuffer[BUFFER_SIZE];
static uint8_t     u8NfcPwrGood = 0;
static uint8_t     u8NfcIrqStatus = 0; /* Latched DPE status from last NFC IRQ */

static uint8_t     u8AttachedCnt = 0;
static uint8_t     u8DettachedCnt = 0;
static const uint8_t code NFC_HeadBlock[4][4] = {
    {0x04, 0x34, 0x74, 0xCC},  //Block 0
    {0xE1, 0xE3, 0x1C, 0x80},  //Block 1
    {0x9E, 0x00, 0x00, 0x00},  //Block 2
    {0xE1, 0x10, 0x12, 0x00},  //Block 3
};

/* Function prototypes ------------------------------------------------------*/
static void XTAPP_NFC_WriteDataBlock(void);
static void XTAPP_NFC_ReadDataBlock(void);
static void XTAPP_NFC_DumpAllBlocks(void);
static void XTAPP_NFC_PrintMenu(void);

/* Function -----------------------------------------------------------------*/
/**
 * @brief  Print register value to UART in hex format
 * @param  addr: 16-bit register address
 * @retval None
 */
void print_addr_data(uint16_t addr, uint8_t* u8arr)
{
    UART0_SendStr("0x");
    UART0_SendHex8(addr >> 8);
    UART0_SendHex8(addr & 0xFF);
    UART0_SendStr(" : 0x");
    UART0_SendHex8(u8arr[3]);
    UART0_SendHex8(u8arr[2]);
    UART0_SendHex8(u8arr[1]);
    UART0_SendHex8(u8arr[0]);
    UART0_SendStr("\r\n");
}

/**
 * @brief  Read one register and print its address and value to UART
 * @param  addr: 16-bit register address
 * @retval None
 */
void dump_reg(uint16_t addr)
{
    uint8_t  u8Array[4];
    TRx_READREG(addr, u8Array);
    print_addr_data(addr, u8Array);
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
            break;
        else
        {
            try_cnt--;
            Delay_ms(10);
            if(!try_cnt)
                return 1;   // fail
        }
        TRx_READREG(0xF100, u8Array);
    } while(1);
    return 0;   // pass
}
/**
 * @brief  Enable NFC mode
 * @param  None
 * @retval None
 */
void NFC_Enable(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    TRx_WRITEREG(0x0030, u8Array);
}

/**
 * @brief  Disable NFC mode
 * @param  None
 * @retval None
 */
void NFC_Disable(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x01);
    TRx_WRITEREG(0x0030, u8Array);
}
 
/**
 * @brief  Set the IRQ source for NFC interrupt (reg 0x93FC bits[26:24]).
 * @note   u8IrqSrcVal is a direct index (0~7), NOT a bitmask.
 *         Only the selected source can trigger INT_N to MCU.
 *         Corresponds to NFC_IGNORED~NFC_USER_CFG7 defines (use the index, not the bitmask):
 *           0 = NFC_IGNORED   1 = NFC_PWRGOOD   2 = NFC_RX_CMD   3 = NFC_TX_REPLY
 *           4 = NFC_USER_CFG4 5 = NFC_USER_CFG5 6 = NFC_USER_CFG6 7 = NFC_USER_CFG7
 * @param  u8IrqSrcVal: IRQ source index, range 0~7
 * @retval 0: pass, 1: fail
 */
uint8_t NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)
{
    uint8_t u8Array[4];

    if (u8IrqSrcVal > 7)
        return 1;

    TRx_READREG(0x93FC, u8Array);
    u8Array[3] = (u8Array[3] & 0xF8) | (u8IrqSrcVal & 0x07); /* irq_src[2:0] = index 0~7 */
    TRx_WRITEREG(0x93FC, u8Array);
    return 0;
}

/**
 * @brief  Get the currently selected NFC IRQ source
 * @param  None
 * @retval irq_src[2:0] index (0~7) from DPE Control byte (0x93FC Byte3)
 */
uint8_t NFC_Get_IRQ_Src_Select(void)
{
    uint8_t u8Array[4]; 
    TRx_READREG(0x93FC, u8Array);
    return u8Array[3] & 0x07; //irq_src[2:0] = index 0~7
}

/**
 * @brief  Clear the active NFC interrupt event
 * @note   Sequence: clear DPE Status byte → set clear/mask bit → wait 1ms → clear clear/mask bit
 *         The 1ms delay ensures DPE latches the clear before the mask is released.
 * @param  None
 * @retval None
 */
void NFC_Clear_NFC_Event(void)
{
    uint8_t u8Array[4];
    TRx_READREG(0x93FC, u8Array);
    u8Array[2] = 0x00;  /* Clear status byte */  
    TRx_WRITEREG(0x93FC, u8Array); 
    
    u8Array[3] |= NFC_CLEAR_INT_MASK;  /* Set bit27:  trigger occupy interrupt clear */
    TRx_WRITEREG(0x93FC, u8Array);
    
    Delay_ms(1);
    
    u8Array[3] &= ~NFC_CLEAR_INT_MASK;  /* Clear bit27 - Interrupt of Control register */
    TRx_WRITEREG(0x93FC, u8Array); 
}
/**
 *  @brief   Set lock bits (Static lock) to blocks between block0 and block15
 *  @param   value        : Bit value to set, 1 = lock (set bit), 0 = unlock (clear bit)
 *  @param   start_block  : The starting block to set, range 0~15
 *  @param   end_block    : The ending block to set (inclusive), range 0~15
 *  @retval  result       : 0 = fail, 1 = success
 *
 *  Note:
 *      - Each block is 4 bytes.
 *      - Lock bits are stored in block2 (address 0x9008):
 *          - Lock0 (bit16~23) controls block0~7
 *          - Lock1 (bit24~31) controls block8~15
 */
uint8_t NFC_SetLockBlock_0_15(uint8_t value, uint16_t start_block, uint8_t end_block)
{
    int i;
    uint8_t u8Array[4];
    uint16_t lock_mask = 0;

    if (start_block > 15 || end_block > 15 || end_block < start_block)
    {
        UART0_SendStr("!Invalid block range!\r\n");
        return 0;   // fail
    }

    // create bit mask
    for (i = start_block; i <= end_block; i++)
        lock_mask |= (1 << i);

    // read block 2 (lock bytes are at byte[2]=Lock0, byte[3]=Lock1)
    TRx_READREG(0x9008, u8Array);

    if (value)
    {
        u8Array[2] |=  (uint8_t)(lock_mask & 0x00FF);         // Lock0: block0~7
        u8Array[3] |=  (uint8_t)((lock_mask & 0xFF00) >> 8);  // Lock1: block8~15
    }
    else
    {
        u8Array[2] &= ~(uint8_t)(lock_mask & 0x00FF);
        u8Array[3] &= ~(uint8_t)((lock_mask & 0xFF00) >> 8);
    }

    //UART0_SendStr("Updated Lock0 = %02X, Lock1 = %02X\r\n", u8Array[2], u8Array[3]);

    TRx_WRITEREG(0x9008, u8Array);
    return 1; // success
}

/**
 *  @brief   Set lock bits (dynamic lock) to blocks between block16 and block247
 *  @param   value        : Bit value to set, 1 = lock (set bit), 0 = unlock (clear bit)
 *  @param   start_block  : The starting block to set, range 16~247
 *  @param   end_block    : The ending block to set (inclusive), range 16~247
 *  @retval  result       : 0 = fail, 1 = success
 *
 *  Note:
 *      - Each block is 4 bytes.
 *      - Lock bits are stored in block248~255 (address 0x93E0~0x93FC):
 *          - bit0~31 of 0x93E0 controls block16~47
 *          - bit0~7 of 0x93FC controls block240~247
 *          - bit8~31 of 0x93FC are not lock bits, but is DPE Register
 */
uint8_t NFC_SetLockBlock_16_247(uint8_t value, uint16_t start_block, uint8_t end_block)
{
    uint16_t i;
    uint8_t u8Array[4];
    uint16_t addr;
    uint8_t bit_in_byte;
    uint8_t byte_index;

    if (start_block < 16 || start_block > 247 || end_block > 247 || end_block < start_block)
    {
        UART0_SendStr("!Invalid block range!\r\n");
        return 0; // fail
    }

    for (i = start_block; i <= end_block; i++)
    {
        uint16_t bit_index   = i - 16;               // 0 ~ 231
        uint8_t  word_offset = bit_index / 32;        // 0 ~ 7
        uint8_t  bit_in_word = bit_index % 32;

        addr = 0x93E0 + (word_offset * 4);

        // Protect 0x93FC - bit8~31(DPE), only allow modification of bit0~7
        if (addr == 0x93FC && bit_in_word > 7)
            continue;

        byte_index  = bit_in_word / 8;   // which byte in u8Array (0~3)
        bit_in_byte = bit_in_word % 8;   // which bit within that byte

        TRx_READREG(addr, u8Array);

        if (value)
            u8Array[byte_index] |=  (1 << bit_in_byte);
        else
            u8Array[byte_index] &= ~(1 << bit_in_byte);

        TRx_WRITEREG(addr, u8Array);
    }
    return 1; // success
}

/**
 * @brief  Initialize NFC tag: write header blocks, clear lock bits, enable NFC
 * @note   IRQ source is set to NFC_IGNORED — INT_N pin is not used in polling mode.
 *         Field detection is done by directly reading the PWRGOOD bit via SPI.
 */
void XTAPP_NFC_Config(void)
{
    uint8_t i;
    uint8_t u8Array[4];
    /* Reset NFC IP */
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    u8Array[NFC_DPE_CTRL_BYTE_IDX] |= NFC_DPE_RST_MASK;
    Delay_ms(10);
    u8Array[NFC_DPE_CTRL_BYTE_IDX] &= ~NFC_DPE_RST_MASK;
    /* Enable NFC */
    NFC_Disable();
    Delay_ms(1);
    NFC_Enable();
    /* Clear all lock bits — allow reader/phone to access block 0~247 */
    NFC_SetLockBlock_0_15(0, 0, 15);
    NFC_SetLockBlock_16_247(0, 16, 247);
    /* Set DPE control reg, make IRQ to Nothing */
    NFC_Select_IRQ_Src(NFC_IGNORED);
    /* Write default header blocks (Block 0~3) */
    for (i = 0; i < 4; i++)
        TRx_WRITEREG(0x9000 + (i << 2), (uint8_t *)NFC_HeadBlock[i]);
}

/**
 * @brief  Initialize RF transceiver and NFC
 * @param  None
 * @retval 0: pass, 1: fail
 */
uint8_t XTAPP_Init(void)
{
    rf_err_mode = 1;

    TRx_IoConfig();

    if (XTAPP_Strobe() == 0)    // Check RF chip present
    {
        TRx_SW_Reset();

        if (TRx_Init() == TRx_STATUS_SUCCESS)
        {
            TRx_WRITEREG(0xF004, (uint8_t *)"\x00\x00\x00\x00"); // Disable NFC interrupt MASK forcely
            XTAPP_NFC_Config();
            rf_err_mode = 0;
        }
    }

    return rf_err_mode;
}

/**
 * @brief  Read DPE Status register and return the PWRGOOD bit
 * @param  None
 * @retval Non-zero if NFC field is present, 0 if field is absent
 */
uint8_t XTAPP_Get_NFC_Statu_Reg(void)
{
    uint8_t u8Array[4];
    uint8_t status_bit_mask;
    /* Poll PWRGOOD bit — if clear, RF field has been removed */
    TRx_READREG(NFC_DPE_BLOCK_ADDRESS, u8Array);
    status_bit_mask = u8Array[NFC_DPE_STATUS_BYTE_IDX]; // Control byte indicates which events are enabled for interrupt
    return status_bit_mask & NFC_PWRGOOD_MASK;
}

/**
 * @brief  Detect NFC field attach/detach by polling the PWRGOOD bit via SPI
 * @note   Call this function repeatedly from the main loop. No interrupt is used.
 *         Attach: transitions u8NfcPwrGood 0→1 on first PWRGOOD high.
 *         Detach: requires DETACHED_CNT_THR (50) consecutive PWRGOOD-low readings
 *                 before transitioning u8NfcPwrGood 1→0, providing debounce.
 * @param  None
 * @retval None
 */
void XTAPP_NFC_Scan(void)
{
    if (rf_err_mode == 1)
        return;

    /* Polling NFC field detection */
    if (!u8NfcPwrGood && 
        (XTAPP_Get_NFC_Statu_Reg() & NFC_PWRGOOD_MASK))
    { /* Field Attached */  
        u8NfcPwrGood = 1;
        Delay_ms(10);
        UART0_SendStr("\r\n+--- NFC Attached ---+\r\n");
    }
    else if(u8NfcPwrGood )
    {
        if((XTAPP_Get_NFC_Statu_Reg() & NFC_PWRGOOD_MASK) == 0)
        {   /* Field detached */                
            u8DettachedCnt++;
            Delay_ms(10);
            if(u8DettachedCnt >= DETACHED_CNT_THR)
            {
                u8NfcPwrGood = 0;
                UART0_SendStr("\r\n+--- NFC Detached ---+\r\n");
                //return;
            }
        }
        else
        {
            u8DettachedCnt = 0;
        }
    }
    
}