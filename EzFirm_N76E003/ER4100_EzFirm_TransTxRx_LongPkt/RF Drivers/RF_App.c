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
#define RF_BUF_SIZE             512
#define RF_TX_SIZE              512 //Must be less than or equal to RF_BUF_SIZE, and also must be less than 2048-3=2045 due to 3 bytes overhead (2 bytes length + 1 byte packet number) added in the beginning of the payload for long packet mode
#define RF_RX_ALMOST_FULL_THR   64  //Same as bit23:16 of 0xA0A4 in TRx_ConfigEx_Array[ ]
#define RF_TX_ALMOST_EMPTY_THR  64  //Same as bit7:0 of 0xA0A4 in TRx_ConfigEx_Array[ ]
/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/
xdata_u8           rf_err_mode;
t_xtapp_config_t   xdata gtXtAppConfigInfo;
xdata_u8           gXtBuffer[RF_BUF_SIZE];
uint16_t           gu16AlreadyRcvdSize;
uint8_t            gu8PacketNumber;
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
 * @brief  Calculate CRC-16/CCITT-Kermit over a data buffer.
 *         Algorithm parameters:
 *           - Polynomial : 0x1021 (reflected = 0x8408)
 *           - Initial value: 0x0000
 *           - Input / Output reflected: yes
 *           - Final XOR  : 0x0000
 *
 * @param  pData    Pointer to the data buffer.
 * @param  DataLen  Number of bytes to process.
 * @retval Calculated 16-bit CRC value.
 *         Returns 0x0000 if DataLen is 0.
 */
uint16_t crc16CcitKermit( uint8_t* pData, uint16_t DataLen)
{
    uint32_t i;
    uint16_t crc = 0x0000;

    if(DataLen <= 0)
        return 0x0000;

    while(DataLen--)
    {
        crc ^= *pData++;            /* XOR next byte into CRC low byte */
        for(i=0; i<8; i++)          /* Process each bit, LSB first */
        {
            if(crc & 1)
                crc = (crc >> 1) ^ 0x8408;  /* Reflected polynomial 0x1021 */
            else
                crc = (crc >> 1);
        }
    }
    return crc;
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
                UART0_SendStr("errID\r\n");
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
            gu16AlreadyRcvdSize = 0; 
            gu8PacketNumber = 0;
            TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                           (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Start RX
            rf_err_mode = 0;
        }
    }
    return rf_err_mode;
}
/**
 * @brief  Verify the CRC-16 of a received long packet.
 *         Packet layout expected in dest_buf (see file header for full map):
 *           dest_buf[0]        : Length high byte
 *           dest_buf[1]        : Length low byte   ──┐ CRC covers
 *           dest_buf[2]        : Packet number       │ dest_buf[1]
 *           dest_buf[3..N-3]   : Payload             │   ..
 *           dest_buf[N-2]      : CRC16 low byte    ──┘ dest_buf[N-4]
 *           dest_buf[N-1]      : CRC16 high byte
 *         (N = pkt_len parsed from dest_buf[0:1])
 *
 *         Steps:
 *           1. Parse pkt_len from dest_buf[0:1] (big-endian).
 *           2. Validate pkt_len (3 <= pkt_len <= RF_BUF_SIZE).
 *           3. Recalculate CRC over dest_buf[1..pkt_len-4] (pkt_len-3 bytes).
 *           4. Read stored CRC from dest_buf[pkt_len-2] (low) and
 *              dest_buf[pkt_len-1] (high), little-endian.
 *           5. Compare and print result to UART.
 *
 * @param  dest_buf  Pointer to the fully received packet buffer.
 * @retval 1  CRC matched — packet is valid.
 * @retval 2  CRC mismatch or pkt_len out of range — packet is corrupt.
 */
