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
static uint16_t     g_u16TxSeqNum = 0;
static uint16_t    gPerRxOk    = 0;    /* packets received with CRC pass */
static uint16_t    gPerRxErr    = 0;    /* packets received with CRC pass */
static uint16_t    gPerRxTotal = 0;    /* total packets received */
/* Function prototypes ------------------------------------------------------*/
static uint8_t XTAPP_CheckRxFrame(uint8_t *buf, uint8_t pkt_len);

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
 * @brief  Print the compiled RF configuration (crystal, cap IO, data rate, deviation,
 *         frequency, TX power, FIFO/interrupt setup, syncword) from the included
 *         SPI_ER41xx_config_*.h to UART.
 * @param  None
 * @retval None
 */
void dump_rf_config(void)
{
    t_cfg_reg xo_cfg     = { XO_CONFIG_1 };
    t_cfg_reg freq_cfg   = { SYNTH_CAL_CONFIG_0 };
    uint8_t   rate_bytes[4] = GLB_DATA_RATE;
    uint8_t   dev_bytes[4]  = GLB_DEVIATION;
    t_cfg_reg pwr_cfg    = { ANCTL_CONFIG_7 };
    t_cfg_reg fifo_cfg   = { FIFO_FEA_REG };
    t_cfg_reg int_cfg    = { INT_EN_REG };
    t_cfg_reg txmode_cfg = { MAC_TX_CFG };
    uint32_t u32Val;

    UART0_SendStr("===== RF Config from Toolkit =====\r\n");
    UART0_SendStr("Crystal Hz     : ");
    UART0_SendU32Dec((uint32_t)CRYSTAL_HZ);
    UART0_SendStr(" Hz\r\n");

    UART0_SendStr("Cap IO         : ");
    UART0_SendDec3((uint16_t)(xo_cfg.reg_data[1] & 0x7F));
    UART0_SendStr("\r\n");

    u32Val = MAKE_U32(rate_bytes[3], rate_bytes[2], rate_bytes[1], rate_bytes[0]);
    u32Val &= ~(1UL << 24);
    UART0_SendStr("Data Rate      : ");
    UART0_SendU32Dec(u32Val);
    UART0_SendStr(" bps\r\n");

    u32Val = MAKE_U32(dev_bytes[3], dev_bytes[2], dev_bytes[1], dev_bytes[0]);
    UART0_SendStr("Deviation      : ");
    UART0_SendU32Dec(u32Val);
    UART0_SendStr(" Hz\r\n");

    u32Val = MAKE_U32(freq_cfg.reg_data[3], freq_cfg.reg_data[2], freq_cfg.reg_data[1], freq_cfg.reg_data[0]);
    UART0_SendStr("Frequency      : ");
    UART0_SendU32Dec(u32Val);
    UART0_SendStr(" Hz\r\n");

    UART0_SendStr("TX Power Level : ");
    UART0_SendDec3((uint16_t)(pwr_cfg.reg_data[3]>>1 & 0x7F));
    UART0_SendStr("\r\n");

    UART0_SendStr("FIFO Order     : 0x");
    UART0_SendHex8(fifo_cfg.reg_data[0]);
    UART0_SendStr(" (MSB_INV=");
    UART0_SendDec3((uint16_t)(fifo_cfg.reg_data[0] & 0x01));
    UART0_SendStr(")\r\n");

    UART0_SendStr("INT Ena Mask   : 0x");
    UART0_SendHex8(int_cfg.reg_data[3]);
    UART0_SendHex8(int_cfg.reg_data[2]);
    UART0_SendHex8(int_cfg.reg_data[1]);
    UART0_SendHex8(int_cfg.reg_data[0]);
    UART0_SendStr(" (RX=");
    UART0_SendDec3((uint16_t)(IS_INT_ST_RX(int_cfg.reg_data) ? 1 : 0));
    UART0_SendStr(" TX=");
    UART0_SendDec3((uint16_t)(IS_INT_ST_TX(int_cfg.reg_data) ? 1 : 0));
    UART0_SendStr(")\r\n");

    UART0_SendStr("TX FIFO Mode   : 0x");
    UART0_SendHex8(txmode_cfg.reg_data[3]);
    UART0_SendHex8(txmode_cfg.reg_data[2]);
    UART0_SendHex8(txmode_cfg.reg_data[1]);
    UART0_SendHex8(txmode_cfg.reg_data[0]);
    UART0_SendStr("\r\n");

    UART0_SendStr("RX FIFO Mode   : ");
    UART0_SendHex8(gtXtAppConfigInfo.rx_mode);
    UART0_SendStr(" (set by gtXtAppConfigInfo.rx_mode in FW)\r\n");
    UART0_SendStr("Syncword       : 0x");
    UART0_SendU32Hex8((uint32_t)PREDEFINED_RX_SYNCWORD);
    UART0_SendStr("\r\n");
}
/**
 * @brief  Calculate CRC-16/KERMIT (poly 0x1021, init 0x0000, refin/refout, xorout
 *         0x0000) over a buffer. Implemented LSB-first with the bit-reversed
 *         polynomial (0x8408), which is algebraically equivalent to reflecting
 *         each input byte and the final CRC.
 * @param  len: number of bytes to include in the CRC
 *         dat: pointer to data buffer
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
 * @brief  Validate and print a received frame: length, sequence number, CRC-CCITT check
 * @param  buf:     pointer to received raw frame [len][seq_hi][seq_lo][payload...][crc_lo][crc_hi]
 *                  len = byte count following this byte (seq + payload + crc), excludes itself
 *         pkt_len: total received frame length in bytes
 * @retval Length(>0): frame passed length/CRC checks, 0: frame rejected
 */
