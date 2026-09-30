/******************************************************************************/
/*
 * @file     RF_Hal.h
 * @version  V1.0.0
 * @brief    HAL driver to access transceiver via hardware SPI0 and GPIO (NANO100)
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
   Reference: NANO100/ER41XX_EZ_StdTRx_Transparent/ER41_HAL.h (same DVB board) */
// ER4100 shutdown pin: PB10
#define RF_SDN_PORT     PB
#define RF_SDN_BIT      BIT10
#define RF_SDN_PIN      PB10
// ER4100 interrupt pin: PB11
#define RF_NIRQ_PORT    PB
#define RF_NIRQ_BIT     BIT11
#define RF_NIRQ_PIN     PB11
// EVB trigger button (SW5): PB15
#define DVB_SW5_PORT    PB
#define DVB_SW5_BIT     BIT15
#define DVB_SW5_PIN     PB15

/* SPI Control Macros (SPI0 hardware peripheral, pins PC0~PC3) */
#define RF_SPI_CS_HI()              SPI_SET_SS0_HIGH(SPI0)   // SPI NSS/CS High
#define RF_SPI_CS_LO()              SPI_SET_SS0_LOW(SPI0)    // SPI NSS/CS Low
#define RF_NIRQ_VAL()               RF_NIRQ_PIN               // NIRQ Value Read

#define SPI_DUMMY                   0x55
#define RF_SPI_WRITE_BYTE(b)        SpiWriteByte(SPI0, b)
#define RF_SPI_READ_BYTE()          SpiReadByte(SPI0)

/* Typedef ------------------------------------------------------------------*/

/* Extend Variables ---------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
void    SpiWriteByte(SPI_T *spi_port, uint8_t byte);
uint8_t SpiReadByte(SPI_T *spi_port);

void    TRx_IoConfig(void);
uint8_t TRx_Write(uint8_t TriggerCMD, const uint8_t* pBuffer, uint16_t DataLen);
uint8_t TRx_WriteAddr(uint8_t TriggerCMD, const uint8_t* pBuffer, uint16_t Addr);
uint8_t TRx_Read(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t DataLen);
uint8_t TRx_ReadAddr(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t Addr);
uint8_t TRx_IsIRQn(void);

#endif /* __RF_HAL_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
