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
// RF_BUF_SIZE: expected RX payload length passed to TRx_RX_Trigger().
//     802.15.4 mode:               this value is ignored; MAC layer reads the
//                                 PHR (Physical Header) in the received frame
//                                 to determine the actual payload length.
//     Standard transparent mode:   1 ~ 128 bytes
//     Long packet transparent mode: 1 ~ 2047 bytes
#define RF_BUF_SIZE  128
#define RF_TX_SIZE   64 //must be less than or equal to RF_BUF_SIZE

// 802.15.4 Format Description 
// [PHR][MHR][PAYLOAD][CRC]
// PHR (1 byte): total length of MHR + PAYLOAD + CRC in bytes
// MHR is mac header as following
//     0x41,0xC8,0x00,       [FrameCtrl:41C8][SeqNum:00]
//     0xCD,0xAB,0xFF,0xFF,  [DestPAN_ID:ABCD][DestAddr:FFFF]
//     0x02,0x00,0x00,0xAB,  [SrcAddr:000000AAAB00000002]
//     0xAA,0x00,0x00,0x00,
// CRC (2 bytes): CRC-16 over MHR and PAYLOAD
#define M802154_MHR_PATTERN \
    0x41,0xC8,0x00, \
    0xCD,0xAB,0xFF,0xFF, \
    0x02,0x00,0x00,0xAB, \
    0xAA,0x00,0x00,0x00, \

#define IDX_MHR         0
#define IDX_PAYLOAD     15

#define PER_TX_COUNT        500     /* total packets to send per PER test */
#define PER_TX_INTERVAL_MS  50      /* delay between TX packets (ms) */
/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
xdata_u8           rf_err_mode;
t_xtapp_config_t   xdata gtXtAppConfigInfo;
code const uint8_t SamplePattern[] = {M802154_MHR_PATTERN};
xdata_u8           gXtBuffer[RF_BUF_SIZE];
xdata_u8           gu8TxSeqNum;
static uint16_t    gPerRxOk    = 0;    /* packets received with HW CRC pass */
static uint16_t    gPerRxErr   = 0;    /* packets received with HW CRC error */
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
    gtXtAppConfigInfo.rx_mode = RX_802154;
    gtXtAppConfigInfo.max_rx_size = RF_BUF_SIZE;    
    gu8TxSeqNum = 0;
    TRx_IoConfig();

    if( XTAPP_Strobe() == 0 )   // Check RF chip present
    {
        TRx_SW_Reset();
        if(TRx_Init() == TRx_STATUS_SUCCESS);   // Default initialization
        {
            TRx_RX_Trigger(0,   // In 802.15.4 mode, the actual RX payload length is determined by the PHR in the received frame, so we can set this to 0 or any value within the max limit.
                           (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Start RX
            rf_err_mode = 0;
        }
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
 * @brief  Trigger RF TX to send data
 * @param  tx_data:  pointer to data buffer
 *         data_len: number of bytes to send (max 128)
 * @retval None
 */
void XTAPP_SendData(uint8_t* tx_data, uint8_t data_len)
{
    xdata_u8 len = (data_len > 128) ? 128 : data_len;   
    TRx_RxOff();    //If RX is enabled, RX must be disabled now.
    TRx_TX_FIFO(len, tx_data);
    TRx_TX_Trigger(len, 0);
}
/**
 * @brief  RF IRQ handler: process interrupt events from transceiver
 * @note   Hardware CRC is handled by ER4100:
 *           IS_INT_ST_RX    = received with CRC pass → gPerRxOk++
 *           IS_INT_ST_RXERR = received with CRC error → gPerRxErr++
 *         Prints PER result when gPerRxOk + gPerRxErr reaches PER_TX_COUNT.
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
        if(IS_INT_ST_RX(u8Array))
        {   /* RX OK — hardware CRC passed */
            xdata_u16 len;
            xdata_u16 u16Rssi;
            uint16_t total;
            XTAPP_ReceiveCheck(&len);
            TRx_GetRSSI_Data(&u16Rssi);
            u16Rssi = (4095 - u16Rssi) / 8;

            gPerRxOk++;
            total = gPerRxOk + gPerRxErr;
            UART0_SendStr("RX OK  #");
            UART0_SendDec3(total);
            UART0_SendStrDec3("  rssi: -", u16Rssi, 0);
            UART0_SendStrDec3("  PER:", gPerRxErr * 100 / total, 0);
            UART0_SendStr("%\r\n");
        }
        else if(IS_INT_ST_RXERR(u8Array))
        {   /* RX error — hardware CRC failed */
            uint16_t total;
            gPerRxErr++;
            total = gPerRxOk + gPerRxErr;
            TRx_RX_FIFOReset();

            /* Print 802.15.4 error detail */
            TRx_READREG(0xA004, u8Array);
            UART0_SendStr("RX CRC ERR #");
            UART0_SendDec3(total);
            UART0_SendStrDec3("  CrcErr:", (u8Array[0] >> 1) & 0x01, 1);
            UART0_SendStrDec3("  FmtErr:", (u8Array[0] >> 2) & 0x01, 1);
            UART0_SendStrDec3("  PER:", gPerRxErr * 100 / total, 0);
            UART0_SendStr("%\r\n");
        }

        /* Print PER result after receiving all PER_TX_COUNT packets */
        if ((gPerRxOk + gPerRxErr) >= PER_TX_COUNT)
        {
            uint16_t lost = PER_TX_COUNT - gPerRxOk;
            UART0_SendStr("--- PER Result ---\r\n");
            UART0_SendStr("RX OK : ");
            UART0_SendDec3(gPerRxOk);
            UART0_SendStr(" / ");
            UART0_SendDec3(PER_TX_COUNT);
            UART0_SendStr("\r\nLost  : ");
            UART0_SendDec3(lost);
            UART0_SendStr("\r\nPER   : ");
            UART0_SendDec3((lost * 100) / PER_TX_COUNT);
            UART0_SendStr("%\r\n");
            /* Reset counters for next test */
            gPerRxOk  = 0;
            gPerRxErr = 0;
        }

        TRx_RX_Trigger(0, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));
    }
}
/**
 * @brief  RF main scan loop: poll NIRQ and PER TX trigger (P05)
 * @note   P05 pressed → send PER_TX_COUNT (500) 802.15.4 packets back-to-back
 *         with PER_TX_INTERVAL_MS (50ms) gap between each.
 *         Packet format: [MHR(15 bytes)][seq(1 byte)][pattern...][HW CRC(2 bytes auto)]
 *         Hardware CRC is appended automatically by ER4100.
 *         RX-side PER calculation is done in XTAPP_IrqHdlr().
 * @param  None
 * @retval None
 */
