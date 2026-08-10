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
#define RF_BUF_SIZE         64
#define RF_TX_SIZE          64
#define PER_TX_COUNT        500     /* total packets to send per PER test */
#define PER_TX_INTERVAL_MS  50      /* delay between TX packets (ms) */
/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
xdata_u8           rf_err_mode;
t_xtapp_config_t   xdata gtXtAppConfigInfo;
xdata_u8           gXtBuffer[RF_BUF_SIZE];
static uint16_t    gPerRxOk    = 0;    /* packets received with CRC pass */
static uint16_t    gPerRxTotal = 0;    /* total packets received */
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
    gtXtAppConfigInfo.rx_mode = RX_TRANSPARENT;
    gtXtAppConfigInfo.max_rx_size = RF_BUF_SIZE;
    rf_err_mode = 1;
    
    TRx_IoConfig();

    if( XTAPP_Strobe() == 0 )   // Check RF chip present
    {
        TRx_SW_Reset();
        
        if(TRx_Init() == TRx_STATUS_SUCCESS);   // Default initialization
        {
            TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
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
 * @note   On RX event: verifies XOR checksum of the received packet,
 *         updates gPerRxOk / gPerRxTotal, and prints PER result when
 *         gPerRxTotal reaches PER_TX_COUNT.
 *         Packet format: byte[0..N-2] = payload, byte[N-1] = XOR of byte[0..N-2]
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
            uint8_t i;
            uint8_t crc;

            XTAPP_ReceiveCheck(&len);
            TRx_GetRSSI_Data(&u16Rssi);
            u16Rssi = (4095 - u16Rssi) / 8;

            gPerRxTotal++;

            /* Verify XOR checksum: byte[N-1] must equal XOR of byte[0..N-2] */
            crc = 0;
            for (i = 0; i < RF_BUF_SIZE - 1; i++)
                crc ^= gXtBuffer[i];

            if (len == RF_BUF_SIZE && crc == gXtBuffer[RF_BUF_SIZE - 1])
            {
                gPerRxOk++;
                UART0_SendStr("RX OK  #");
                UART0_SendDec3(gPerRxTotal);
                UART0_SendStrDec3("  rssi: -", u16Rssi, 0);
            }
            else
            {
                UART0_SendStr("RX CRC ERR #");
                UART0_SendDec3(gPerRxTotal);
                UART0_SendStrDec3("  rssi: -", u16Rssi, 0);
            }
            /* Running PER based on received count so far */
            UART0_SendStrDec3("  PER:", (gPerRxTotal - gPerRxOk) * 100 / gPerRxTotal, 0);
            UART0_SendStr("%\r\n");

            /* Print PER result after receiving all PER_TX_COUNT packets */
            if (gPerRxTotal >= PER_TX_COUNT)
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
                gPerRxOk    = 0;
                gPerRxTotal = 0;
            }
        }
        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
/**
 * @brief  RF main scan loop: poll NIRQ and PER TX trigger (P05)
 * @note   P05 pressed → send PER_TX_COUNT (500) packets back-to-back with
 *         PER_TX_INTERVAL_MS (10ms) gap between each.
 *         Packet format: byte[0]   = high byte of sequence number
 *                        byte[1]   = low  byte of sequence number
 *                        byte[2..N-2] = incremental pattern
 *                        byte[N-1] = XOR checksum of byte[0..N-2]
 *         RX-side CRC verification and PER calculation are done in XTAPP_IrqHdlr().
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
        uint8_t  crc;
        uint16_t txCnt;

        UART0_SendStr("PER TX start (");
        UART0_SendDec3(PER_TX_COUNT);
        UART0_SendStr(" pkts)\r\n");

        for (txCnt = 0; txCnt < PER_TX_COUNT; txCnt++)
        {
            /* Build packet */
            gXtBuffer[0] = (uint8_t)(txCnt >> 8);      /* seq high byte */
            gXtBuffer[1] = (uint8_t)(txCnt & 0xFF);    /* seq low  byte */
            for (i = 2; i < RF_TX_SIZE - 1; i++)
                gXtBuffer[i] = i;                       /* incremental pattern */

            /* XOR checksum: covers byte[0..N-2] */
            crc = 0;
            for (i = 0; i < RF_TX_SIZE - 1; i++)
                crc ^= gXtBuffer[i];
            gXtBuffer[RF_TX_SIZE - 1] = crc;

            XTAPP_SendData(gXtBuffer, RF_TX_SIZE);
            while(NIRQ_Value);                          /* wait TX done */
            TRx_GetIntStatus(u8Array);
            TRx_ClearIntFlag(u8Array);

            Delay_ms(PER_TX_INTERVAL_MS);
        }

        UART0_SendStr("PER TX done.\r\n");

        while(P05 == 0);    /* wait button release */

        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));  /* restart RX */
    }
}
