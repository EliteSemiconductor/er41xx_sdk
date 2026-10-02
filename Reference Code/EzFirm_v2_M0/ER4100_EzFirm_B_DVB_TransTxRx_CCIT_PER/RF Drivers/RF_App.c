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
    /* addr is unused here; only the register bytes (b0..b3, LSB first) matter */
    t_cfg_reg xo_cfg     = { XO_CONFIG_1 };      // XTAL Cap IO
    t_cfg_reg freq_cfg   = { SYNTH_CAL_CONFIG_0 };
    uint8_t   rate_bytes[4] = GLB_DATA_RATE;
    uint8_t   dev_bytes[4]  = GLB_DEVIATION;
//    t_cfg_reg pa0_cfg    = { PA1G_CONFIG_0 };
//    t_cfg_reg pa1_cfg    = { PA1G_CONFIG_1 };
//    t_cfg_reg pa2_cfg    = { PA1G_CONFIG_2 };
    t_cfg_reg pwr_cfg    = { ANCTL_CONFIG_7 };    // TX power level
    t_cfg_reg fifo_cfg   = { FIFO_FEA_REG };      // FIFO data order
    t_cfg_reg int_cfg    = { INT_EN_REG };
    t_cfg_reg txmode_cfg = { MAC_TX_CFG };        // Tx FIFO mode
    uint32_t u32Val;

    printf("===== RF Config from Toolkit =====\r\n");

    printf("Crystal Hz     : %lu Hz\r\n", (unsigned long)CRYSTAL_HZ);

    /* CAP IO occupies bit6:0 of XO_CONFIG_1 byte1, bit7 is a separate enable flag */
    printf("Cap IO         : %u\r\n", (unsigned)(xo_cfg.reg_data[1] & 0x7F));

    u32Val = MAKE_U32(rate_bytes[3], rate_bytes[2], rate_bytes[1], rate_bytes[0]);
    u32Val &= ~(1UL << 24);   // bit24 is the extended-rate flag, not part of the value
    printf("Data Rate      : %lu bps\r\n", (unsigned long)u32Val);

    u32Val = MAKE_U32(dev_bytes[3], dev_bytes[2], dev_bytes[1], dev_bytes[0]);
    printf("Deviation      : %lu Hz\r\n", (unsigned long)u32Val);

    u32Val = MAKE_U32(freq_cfg.reg_data[3], freq_cfg.reg_data[2], freq_cfg.reg_data[1], freq_cfg.reg_data[0]);
    printf("Frequency      : %lu Hz\r\n", (unsigned long)u32Val);

//    printf("TX Power PA    : PA1G_CFG0=0x%02X%02X%02X%02X PA1G_CFG1=0x%02X%02X%02X%02X PA1G_CFG2=0x%02X%02X%02X%02X\r\n",
//           pa0_cfg.reg_data[3], pa0_cfg.reg_data[2], pa0_cfg.reg_data[1], pa0_cfg.reg_data[0],
//           pa1_cfg.reg_data[3], pa1_cfg.reg_data[2], pa1_cfg.reg_data[1], pa1_cfg.reg_data[0],
//           pa2_cfg.reg_data[3], pa2_cfg.reg_data[2], pa2_cfg.reg_data[1], pa2_cfg.reg_data[0]);

    printf("TX Power Level : %u\r\n", (unsigned)((pwr_cfg.reg_data[3]>>1) & 0x7F));

    printf("FIFO Order     : 0x%02X (MSB_INV=%u)\r\n",
           fifo_cfg.reg_data[0], (unsigned)(fifo_cfg.reg_data[0] & 0x01));

    printf("INT Ena Mask   : 0x%02X%02X%02X%02X (RX=%u TX=%u)\r\n",
           int_cfg.reg_data[3], int_cfg.reg_data[2], int_cfg.reg_data[1], int_cfg.reg_data[0],
           (unsigned)(IS_INT_ST_RX(int_cfg.reg_data) ? 1 : 0),
           (unsigned)(IS_INT_ST_TX(int_cfg.reg_data) ? 1 : 0));

    printf("TX FIFO Mode   : 0x%02X\r\n", txmode_cfg.reg_data[0]);

    printf("RX FIFO Mode   : 0x%02X (set by gtXtAppConfigInfo.rx_mode in FW)\r\n", gtXtAppConfigInfo.rx_mode);

    printf("Syncword       : 0x%08lX\r\n", (unsigned long)PREDEFINED_RX_SYNCWORD);
   
    printf("\r\n");
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
        printf("RX: bad length\r\n");
        return 0;
    }

    payload_len = buf[0] - 4;   // buf[0] = seq(2) + payload[1~123] + crc(2)
    // Standard transparent mode always fills the fixed-size RX buffer (max_rx_size),
    // so pkt_len is constant regardless of the actual frame length - it is NOT the
    // real payload boundary. Only bound-check against it (avoid reading past what
    // was actually captured); the real pass/fail signal is the CRC check below.
    if((uint8_t)(payload_len + 5) > pkt_len)
    {
        printf("RX: len mismatch (payload len exceeds pkt len)\r\n");
        return 0;
    }

    seq_num = ((uint16_t)buf[1] << 8) | (uint16_t)buf[2];
    crc_calc = ccit_calculator((uint8_t)(payload_len + 2), &buf[1]);   // CRC over seq + payload only, excludes buf[0]
    crc_rcvd = (uint16_t)buf[payload_len + 3] | ((uint16_t)buf[payload_len + 4] << 8);
    if(crc_calc != crc_rcvd)
    {
        return 0;
    }
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
 * @brief  Trigger RF TX to send data
 * @param  tx_data:  pointer to data buffer
 *         data_len: number of bytes to send (max 128)
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
                printf("CRC OK : ");
                //dump_buffer(gXtBuffer, valid_len, 1, u16Rssi);
            }
            else
            {
                gPerRxErr++;
                printf("====>CRC ERR : %u", gPerRxOk);
            }
            
            printf("pass:%03u, err:%03u, rssi:-%u dBm, ",  (unsigned)gPerRxOk, (unsigned)gPerRxErr, (unsigned)u16Rssi);
            /* Running PER based on received count so far */
            loss_count = PER_TX_COUNT - gPerRxOk;
            integer_part = (loss_count * 100) / PER_TX_COUNT;
            remainder = (loss_count * 100) % PER_TX_COUNT;
            fractional_part = (remainder * 100) / PER_TX_COUNT;
            printf(" PER:%u.%02u%%", integer_part, fractional_part);  // Use %02u to ensure leading zeros for numbers < 10 (e.g., 0.05%)
            //printf(" PER:%u.%02u%%", remainder, fractional_part); 
            printf("\r\n");
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
    /* PER TX trigger: DVB_SW5_PIN sends PER_TX_COUNT packets */
    if(DVB_SW5_PIN == 0)
    {
        uint8_t  u8Array[4];
        uint8_t  i;
        //uint8_t  crc;
        uint16_t txCnt;

        printf("PER TX start (%u pkts)\r\n", (unsigned)PER_TX_COUNT);

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
            
            printf(".");
            Delay_ms(PER_TX_INTERVAL_MS);
        }

        printf("\r\nPER TX done.\r\n");

        while(DVB_SW5_PIN == 0);    /* wait button release */

        TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                       (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));  /* restart RX */
    }
}
