/******************************************************************************/
/*
 * @file     RF_NFC_APP.h
 * @version  V1.0.0
 * @brief    RF application layer for ER4100 transceiver
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RF_NFC_APP_H__
#define __RF_NFC_APP_H__

/* Includes -----------------------------------------------------------------*/
/* Definition & Macro -------------------------------------------------------*/
/* NFC block addresses in memory */
#define NFC_BLOCK0_ADDRESS              0x9000
#define NFC_BLOCK255_ADDRESS            0x93FC
#define NFC_BLOCK_COUNT                 256
#define NFC_HEAD_BLOCK_START_ADDRESS    NFC_BLOCK0_ADDRESS
#define NFC_HEAD_BLOCK_END_ADDRESS      0x900C
#define NFC_DPE_BLOCK_ADDRESS           NFC_BLOCK255_ADDRESS
#define NFC_DPE_CTRL_BYTE_IDX           3   /* Byte3 = DPE Control (MCU writes) */
#define NFC_DPE_STATUS_BYTE_IDX         2   /* Byte2 = DPE Status  (Host writes) */
/* NFC data block range for UART cmd '1'/'2' */
#define NFC_DATA_BLOCK_START_ADDRESS    0x9010
#define NFC_DATA_BLOCK_END_ADDRESS      0x93DC
#define NFC_DATA_BLOCK_START_NUM        4
#define NFC_DATA_BLOCK_END_NUM          247
#define NFC_DATA_BLOCK_BYTE_COUNT       976//(NFC_DATA_BLOCK_END_NUM - NFC_DATA_BLOCK_START_NUM + 1) * 4  /* 64 bytes */

/* DPE Control reg byte[3] � irq_src[2:0]: index written to NFC_Select_IRQ_Src() */
#define NFC_IGNORED                     (uint8_t)(0)   /* no interrupt          */
#define NFC_PWRGOOD                     (uint8_t)(1)   /* RF field present      */
#define NFC_RX_CMD                      (uint8_t)(2)   /* reader sent a command */
#define NFC_TX_REPLY                    (uint8_t)(3)   /* reply transmitted     */
#define NFC_USER_CFG4                   (uint8_t)(4)
#define NFC_USER_CFG5                   (uint8_t)(5)
#define NFC_USER_CFG6                   (uint8_t)(6)
#define NFC_USER_CFG7                   (uint8_t)(7)
/* DPE Control reg byte[3] � bit masks */
#define NFC_IRQ_SRC_MASK                (uint8_t)(0x07)  /* bits[2:0] irq_src   */
#define NFC_CLEAR_INT_MASK              (uint8_t)(0x08)  /* bit[3]  clear/mask  */
#define NFC_DPE_RST_MASK                (uint8_t)(0x10)  /* bit[4]  DPE reset   */

/* DPE Status reg byte[2] � event flags set by DPE, cleared by host/reader */
#define NFC_IGNORED_MASK                (uint8_t)(1 << 0)
#define NFC_PWRGOOD_MASK                (uint8_t)(1 << 1)
#define NFC_RX_CMD_MASK                 (uint8_t)(1 << 2)
#define NFC_TX_REPLY_MASK               (uint8_t)(1 << 3)
#define NFC_USER_CFG4_MASK              (uint8_t)(1 << 4)
#define NFC_USER_CFG5_MASK              (uint8_t)(1 << 5)
#define NFC_USER_CFG6_MASK              (uint8_t)(1 << 6)
#define NFC_USER_CFG7_MASK              (uint8_t)(1 << 7)
/* Typedef ------------------------------------------------------------------*/
typedef struct
{
    uint8_t  reserved;              // Reserved for future use; do not modify
    uint8_t  rx_mode;               // RX mode: RX_TRANSPARENT or RX_802154
    uint16_t max_rx_size;           // Max RX payload size in bytes
                                    //   Standard FIFO mode: 1~128 bytes
                                    //   Long FIFO mode    : 1~2047 bytes
} t_xtapp_config_t;

/* Extend Variables ---------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/* Configuration & Init */
uint8_t XTAPP_Init(void);
/* NFC Control */
void XTAPP_ReadWriteTest_Phone_Side(void);
void XTAPP_ReadWriteTest_MCU_Side(void);
#endif /* __RF_NFC_APP_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
