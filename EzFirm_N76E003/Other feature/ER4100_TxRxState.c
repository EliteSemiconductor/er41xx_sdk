/**
 * @file    ANCTL_ST_REG_1EFC.c
 * @brief   Bit-field decode reference for ANCTL_ST_REG (0x1EFC), transcribed
 *          from the RTL localparam state definitions.
 *
 * ANCTL_ST_REG (0x1EFC) overall layout:
 *   [15:0]  ANCTL_CUR_ST       - current analog controller state (packed as
 *                                per-state-machine nibbles, see below)
 *   [18:16] ANCTL_CUR_DCDC_TAR
 *   [31:19] RESERVED
 *
 * ANCTL_CUR_ST (bits 15:0) is itself packed as one nibble per sub state
 * machine:
 *   [3:0]  ST_TOP - top-level state machine
 *   [7:4]  ST_RX  - RX state machine
 */

/* [3:0] ST_TX_TOP - top-level state machine */
#define ST_TX_TOP_INIT     0   // Initial state
#define ST_TX_TOP_IDLE     1   // IDLE state
#define ST_TX_TOP_STR      2   // Startup
#define ST_TX_TOP_TPM      3   // TPM calibration
#define ST_TX_TOP_RU       4   // Ramp up
#define ST_TX_TOP_DATHND   5   // 1st Data handle
#define ST_TX_TOP_DAT      6   // Data transfer
#define ST_TX_TOP_RD       7   // Ramp down
#define ST_TX_TOP_W4R      8   // Wait for Ramp state back to IDLE
#define ST_TX_TOP_PD       9   // Power Down
#define ST_TX_TOP_END      10  // End state
#define ST_TX_TOP_PU       11  // Power Up
#define ST_TX_TOP_PKTMOD   12  // Packet Mode
#define ST_TX_TOP_W4DAT    13  // Wait for data ready
#define ST_TX_TOP_TPMSET   14  // TPM settling
#define ST_TX_TOP_FSSET    15  // FS settling

/* [7:4] ST_RX - RX state machine */
#define ST_RX_IDLE      0
#define ST_RX_PU        1
#define ST_RX_SETUP     2
#define ST_RX_STR       3
#define ST_RX_LDCAP     4
#define ST_RX_PLL       5
#define ST_RX_IQ        6
#define ST_RX_RCV       7
#define ST_RX_EXIT      8
#define ST_RX_PD        9
#define ST_RX_FSSET     10

/* SPI_ER41xx.h declares TRx_READREG(uint16_t Addr, uint8_t *TRxData) */
#if 0 //32bit
/* [11:8] ST_TX - TX state machine */
/* Extract each state field from a raw ANCTL_ST_REG (0x1EFC) 32-bit value */
#define GET_ST_TX_TOP(reg32)  (((reg32) >> 0) & 0xF)
#define GET_ST_RX(reg32)   (((reg32) >> 4) & 0xF)
#else //8bit array
/* Read ANCTL_ST_REG (0x1EFC) and extract ST_TOP ([3:0] of byte0) */
uint8_t GET_ST_TX_TOP(void)
{
    uint8_t u8Array[4];
    TRx_READREG(0x1EFC, u8Array);
    return (u8Array[0] & 0xF);
}

/* Read ANCTL_ST_REG (0x1EFC) and extract ST_RX ([7:4] of byte0) */
uint8_t GET_ST_RX(void)
{
    uint8_t u8Array[4];
    TRx_READREG(0x1EFC, u8Array);
    return ((u8Array[0] >> 4) & 0xF);
}
#endif