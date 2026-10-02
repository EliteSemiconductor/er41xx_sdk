/******************************************************************************/
/*
 * @file     RF_App.h
 * @version  V1.0.0
 * @brief    RF application layer for ER4100 transceiver
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RF_APP_H__
#define __RF_APP_H__

/* Includes -----------------------------------------------------------------*/
/* Definition & Macro -------------------------------------------------------*/
/* Source address: this device's own identity */
#define SRC_PANID       0xABCD		//Set source MAC_PANID
#define SRC_ADDR        0x1234		//Set source MAC_ADDRESS
/* Destination address: the target device this packet is sent to */
#define DES_PANID_MATCH         0xABCD		// destination MAC_PANID match this device
#define DES_PANID_MISMATCH      0x5555		// destination MAC_PANID mismatch this device
#define DES_PANID_ALL           0xFFFF		// destination MAC_PANID for all device

#define DES_ADDR_MATCH          0x1234		// destination MAC_ADDRESS match this device
#define DES_ADDR_MISMATCH       0x5678		// destination MAC_ADDRESS mismatch this device
#define DES_ADDR_ALL            0xFFFF		// destination MAC_ADDRESS for all device
/* Typedef ------------------------------------------------------------------*/
typedef struct
{
    uint8_t  reserved       :8;     // Reserved for future use; do not modify
    uint8_t  rx_mode        :8;     // RX mode: RX_TRANSPARENT or RX_802154
    uint16_t max_rx_size;           // Max RX payload size in bytes
                                    //   Standard FIFO mode: 1~128 bytes
                                    //   Long FIFO mode    : 1~2047 bytes
} t_xtapp_config_t;

/* Extend Variables ---------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/* RX Control */
void XTAPP_ReceiveCheck(uint16_t* data_len);
/* TX Control */
void XTAPP_SendData(uint8_t* tx_data, uint8_t data_len);
/* Configuration & Init */
void dump_rf_config(void);
uint8_t XTAPP_Init(void);
/* IRQ Handler */
void XTAPP_IrqHdlr(void);
/* Main handler */
void XTAPP_Scan(void);
#endif /* __RF_APP_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
