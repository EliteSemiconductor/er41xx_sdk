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

#define CCA_THRESHOLD   80  //unit: dBm
/* Typedef ------------------------------------------------------------------*/

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
            uint16_t u16data;
            u16data = 0xfff - 8*CCA_THRESHOLD;
            TRx_CCA_Config(u16data);
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
void XTAPP_SendDataCCA(uint8_t* tx_data, uint8_t data_len)
{
    xdata_u8 len = (data_len > 128) ? 128 : data_len;

    TRx_RxOff();    //If RX is enabled, RX must be disabled now.
    TRx_TX_FIFO(len, tx_data);
    TRx_TX_Trigger(len, 1);  //2nd parameter is CCA_enable
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
            UART0_SendStr("RX Error\r\n");
            TRx_RX_FIFOReset();
        }

        
        TRx_RX_Trigger(0, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0)); 
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
    /* CCA TX trigger process */
    if(P05 == 0)
    {
        uint8_t  u8Array[4];
        uint8_t i;
        UART0_SendStr("CCA Tx...\r\n");
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
        XTAPP_SendDataCCA(gXtBuffer, RF_TX_SIZE);
        /* wait INT_ST_TX */
        while(NIRQ_Value);  
        TRx_GetIntStatus(u8Array);
        TRx_ClearIntFlag(u8Array);
        if(IS_INT_ST_TX(u8Array)) 
        {
            dump_buffer(gXtBuffer, RF_TX_SIZE, 0, 0);
            UART0_SendStr("CCA Tx done.\r\n");
        }
        else //IS_INT_ST_TXERR ==> detect too much environment noise by CCA machanism														//Tx fail because enviroment noise bigger than threshold
        {
            UART0_SendStr("CCA Tx fail.\r\n");
        }
        /* TX complete event */
        while(P05 == 0);   // Wait for button release
       
        TRx_RX_Trigger(0, (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
    }
}