static uint8_t XTAPP_CheckRxFrame(uint8_t *buf, uint8_t pkt_len)
{
    uint8_t payload_len;
    uint16_t seq_num;
    uint16_t crc_calc;
    uint16_t crc_rcvd;

    if(pkt_len < 5)
    {
        UART0_SendStr("RX: bad length\r\n");
        return 0;
    }

    payload_len = buf[0] - 4;   // buf[0] = seq(2) + payload + crc(2)
    // Standard transparent mode always fills the fixed-size RX buffer (max_rx_size),
    // so pkt_len is constant regardless of the actual frame length - it is NOT the
    // real payload boundary. Only bound-check against it (avoid reading past what
    // was actually captured); the real pass/fail signal is the CRC check below.
    if((uint8_t)(payload_len + 5) > pkt_len)
    {
        UART0_SendStr("RX: len mismatch\r\n");
        return 0;
    }

    seq_num = ((uint16_t)buf[1] << 8) | (uint16_t)buf[2];
    crc_calc = ccit_calculator((uint8_t)(payload_len + 2), &buf[1]);   // CRC over seq + payload only, excludes buf[0]
    crc_rcvd = (uint16_t)buf[payload_len + 3] | ((uint16_t)buf[payload_len + 4] << 8);

    UART0_SendStr("RX seq=");
    UART0_SendDec3(seq_num);
    UART0_SendStr(" ");

    if(crc_calc != crc_rcvd)
    {
        UART0_SendStr("CRC FAIL\r\n");
        return 0;
    }

    UART0_SendStr("CRC OK\r\n");
    return (payload_len+5);
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
 * @brief  Build a frame [len][seq_hi][seq_lo][payload...][crc_lo][crc_hi], append it
 *         to the TX FIFO, and trigger RF TX to send it
 *         len = byte count following this byte (seq + payload + crc), excludes itself
 * @param  tx_data:  pointer to data buffer
 *         data_len: number of bytes to send (clamped internally to RF_BUF_SIZE - 5)
 * @retval None
 */
void XTAPP_SendData(uint8_t* tx_data, uint8_t data_len)
{
    uint8_t frame_len;
    uint8_t seq_lo;
    uint8_t seq_hi;
    uint16_t crc;
    uint8_t i;
    xdata_u8 frame_buf[RF_BUF_SIZE];

    if(data_len > (RF_BUF_SIZE - 5))
    {
        data_len = RF_BUF_SIZE - 5;
    }

    frame_len = data_len + 5;
    seq_lo = (uint8_t)(g_u16TxSeqNum & 0x00FF);
    seq_hi = (uint8_t)((g_u16TxSeqNum >> 8) & 0x00FF);

    frame_buf[0] = data_len + 4;   // seq(2) + payload + crc(2), excludes this byte itself
    frame_buf[1] = seq_hi;
    frame_buf[2] = seq_lo;
    for(i = 0; i < data_len; i++)
    {
        frame_buf[3 + i] = tx_data[i];
    }

    crc = ccit_calculator((uint8_t)(data_len + 2), &frame_buf[1]);   // CRC over seq + payload only, excludes frame_buf[0]
    frame_buf[3 + data_len] = (uint8_t)(crc & 0x00FF);
    frame_buf[4 + data_len] = (uint8_t)((crc >> 8) & 0x00FF);

    TRx_RxOff();    //If RX is enabled, RX must be disabled now.
    TRx_TX_FIFO(frame_len, frame_buf);
    TRx_TX_Trigger(frame_len, 0);

    //dump_buffer(frame_buf, RF_TX_SIZE, 0, 0);

    g_u16TxSeqNum++;
}

static void XTAPP_PrintU16(uint16_t v)
{
    uint8_t digits[6];
    uint8_t idx = 0;

    if(v == 0)
    {
        Send_Data_To_UART0('0');
        return;
    }

    while(v > 0)
    {
        digits[idx++] = (uint8_t)('0' + (v % 10));
        v /= 10;
    }

    while(idx > 0)
    {
        Send_Data_To_UART0(digits[--idx]);
    }
}

static void XTAPP_PrintDec2(uint16_t v)
{
    Send_Data_To_UART0('0' + (uint8_t)(v / 10));
    Send_Data_To_UART0('0' + (uint8_t)(v % 10));
}

/**
 * @brief  RF IRQ handler: process interrupt events from transceiver
 * @param  None
 * @retval None
 */
void XTAPP_IrqHdlr(void)
{
    uint8_t  u8Array[4];
    uint8_t  valid_len = 0;
    TRx_GetIntStatus(u8Array);
    if(IS_INT_ST_ANY(u8Array))
    {
        TRx_ClearIntFlag(u8Array);

        if(IS_INT_ST_ANY_RX(u8Array))
        {   /* RX event */
            xdata_u16 len;
            xdata_u16 u16Rssi;
            
            xdata_u16 loss_count ;      //= PER_TX_COUNT - gPerRxOk;
            // Multiply by 100 first (Max value: 500 * 100 = 50,000 < 65,535, safe from u16 overflow)
            xdata_u16 integer_part;      // = (loss_count * 100) / PER_TX_COUNT;
            // Calculate remainder, then multiply by 100 to get 2 decimal places 
            // (Max value: 499 * 100 = 49,900 < 65,535, safe from u16 overflow)
            xdata_u16 remainder;      // = (loss_count * 100) % PER_TX_COUNT;
            xdata_u16 fractional_part;      // = (remainder * 100) / PER_TX_COUNT;
            

            XTAPP_ReceiveCheck(&len);
            TRx_GetRSSI_Data(&u16Rssi);
            u16Rssi = (4095 - u16Rssi) / 8;

            gPerRxTotal++;
            
            valid_len = XTAPP_CheckRxFrame(gXtBuffer, (uint8_t)len);
            if(valid_len >0)
            {
                gPerRxOk++;
                UART0_SendStr("CRC OK : ");
                //dump_buffer(gXtBuffer, valid_len, 1, u16Rssi);
            }
            else
            {
                gPerRxErr++;
                UART0_SendStr("====>CRC ERR : ");
                UART0_SendDec3(gPerRxOk);
            }

            UART0_SendStr("pass:");
            UART0_SendDec3(gPerRxOk);
            UART0_SendStr(", err:");
            UART0_SendDec3(gPerRxErr);
            UART0_SendStr(", rssi:-");
            UART0_SendDec3(u16Rssi);
            UART0_SendStr("dBm, ");
            /* Running PER based on received count so far */
            loss_count = PER_TX_COUNT - gPerRxOk;
            integer_part = (loss_count * 100) / PER_TX_COUNT;
            remainder = (loss_count * 100) % PER_TX_COUNT;
            fractional_part = (remainder * 100) / PER_TX_COUNT;
            UART0_SendStr(" PER:");
            XTAPP_PrintU16(integer_part);
            UART0_SendStr(".");
            XTAPP_PrintDec2(fractional_part);
            UART0_SendStr("%");
            UART0_SendStr("\r\n");
        }
        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
/**
 * @brief  RF main scan loop: poll NIRQ and PER TX trigger (DVB_SW5_PIN)
 * @note   DVB_SW5_PIN pressed → send PER_TX_COUNT (500) packets back-to-back with
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
    /* IRQ handler: process RX received / TX complete events */
    if(RF_NIRQ_VAL() == 0)
    {
        XTAPP_IrqHdlr();
    }
    /* TX trigger */
    if(P05 == 0)
    {
        uint8_t  u8Array[4];
        uint8_t  i;
        //uint8_t  crc;
        uint16_t txCnt;

        UART0_SendStr("PER TX start (");
        UART0_SendDec3(PER_TX_COUNT);
        UART0_SendStr(" pkts)\r\n");

        for (txCnt = 0; txCnt < PER_TX_COUNT; txCnt++)
        {
            for(i = 0; i < RF_TX_SIZE; i++)
            {
                gXtBuffer[i] = i;
            }

            XTAPP_SendData(gXtBuffer, RF_TX_SIZE);
            while(RF_NIRQ_VAL());                          /* wait TX done */
            TRx_GetIntStatus(u8Array);
            TRx_ClearIntFlag(u8Array);
						UART0_SendStr(".");
            Delay_ms(PER_TX_INTERVAL_MS);
        }

        UART0_SendStr("\r\nPER TX done.\r\n");
        
        while(P05 == 0);   // Wait for button release
        
        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                           (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