void XTAPP_Scan(void)
{
    if(rf_err_mode == 1)
        return;
    /* IRQ handler: process RX / TX complete events */
    if(NIRQ_Value == 0)
    {
        XTAPP_IrqHdlr();
    }
    /* PER TX trigger: P05 sends PER_TX_COUNT packets */
    if(P05 == 0)
    {
        uint8_t  u8Array[4];
        uint8_t  i;
        uint16_t txCnt;

        gu8TxSeqNum = 0;
        UART0_SendStr("PER TX start (");
        UART0_SendDec3(PER_TX_COUNT);
        UART0_SendStr(" pkts)\r\n");

        for (txCnt = 0; txCnt < PER_TX_COUNT; txCnt++)
        {
            /* Copy MHR to buffer */
            for (i = 0; i < sizeof(SamplePattern); i++)
                gXtBuffer[i] = SamplePattern[i];

            /* Set payload: first byte = seq, rest = incremental pattern */
            gXtBuffer[IDX_PAYLOAD] = gu8TxSeqNum++;
            for (i = IDX_PAYLOAD + 1; i < RF_TX_SIZE; i++)
                gXtBuffer[i] = i;

            XTAPP_SendData(gXtBuffer, RF_TX_SIZE);
            while(NIRQ_Value);                  /* wait TX done */
            TRx_GetIntStatus(u8Array);
            TRx_ClearIntFlag(u8Array);

            Delay_ms(PER_TX_INTERVAL_MS);
        }

        UART0_SendStr("PER TX done.\r\n");

        while(P05 == 0);    /* wait button release */

        TRx_RX_Trigger(0, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   /* restart RX */
    }
}
