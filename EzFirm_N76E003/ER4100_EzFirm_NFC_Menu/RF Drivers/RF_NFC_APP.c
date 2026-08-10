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
//-------------------------------------------------------------------------------
// NFC Function                                                                 
//-------------------------------------------------------------------------------
/**
 * @brief  Enable NFC mode (write 0x00000000 to reg 0x0030)
 */
void NFC_Enable(void)
{
    uint8_t u8Array[4];
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x00, 0x00);
    TRx_WRITEREG(0x0030, u8Array);
}

/**
 * @brief  Disable NFC mode (write 0x00000001 to reg 0x0030)
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
 * @brief  Read current IRQ source selection from DPE Control reg (0x93FC bits[2:0])
 * @retval irq_src index 0~7
 */
uint8_t NFC_Get_IRQ_Src_Select(void)
{
    uint8_t u8Array[4];
    TRx_READREG(0x93FC, u8Array);
    return u8Array[3] & 0x07; //irq_src[2:0] = index 0~7
}

/**
 * @brief  Clear active NFC interrupt via DPE Control reg bit3 (clear/mask IRQ)
 * @note   Sets bit3 to 1 (clears + masks interrupt), waits 1 ms, then clears bit3
 *         to re-enable interrupts. Delay is required for DPE to latch the clear.
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
 * @brief  Read DPE Status byte (0x93FC Byte2) — event flags set by NFC host/reader
 * @retval DPE Status byte (bit mask of NFC_*_MASK defines)
 */
uint8_t NFC_Get_Status(void)
{
    return 0;
}
/**
 * @brief  Set NFC antenna impedance at reg 0x1700[bits 12:10].
 * @param  u8IMP_0_7: impedance value (0~7)
 * @retval 0: pass, 1: fail
 */
uint8_t NFC_Set_Impedance(uint8_t u8IMP_0_7)
{
    uint8_t u8Array[4];
    if (u8IMP_0_7 > 7)
        return 1;
    TRx_READREG(0x1700, u8Array);
    u8Array[1] = (u8Array[1] & 0xE3) | ((u8IMP_0_7 & 0x07) << 2); /* bits[4:2] = impedance */
    TRx_WRITEREG(0x1700, u8Array);
    return 0;
}

/**
 * @brief  Read NFC antenna impedance from reg 0x1700[bits 12:10].
 * @retval impedance value 0~7 on success, 1 on read failure
 */
