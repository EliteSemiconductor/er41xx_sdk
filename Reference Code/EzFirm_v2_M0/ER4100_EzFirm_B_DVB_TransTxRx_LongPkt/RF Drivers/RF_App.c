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

    printf("===== RF Config form Toolkit=====\r\n");

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
}

/**
 * @brief  Calculate CRC-16/CCITT-Kermit over a data buffer.
 *         Shared with the CCIT project to keep CRC behavior identical.
 * @param  len: number of bytes to include in the CRC
 *         dat: pointer to the data buffer
 * @retval Calculated 16-bit CRC value
 */
uint16_t ccit_calculator(uint8_t len, uint8_t *dat)
{
    uint16_t crc = 0x0000;
    uint8_t i;

    while(len--)
    {
        crc ^= (uint16_t)(*dat++);
        for(i = 0; i < 8; i++)
        {
            if(crc & 0x0001)
            {
                crc = (uint16_t)((crc >> 1) ^ 0x8408);
            }
            else
            {
                crc = (uint16_t)(crc >> 1);
            }
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
        
        if(TRx_Init() == TRx_STATUS_SUCCESS)   // Default initialization
        {
            gu16AlreadyRcvdSize = 0; 
            gu8PacketNumber = 0;
            TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                           (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Start RX
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
 * @brief  Validate and print a received long packet: packet number and CRC.
 *         Match the CCIT project naming and calculation flow.
 * @param  buf:     pointer to received packet buffer
 *         pkt_len: total packet length in bytes
 * @retval 1: valid packet, 0: invalid packet
 */
static uint8_t XTAPP_CheckRxFrame(uint8_t *buf, uint8_t pkt_len)
{
    uint16_t pkt_len_full = ((uint16_t)buf[0] << 8) | buf[1];
    uint8_t  pkt_num = buf[2];
    uint16_t crc_calc;
    uint16_t crc_rcvd;

    printf("PktNum:%02X", pkt_num);

    if(pkt_len_full >= 3 && pkt_len_full <= RF_BUF_SIZE)
    {
        crc_calc = ccit_calculator((uint8_t)(pkt_len_full - 3), &buf[1]);
        crc_rcvd = (uint16_t)buf[pkt_len_full - 2] | ((uint16_t)buf[pkt_len_full - 1] << 8);

        if(crc_calc == crc_rcvd)
        {
            printf(" CRC:OK\r\n");
            return 1;
        }

        printf(" CRC:FAIL calc=%04X rcvd=%04X\r\n", crc_calc, crc_rcvd);
        return 0;
    }

    printf(" LEN:INVALID\r\n");
    return 0;
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
        result = XTAPP_CheckRxFrame(dest_buf, gtXtAppConfigInfo.max_rx_size);
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
        if(RF_NIRQ_VAL() == 0)
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
                printf(":sent\r\n");
                break;
            }
        }
        /* Timeout guard: ~250 ms (5000 × Delay_10us(5)) */
        if(++timeout_cnt > 5000)
        {
            printf("TX Timeout!\r\n");
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
    if(RF_NIRQ_VAL() == 0)
    {
        XTAPP_IrqHdlr();
    }
    /* Long packet TX Trigger and sent process */
    if(DVB_SW5_PIN == 0)
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
        crc = ccit_calculator((uint8_t)(RF_TX_SIZE - 3), &gXtBuffer[1]);
        gXtBuffer[RF_TX_SIZE-2] = crc &0xFF;
        gXtBuffer[RF_TX_SIZE-1] = crc >>8;
        
        dump_buffer(gXtBuffer, RF_TX_SIZE, 0, 0);
        XTAPP_LongPktTransmit(gXtBuffer, RF_TX_SIZE);
           
        while(DVB_SW5_PIN == 0);   // Wait for button release
        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