uint8_t XTAPP_CheckRxCRC(uint8_t *dest_buf)
{
    uint16_t pkt_len  = ((uint16_t)dest_buf[0] << 8) | dest_buf[1];
    uint8_t  pkt_num  = dest_buf[2];
    uint16_t crc_calc, crc_rcvd;
    uint8_t result = 0;

    UART0_SendStr("PktNum:");
    UART0_SendHex8(pkt_num);

    if(pkt_len >= 3 && pkt_len <= RF_BUF_SIZE)
    {
        /* Recalculate CRC over dest_buf[1]..[pkt_len-4] */
        crc_calc = crc16CcitKermit(&dest_buf[1], pkt_len - 3);
        /* Read stored CRC (little-endian: low byte first) */
        crc_rcvd = (uint16_t)dest_buf[pkt_len - 2] |
                   ((uint16_t)dest_buf[pkt_len - 1] << 8);
        if(crc_calc == crc_rcvd)
        {
            UART0_SendStr(" CRC:OK\r\n");
            result = 1;
        }
        else
        {
            UART0_SendStr(" CRC:FAIL calc=");
            UART0_SendHex8(crc_calc >> 8);
            UART0_SendHex8(crc_calc & 0xFF);
            UART0_SendStr(" rcvd=");
            UART0_SendHex8(crc_rcvd >> 8);
            UART0_SendHex8(crc_rcvd & 0xFF);
            UART0_SendStr("\r\n");
            result = 2;
        }
    }
    else
    {
        UART0_SendStr(" LEN:INVALID\r\n");
        result = 2;
    }

    return result;
}
/**
 * @brief  Long packet reception state machine.
 *         Called from XTAPP_IrqHdlr() on every RX-related interrupt.
 *         Handles two interrupt events:
 *
 *         [1] RX FIFO Almost Full (IS_INT_ST_RX_FIFO_AFULL):
 *             Fired periodically while the long packet is being received.
 *             Drains RF_RX_ALMOST_FULL_THR bytes from the FIFO into dest_buf
 *             and advances gu16AlreadyRcvdSize. Repeats until end-of-packet.
 *
 *         [2] RX End-of-Packet (IS_INT_ST_RX):
 *             Fired when the transceiver has received the last byte.
 *             Reads the remaining bytes (total - already received) from FIFO,
 *             resets gu16AlreadyRcvdSize, then calls XTAPP_CheckRxCRC() to
 *             verify the packet integrity.
 *
 * @param  evtArray  4-byte interrupt status array from TRx_GetIntStatus().
 * @param  dest_buf  Destination buffer (must be >= max_rx_size bytes).
 *                   Accumulated across multiple calls until EOP.
 * @retval 0  Packet not yet complete (intermediate chunk stored).
 * @retval 1  Whole packet received, CRC passed.
 * @retval 2  Whole packet received, CRC failed or length field invalid.
 */
uint8_t XTAPP_LongPktReceiveCheck(uint8_t *evtArray, uint8_t *dest_buf)
{
    uint8_t result = 0;
    /* 1. Handle Intermediate Data (FIFO Almost Full) */
    if(IS_INT_ST_RX_FIFO_AFULL(evtArray)){
        /* Boundary check to prevent buffer overflow */
        if (gu16AlreadyRcvdSize + RF_RX_ALMOST_FULL_THR <= RF_BUF_SIZE) {
            /* Drain one threshold-sized chunk from FIFO into dest_buf */
            TRx_RX_FIFOTHD(RF_RX_ALMOST_FULL_THR, &dest_buf[gu16AlreadyRcvdSize]);
            gu16AlreadyRcvdSize += RF_RX_ALMOST_FULL_THR;
        }
    }

    /* 2. Handle Packet Completion (End of Packet) */
    if(IS_INT_ST_RX(evtArray)){
        /* Read remaining bytes to complete the fixed-size packet */
        if (gtXtAppConfigInfo.max_rx_size > gu16AlreadyRcvdSize){
            uint16_t remaining = gtXtAppConfigInfo.max_rx_size - gu16AlreadyRcvdSize;
            TRx_RX_FIFOTHD(remaining, &dest_buf[gu16AlreadyRcvdSize]);
        }
        gu16AlreadyRcvdSize = 0;    /* Reset for next packet */

        /* Verify packet CRC and report result via UART */
        result = XTAPP_CheckRxCRC(dest_buf);
    }

    return result;
}
/**
 * @brief  Transmit a long packet (> XT_MAX_FIFO_SIZE bytes) via the ER4100.
 *         Because the packet exceeds the hardware FIFO depth, transmission is
 *         split into three stages:
 *
 *         [Stage 1] Pre-fill & Trigger:
 *             Fill the FIFO with the first XT_MAX_FIFO_SIZE bytes, then issue
 *             TRx_TX_Trigger() with the total packet length to start the RF
 *             carrier and begin transmitting.
 *
 *         [Stage 2] Refill loop (TX FIFO Almost Empty interrupt):
 *             While the transceiver is on-air, the FIFO drains and fires the
 *             TX_FIFO_AEMPTY interrupt. Each time, refill up to
 *             RF_TX_ALMOST_EMPTY_THR bytes from the remaining source data.
 *             Reset timeout_cnt on each successful refill to prevent false
 *             timeout during normal transmission.
 *
 *         [Stage 3] End of Packet (TX Done interrupt):
 *             IS_INT_ST_TX signals the last byte has been sent.
 *             Print ":sent" to UART and exit the loop.
 *
 *         A polling timeout (5000 × 50 µs ≈ 250 ms) guards against NIRQ
 *         never going low (e.g. hardware fault).
 *
 * @param  src_buf  Pointer to the TX data buffer.
 * @param  src_len  Total number of bytes to transmit (must be <= RF_BUF_SIZE).
 * @retval None
 */
