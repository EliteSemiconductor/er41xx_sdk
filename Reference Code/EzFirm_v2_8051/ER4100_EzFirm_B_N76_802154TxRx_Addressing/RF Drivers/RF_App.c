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

    UART0_SendStr("===== RF Config form Toolkit=====\r\n");
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
    UART0_SendDec3((uint16_t)((pwr_cfg.reg_data[3] >> 1) & 0x7F));
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

void XTAPP_SetDeviceAddr(uint16_t pan_id, uint16_t addr)
{
    uint8_t  u8Array[4];
		UART0_SendStr("\r\n");
    UART0_SendStr("PAN ID = 0x");
    UART0_SendHex8((uint8_t)(pan_id >> 8));
    UART0_SendHex8((uint8_t)(pan_id & 0xFF));
		UART0_SendStr("\r\n");
    UART0_SendStr("Addr = 0x");
    UART0_SendHex8((uint8_t)(addr >> 8));
    UART0_SendHex8((uint8_t)(addr & 0xFF));
    UART0_SendStr("\r\n");

    //Set MAC_PANID command
    SET_U8_ARRAY(u8Array, 0x00, 0x00, pan_id>>8, pan_id&0xFF);	
    //UART0_SendStr("-> write PAN ID to 0xA070\r\n");
    TRx_WRITEREG(0xA070, u8Array);	
    //Set MAC_ADDR command
    SET_U8_ARRAY(u8Array, 0x00, 0x00, addr>>8, addr&0xFF);	
    //UART0_SendStr("-> write Addr to 0xA074\r\n");
    TRx_WRITEREG(0xA074, u8Array);	
    //Enable MAC_ADDR and MAC_PAN_ID check
    SET_U8_ARRAY(u8Array, 0x00, 0x00, 0x05, 0x00);	
    //UART0_SendStr("-> enable MAC_ADDR / MAC_PAN_ID check at 0xA00C\r\n");
    TRx_WRITEREG(0xA00C, u8Array);	
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
			
				XTAPP_SetDeviceAddr(SRC_PANID, SRC_ADDR);
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

static void XTAPP_PrintTxTarget(uint8_t cmd)
{
    if(cmd == '1')
    {
        UART0_SendStr("Send Match packet: PAN_ID=0x");
        UART0_SendHex8((uint8_t)(DES_PANID_MATCH >> 8));
        UART0_SendHex8((uint8_t)(DES_PANID_MATCH & 0xFF));
        UART0_SendStr(", Addr=0x");
        UART0_SendHex8((uint8_t)(DES_ADDR_MATCH >> 8));
        UART0_SendHex8((uint8_t)(DES_ADDR_MATCH & 0xFF));
        UART0_SendStr("\r\n");
    }
    else if(cmd == '2')
    {
        UART0_SendStr("Broadcast: PAN_ID=0x");
        UART0_SendHex8((uint8_t)(DES_PANID_ALL >> 8));
        UART0_SendHex8((uint8_t)(DES_PANID_ALL & 0xFF));
        UART0_SendStr(", Addr=0x");
        UART0_SendHex8((uint8_t)(DES_ADDR_ALL >> 8));
        UART0_SendHex8((uint8_t)(DES_ADDR_ALL & 0xFF));
        UART0_SendStr("\r\n");
    }
    else if(cmd == '3')
    {
        UART0_SendStr("Send PANID mismatch packet: PAN_ID=0x");
        UART0_SendHex8((uint8_t)(DES_PANID_MISMATCH >> 8));
        UART0_SendHex8((uint8_t)(DES_PANID_MISMATCH & 0xFF));
        UART0_SendStr(", Addr=0x");
        UART0_SendHex8((uint8_t)(DES_ADDR_MATCH >> 8));
        UART0_SendHex8((uint8_t)(DES_ADDR_MATCH & 0xFF));
        UART0_SendStr("\r\n");
    }
    else if(cmd == '4')
    {
        UART0_SendStr("Send ADDR mismatch packet: PAN_ID=0x");
        UART0_SendHex8((uint8_t)(DES_PANID_MATCH >> 8));
        UART0_SendHex8((uint8_t)(DES_PANID_MATCH & 0xFF));
        UART0_SendStr(", Addr=0x");
        UART0_SendHex8((uint8_t)(DES_ADDR_MISMATCH >> 8));
        UART0_SendHex8((uint8_t)(DES_ADDR_MISMATCH & 0xFF));
        UART0_SendStr("\r\n");
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
            UART0_SendStr("\r\nRX Error\r\n");
            TRx_RX_FIFOReset();
            
            #if 1 /* Get Rx Error detail, if it exist */
            {
                TRx_READREG(0xA004, u8Array);
                UART0_SendStrDec3("*Valid802154 : ", (u8Array[0] >> 0) & 0x01, 1);
                UART0_SendStrDec3("*CrcErr      : ", (u8Array[0] >> 1) & 0x01, 1);
                UART0_SendStrDec3("*FormatErr   : ", (u8Array[0] >> 2) & 0x01, 1);
                UART0_SendStrDec3("*FormatRej   : ", (u8Array[0] >> 3) & 0x01, 1);
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
    /* IRQ handler: process RX received */
    if(NIRQ_Value == 0)
    {
        XTAPP_IrqHdlr();
    }
    /* TX trigger process: UART input '1'=P04, '2'=P05, '3'=P06 */
    if(RI)
    {
        uint8_t cmd = SBUF; //Get UART fifo data to trigger different TX patterns
        RI = 0;
        if((cmd == '1') || (cmd == '2') || (cmd == '3') || (cmd == '4'))
        {
            uint8_t  u8Array[4];
            uint8_t i;
            /* Copy MHR to buffer*/
            for(i=0; i<sizeof(SamplePattern); i++)
            {
                gXtBuffer[i] = SamplePattern[i];
            }
            /* Set destination address in buffer*/
            if(cmd == '1')
            {// Correct id
                gXtBuffer[3] = (DES_PANID_MATCH  >>0) &0xFF;    //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[4] = (DES_PANID_MATCH  >>8) &0xFF;    //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[5] = (DES_ADDR_MATCH  >>0) &0xFF;    //Set destination MAC_Address to Tx FIFO
                gXtBuffer[6] = (DES_ADDR_MATCH  >>8) &0xFF;    //Set destination MAC_Address to Tx FIFO
            }
            else if(cmd == '2')
            {// in
                gXtBuffer[3] = (DES_PANID_ALL >>0) &0xFF; //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[4] = (DES_PANID_ALL >>8) &0xFF; //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[5] = (DES_ADDR_ALL  >>0) &0xFF; //Set destination MAC_Address to Tx FIFO
                gXtBuffer[6] = (DES_ADDR_ALL  >>8) &0xFF; //Set destination MAC_Address to Tx FIFO
            }
            else if(cmd == '3')
            {
                gXtBuffer[3] = (DES_PANID_MISMATCH >>0) &0xFF; //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[4] = (DES_PANID_MISMATCH >>8) &0xFF; //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[5] = (DES_ADDR_MATCH >>0) &0xFF;    //Set destination MAC_Address to Tx FIFO
                gXtBuffer[6] = (DES_ADDR_MATCH >>8) &0xFF;    //Set destination MAC_Address to Tx FIFO
            }
            else if(cmd == '4')
            {
                gXtBuffer[3] = (DES_PANID_MATCH >>0) &0xFF; //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[4] = (DES_PANID_MATCH >>8) &0xFF; //Set destination MAC_PANID to Tx FIFO
                gXtBuffer[5] = (DES_ADDR_MISMATCH >>0) &0xFF;    //Set destination MAC_Address to Tx FIFO
                gXtBuffer[6] = (DES_ADDR_MISMATCH >>8) &0xFF;    //Set destination MAC_Address to Tx FIFO
            }

            XTAPP_PrintTxTarget(cmd);

            /* Set Payload to buffer*/
            for(i = sizeof(SamplePattern); i < RF_TX_SIZE; i++)
            {
                gXtBuffer[i] = i;
            }

            gXtBuffer[sizeof(SamplePattern)] = gu8TxSeqNum++;
            //dump_buffer(gXtBuffer, RF_TX_SIZE, 0, 0);
            XTAPP_SendData(gXtBuffer, RF_TX_SIZE);
            /* wait INT_ST_TX */
            while(NIRQ_Value);
            TRx_GetIntStatus(u8Array);
            TRx_ClearIntFlag(u8Array);
            UART0_SendStr("Tx done.\r\n");

            TRx_RX_Trigger(0, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
        }
    }
}
