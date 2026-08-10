/******************************************************************************/
/*
 * @file     RF_Hal.h
 * @version  V1.0.0
 * @brief    HAL driver to access transceiver via SPI and GPIO
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __RF_HAL_H__
#define __RF_HAL_H__

/* Includes -----------------------------------------------------------------*/
/* Definition & Macro -------------------------------------------------------*/
/* Pin Definitions 
   Note: _PORT defines are placeholders; N76E003 uses single bit-addressable pins */
// RF_NIRQ  P03
#define NIRQ_PORT
#define NIRQ_PIN        P03
#define NIRQ_In         P03_Input_Mode
#define NIRQ_Value      NIRQ_PIN
#define NIRQ_EnaInt     Enable_BIT3_FallEdge_Trig;set_EPI
// RF_SI    P00
#define MOSI_PORT
#define MOSI_PIN        P00
#define Set_MOSI        (P00 = 1)
#define Clr_MOSI        (P00 = 0)
#define MOSI_Out        P00_Quasi_Mode
#define MOSI_In         P00_Input_Mode
// RF_SO    P01
#define MISO_PORT
#define MISO_PIN        P01
#define MISO            MISO_PIN
#define MISO_In         P01_Input_Mode
#define MISO_Value      MISO_PIN
// RF_SCK   P10
#define SCK_PORT
#define SCK_PIN         P10
#define Set_SCK         (P10 = 1)
#define Clr_SCK         (P10 = 0)
#define SCK_Out         P10_Quasi_Mode
#define SCK_In          P10_Input_Mode
// RF_NSS   P15
#define NSS_PORT
#define NSS_PIN         P15
#define Set_NSS         (P15 = 1)
#define Clr_NSS         (P15 = 0)
#define NSS_Out         P15_Quasi_Mode
#define NSS_In          P15_Input_Mode

/* SPI Control Macros */
#define RF_SPI_CS_HI()  Set_NSS     // SPI NSS/CS High
#define RF_SPI_CS_LO()  Clr_NSS     // SPI NSS/CS Low
#define RF_NIRQ_VAL()   NIRQ_Value  // NIRQ Value Read

#define SPI_DUMMY                   0x55
#define RF_SPI_WRITE_BYTE(b)        SpiWriteByte(b)
#define RF_SPI_READ_BYTE()          SpiReadByte()

/* Typedef ------------------------------------------------------------------*/

/* Extend Variables ---------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
void    TRx_IoConfig(void);
uint8_t TRx_Write(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t DataLen);
uint8_t TRx_WriteAddr(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t Addr);
uint8_t TRx_Read(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t DataLen);
uint8_t TRx_ReadAddr(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t Addr);
uint8_t TRx_IsIRQn(void);

#endif /* __RF_HAL_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
