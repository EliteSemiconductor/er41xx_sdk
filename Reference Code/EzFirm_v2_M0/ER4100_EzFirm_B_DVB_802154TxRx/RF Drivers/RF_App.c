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
code const uint8_t SamplePattern[] = {M802154_MHR_PATTERN};
xdata_u8           gXtBuffer[RF_BUF_SIZE];
xdata_u8           gu8TxSeqNum;
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

    /* Level occupies bit6:0 of ANCTL_CONFIG_7 byte3, bit7 is a separate enable flag */
    printf("TX Power Level : %u\r\n", (unsigned)((pwr_cfg.reg_data[3]>>1) & 0x7F));

    printf("FIFO Order     : 0x%02X (MSB_INV=%u)\r\n",
           fifo_cfg.reg_data[0], (unsigned)(fifo_cfg.reg_data[0] & 0x01));

    printf("INT Ena Mask   : 0x%02X%02X%02X%02X (RX=%u TX=%u)\r\n",
           int_cfg.reg_data[3], int_cfg.reg_data[2], int_cfg.reg_data[1], int_cfg.reg_data[0],
           (unsigned)(IS_INT_ST_RX(int_cfg.reg_data) ? 1 : 0),
           (unsigned)(IS_INT_ST_TX(int_cfg.reg_data) ? 1 : 0));

    printf("TX FIFO Mode   : 0x%02X\r\n", txmode_cfg.reg_data[0]);

    /* This config header does not program MAC_RX_802 (0xA008), the RX-side
       counterpart of MAC_TX_CFG - it is left at the chip's power-on default. */
    printf("RX FIFO Mode   : 0x%02X (set by gtXtAppConfigInfo.rx_mode in FW)\r\n", gtXtAppConfigInfo.rx_mode);

    printf("Syncword       : 0x%08lX\r\n", (unsigned long)PREDEFINED_RX_SYNCWORD);
   
    printf("\r\n");
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
    gtXtAppConfigInfo.max_rx_size = RF_BUF_SIZE; //This is not important, for 802.15.4 mode, the actual RX payload length is determined by the PHR in the received frame
    rf_err_mode = 1;
    gu8TxSeqNum = 0;
    TRx_IoConfig();

    if( XTAPP_Strobe() == 0 )   // Check RF chip present
    {
        TRx_SW_Reset();
        if(TRx_Init() == TRx_STATUS_SUCCESS)   // Default initialization
        {
            TRx_RX_Trigger(0,   // In 802.15.4 mode, the actual RX payload length is determined by the PHR in the received frame, so we can set this to 0 or any value within the max limit.
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
    
    dump_buffer(tx_data, data_len, 0, 0);
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
        if(IS_INT_ST_RX(u8Array))
        {   /* RX event */
            xdata_u16 len;
            xdata_u16 u16Rssi;
            XTAPP_ReceiveCheck(&len);
            TRx_GetRSSI_Data(&u16Rssi);
            u16Rssi = (4095 - u16Rssi) / 8;
            dump_buffer(gXtBuffer, len, 1, u16Rssi);
        }
        else if(IS_INT_ST_RXERR(u8Array))
        {   /* RX error event */
            printf("RX Error\r\n");
            TRx_RX_FIFOReset();
            
            #if 1 /* Get Rx Error detail, if it exist */
            {
                TRx_READREG(0xA004, u8Array);
                printf("Valid802154 : %d\r\n", (u8Array[0] >> 0) & 0x01);
                printf("CrcErr      : %d\r\n", (u8Array[0] >> 1) & 0x01);
                printf("FormatErr   : %d\r\n", (u8Array[0] >> 2) & 0x01);
                printf("FormatRej   : %d\r\n", (u8Array[0] >> 3) & 0x01);
            }
            #endif
        }

        
        TRx_RX_Trigger(0,  (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0)); 
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
    /* IRQ handler: process RX received / TX complete events */
    if(RF_NIRQ_VAL() == 0)
    {
        XTAPP_IrqHdlr();
    }
    /* TX trigger process */
    if(DVB_SW5_PIN == 0)
    {
        uint8_t  u8Array[4];
        uint8_t i;
        /* Copy MHR to buffer*/
        for(i=0; i<sizeof(SamplePattern); i++)
        {
            gXtBuffer[i] = SamplePattern[i];
        }

        /* Set Payload to buffer*/
        for(i = sizeof(SamplePattern); i < RF_TX_SIZE; i++)
        {
            gXtBuffer[i] = i;
        }
        
        gXtBuffer[sizeof(SamplePattern)] = gu8TxSeqNum++;   // Payload length, can be set to any value within the max limit in 802.15.4 mode, as the actual payload length is determined by the PHR in the sent frame.
        dump_buffer(gXtBuffer, RF_TX_SIZE, 0, 0);
        XTAPP_SendData(gXtBuffer, RF_TX_SIZE);
        /* wait INT_ST_TX */
        while(RF_NIRQ_VAL());
        TRx_GetIntStatus(u8Array);
        TRx_ClearIntFlag(u8Array);
        UART0_SendStr("Tx done.\r\n");
        /* TX complete event */
        while(DVB_SW5_PIN == 0);   // Wait for button release
        
        TRx_RX_Trigger(0, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