void XTAPP_LongPktTransmit(uint8_t *src_buf, uint16_t src_len)
{
    uint8_t u8Array[4];
    xdata_u16 remaining = src_len;
    xdata_u8 *p_curr_tx_buf_position = src_buf;
    xdata_u16 timeout_cnt = 0;

    /* Stage 1: Fill FIFO with first chunk and start transmission */
    TRx_RxOff();                                    /* Disable RX before TX */
    TRx_TX_FIFO(XT_MAX_FIFO_SIZE, src_buf);         /* Pre-load FIFO */
    remaining -= XT_MAX_FIFO_SIZE;
    p_curr_tx_buf_position = src_buf + XT_MAX_FIFO_SIZE;
    TRx_TX_Trigger(src_len, 0);                     /* Start TX with total length */

    while(1)
    {
        if(NIRQ_Value == 0)
        {
            /* Stage 2 & 3: Handle TX interrupts */
            TRx_GetIntStatus(u8Array);
            TRx_ClearIntFlag(u8Array);

            if(IS_INT_ST_TX_FIFO_AEMPTY(u8Array))
            {   /* Stage 2: FIFO running low — refill with next chunk */
                uint16_t fill_size = (remaining > RF_TX_ALMOST_EMPTY_THR) ? RF_TX_ALMOST_EMPTY_THR : remaining;
                TRx_TX_FIFO(fill_size, p_curr_tx_buf_position);
                remaining -= fill_size;
                p_curr_tx_buf_position += fill_size;
                timeout_cnt = 0;                    /* Reset timeout after each refill */
            }

            if(IS_INT_ST_TX(u8Array))
            {   /* Stage 3: Entire packet transmitted */
                UART0_SendStr(":sent\r\n");
                break;
            }
        }
        /* Timeout guard: ~250 ms (5000 × Delay_10us(5)) */
        if(++timeout_cnt > 5000)
        {
            UART0_SendStr("TX Timeout!\r\n");
            break;
        }
        Delay_10us(5);
    }
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
        { 
            uint8_t result = XTAPP_LongPktReceiveCheck(u8Array, gXtBuffer);
            if(result == 1)
            {
                uint16_t rssi;
                TRx_GetRSSI_Data(&rssi);
                rssi = (4095-rssi)/8;
                dump_buffer(gXtBuffer, gtXtAppConfigInfo.max_rx_size, 1, rssi);
            }
            else if(result == 2)
            {
            }
        }
        
        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
/**
 * @brief  RF main scan loop: poll NIRQ and TX trigger button
 * @param  None
 * @retval None
 */
void XTAPP_Scan(void)
{
    if(rf_err_mode == 1)
        return;
    /*
    * Packet Structure (RF_BUF_SIZE bytes total, e.g. 512 bytes)
    *
    *  Byte  0        : Length high byte  (= RF_BUF_SIZE >> 8)
    *  Byte  1        : Length low  byte  (= RF_BUF_SIZE & 0xFF)
    *  Byte  2        : Packet number     (incremented each TX)
    *  Byte  3 ..
    *    .. (RF_BUF_SIZE-3) : Payload data
    *  Byte (RF_BUF_SIZE-2): CRC16 low  byte  \  CRC-16/CCITT-Kermit
    *  Byte (RF_BUF_SIZE-1): CRC16 high byte  /  calculated over bytes[1]..(RF_BUF_SIZE-4)
    */
    /* Long packet RX received process */
    if(NIRQ_Value == 0)
    {
        XTAPP_IrqHdlr();
    }
    /* Long packet TX Trigger and sent process */
    if(P05 == 0)
    {
        uint16_t i;
        uint16_t crc;
        for(i = 0; i < RF_TX_SIZE; i++)
        {
            gXtBuffer[i] = i;
        }
        
        gXtBuffer[0] = RF_TX_SIZE >> 8;    //Packet length from buf[2]~buf[RF_TX_SIZE-3]
        gXtBuffer[1] = RF_TX_SIZE &0xFF;
        gXtBuffer[2] = gu8PacketNumber++;     //Packet Number
        crc = crc16CcitKermit(&gXtBuffer[1], RF_TX_SIZE-3);
        gXtBuffer[RF_TX_SIZE-2] = crc &0xFF;
        gXtBuffer[RF_TX_SIZE-1] = crc >>8;
        
        dump_buffer(gXtBuffer, RF_TX_SIZE, 0, 0);
        XTAPP_LongPktTransmit(gXtBuffer, RF_TX_SIZE);
           
        while(P05 == 0);   // Wait for button release
        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