uint8_t NFC_Get_Impedance(void)
{
    uint8_t u8Array[4];
    if (TRx_READREG(0x1700, u8Array))
        return 1;
    return (u8Array[1] >> 2) & 0x07; // byte[1] bits[4:2] = bits[12:10]
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
 * @brief  Set or clear dynamic lock bits for Block 16~247
 * @note   Lock bits are stored in regs 0x93E0~0x93FC (each 32-bit word covers 32 blocks).
 *         Blocks 240~247 use bit0~7 of 0x93FC Byte1; DPE registers (Byte2/3) are protected.
 * @param  value       : 1 = lock, 0 = unlock
 * @param  start_block : first block to set, range 16~247
 * @param  end_block   : last block to set (inclusive), range 16~247, must be >= start_block
 * @retval 1 = success, 0 = invalid range
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
//-------------------------------------------------------------------------------
// Task Flow                                                                 
//-------------------------------------------------------------------------------
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
 * @brief  Write test pattern (0x01~0x40) to NFC data blocks (Block 4~19)
 */
static void XTAPP_NFC_WriteDataBlock(void)
{
    uint8_t i;
    for (i = 0; i < BUFFER_SIZE; i++)
        gXtBuffer[i] = i + 1;
    for (i = 0; i < BUFFER_SIZE; i++)
        TRx_WRITEREG(NFC_DATA_BLOCK_START_ADDRESS + (i << 2), &gXtBuffer[i * 4]);
    
    dump_buffer(gXtBuffer, BUFFER_SIZE, 0, 0);
}

/**
 * @brief  Read NFC data blocks (Block 4~19) into gXtBuffer and print
 */
static void XTAPP_NFC_ReadDataBlock(void)
{
    uint8_t i;
    for (i = 0; i < BUFFER_SIZE; i++)
        TRx_READREG(NFC_DATA_BLOCK_START_ADDRESS + (i << 2), &gXtBuffer[i * 4]);

    dump_buffer(gXtBuffer, BUFFER_SIZE, 1, 0);
}
/**
 * @brief  Read all NFC blocks (Block 0~255) and print in hex
 */
static void XTAPP_NFC_DumpAllBlocks(void)
{
    //uint8_t u8Array[4];
    uint16_t i;
    for (i = 0; i < 256; i++)
    {
        //TRx_READREG(NFC_BLOCK0_ADDRESS + (i << 2), u8Array);
        dump_reg(NFC_BLOCK0_ADDRESS + (i << 2));
    }
}
/**
 * @brief  Print NFC UART command menu
 */
static void XTAPP_NFC_PrintMenu(void)
{
    UART0_SendStr("+----------------------------------------------------+\r\n");
    UART0_SendStr("+ NFC Attached                                       +\r\n");
    UART0_SendStr("+ ER41XX NFC Test — Select task by UART input:       +\r\n");
    UART0_SendStr("+ '1': MCU Write 64-Byte to data block (Bk4~Bk19)    +\r\n");
    UART0_SendStr("+ '2': MCU Read  64-Byte from data block (Bk4~Bk19)  +\r\n");
    UART0_SendStr("+ '3': Read DPE register                             +\r\n");
    UART0_SendStr("+ '4': Set irq_src = status4, wait reader trigger    +\r\n");
    UART0_SendStr("+ '5': Set irq_src = status5, wait reader trigger    +\r\n");
    UART0_SendStr("+ '6': Set irq_src = status6, wait reader trigger    +\r\n");
    UART0_SendStr("+ '7': Set irq_src = status7, wait reader trigger    +\r\n");
    UART0_SendStr("+ 'a': Read all blocks (Bk0~Bk255)                   +\r\n");
    UART0_SendStr("+----------------------------------------------------+\r\n");
}
/**
 * @brief  Initialize NFC tag: write header blocks, clear lock bits, enable NFC
 */
void XTAPP_NFC_Config(void)
{
    uint8_t u8Array[4];
    uint8_t i;
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
    /* Set DPE control reg, make IRQ to wait for NFC field */
    NFC_Select_IRQ_Src(NFC_PWRGOOD);
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
            XTAPP_NFC_Config();
            rf_err_mode = 0;
        }
    }

    return rf_err_mode;
}

/**
 * @brief  Process NFC DPE event (called from XTAPP_IrqHdlr on NFC IRQ)
 *         Handles field attach/detach and status4~7 triggers.
 */
uint8_t XTAPP_NFC_IRQ_Parser(void)
{
    uint8_t u8Array[4];
    uint8_t selected_irq_val, status_bit_mask;
    /* Read DPE block to get current control and status */
    TRx_READREG(NFC_DPE_BLOCK_ADDRESS, u8Array);
    selected_irq_val = u8Array[NFC_DPE_CTRL_BYTE_IDX] & 0x07; // Control byte indicates which events are enabled for interrupt
    status_bit_mask = u8Array[NFC_DPE_STATUS_BYTE_IDX];
    /* check if there is a valid event */
    if (selected_irq_val)
    {
        switch (selected_irq_val)
        {
            case NFC_PWRGOOD:
                /* Field attached */
                u8NfcPwrGood = 1;
                NFC_Select_IRQ_Src(NFC_IGNORED); /* Switch IRQ src to IGNORED — prevents continuous interrupts while RF field remains present */
                XTAPP_NFC_PrintMenu();
                break;

            case NFC_USER_CFG4:
            case NFC_USER_CFG5:
            case NFC_USER_CFG6:
            case NFC_USER_CFG7:
                /* Write DPE Status from reader/phone */
                UART0_SendStr("DPE_Status: ");
                UART0_SendHex8(status_bit_mask);
                UART0_SendStr("\r\n");
                NFC_Select_IRQ_Src(NFC_IGNORED); /* Back to default after trigger */
                break;

            default:
                break;
        }
    }
    /* Clear current event (interrupt flag) */
    NFC_Clear_NFC_Event();
    return selected_irq_val;
}

/**
 * @brief  Process UART commands while NFC field is attached (non-blocking)
 *         Call from XTAPP_Scan. gUartCmd must be filled by UART RX ISR.
 */
void XTAPP_NFC_Task(void)
{
    uint8_t u8Array[4];
    uint8_t c;
    /* Wait for command from UART */
    if(!RI)
        return;
    /* Process the command */
    c = SBUF;
    RI = 0;
    Send_Data_To_UART0(c);
    UART0_SendStr("\r\n");
    switch (c)
    {
        case '1':
            XTAPP_NFC_WriteDataBlock();
            break;

        case '2':
            XTAPP_NFC_ReadDataBlock();
            break;

        case '3':
            TRx_READREG(NFC_DPE_BLOCK_ADDRESS, u8Array);
            UART0_SendStr("DPE_Ctrl: ");
            UART0_SendHex8(u8Array[NFC_DPE_CTRL_BYTE_IDX]);
            UART0_SendStr("\r\n");
            UART0_SendStr("DPE_Status: ");
            UART0_SendHex8(u8Array[NFC_DPE_STATUS_BYTE_IDX]);
            UART0_SendStr("\r\n");
            break;
        case '4':
            NFC_Select_IRQ_Src(NFC_USER_CFG4);
            UART0_SendStr("irq_src = status4. Reader trigger: WRITE A2 FF 00 00 10 04\r\n");
            break;
        case '5':
            NFC_Select_IRQ_Src(NFC_USER_CFG5);
            UART0_SendStr("irq_src = status5. Reader trigger: WRITE A2 FF 00 00 20 05\r\n");
            break;
        case '6':
            NFC_Select_IRQ_Src(NFC_USER_CFG6);
            UART0_SendStr("irq_src = status6. Reader trigger: WRITE A2 FF 00 00 40 06\r\n");
            break;
        case '7':
            NFC_Select_IRQ_Src(NFC_USER_CFG7);
            UART0_SendStr("irq_src = status7. Reader trigger: WRITE A2 FF 00 00 80 07\r\n");
            break;
        case 'a':
            XTAPP_NFC_DumpAllBlocks();
            break;
        default:
            UART0_SendStr("Input error\r\n");
            break;
    }
    
    if((c >= '4') && (c <= '7'))
    {
        uint8_t irq_event = 0;
        UART0_SendStr("Waiting for status# trigger by reader...\r\n");
        
        while(NIRQ_Value);
        
        XTAPP_NFC_IRQ_Parser();
    }  
    
    UART0_SendStr("\r\nSelect a Task: ");
}

/**
 * @brief  RF main scan loop: poll NIRQ and process NFC commands
 */
void XTAPP_NFC_Scan(void)
{
    if (rf_err_mode == 1)
        return;

    /* Check Chip-Interrupt status is relatived to NFC */
    if (NIRQ_Value == 0)
    {
        uint8_t u8Array[4];
        UART0_SendStr("NIRQ_Value\r\n");
        //XTAPP_IrqHdlr();
        TRx_GetIntStatus(u8Array);
        TRx_ClearIntFlag(u8Array);
        if (IS_INT_ST_NFC(u8Array))
        {
            Delay_ms(10); //waiting for stable NFC event
            /* check and clear NFC event */
            XTAPP_NFC_IRQ_Parser();    
            UART0_SendStr("\r\n+--- NFC Attached ---+\r\n");
            /* Enter to NFC mode to process advanced NFC Interrupt event */
            while(u8NfcPwrGood)
            {
                uint8_t status_bit_mask;
                /* Poll PWRGOOD bit — if clear, RF field has been removed */
                TRx_READREG(NFC_DPE_BLOCK_ADDRESS, u8Array);
                status_bit_mask = u8Array[NFC_DPE_STATUS_BYTE_IDX];
                if (!(status_bit_mask & NFC_PWRGOOD_MASK))
                {   /* Field detached */                
                    u8DettachedCnt++;
                    Delay_ms(10);
                    if(u8DettachedCnt >= DETACHED_CNT_THR)
                    {
                        u8NfcPwrGood = 0;
                        NFC_Select_IRQ_Src(NFC_PWRGOOD); // Re-arm: wait for next field
                        UART0_SendStr("\r\n+--- NFC Detached ---+\r\n");
                        return;
                    }
                }
                else
                {
                    u8DettachedCnt = 0;
                }
                
                /* NFC UART command processor */
                XTAPP_NFC_Task();
            }
        }
    }
}
