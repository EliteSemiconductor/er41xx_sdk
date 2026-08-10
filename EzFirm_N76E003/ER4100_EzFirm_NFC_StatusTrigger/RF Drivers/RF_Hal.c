/******************************************************************************/
/*
 * @file     RF_Hal.c
 * @version  V1.0.0
 * @brief    HAL driver to access transceiver via SPI and GPIO
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "Common.h"

/* Definition & Macro -------------------------------------------------------*/

/* Typedef ------------------------------------------------------------------*/

/* Variables ----------------------------------------------------------------*/

/* Function prototypes ------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/**
 * @brief  Write one byte via software SPI (bit-bang, MSB first, Mode 0)
 * @param  byte: data to write
 * @retval None
 */
void SpiWriteByte(uint8_t byte)
{
    uint8_t i = 8;
    while (i--) {
        MOSI_PIN = (byte & 0x80) ? 1 : 0;
        byte <<= 1;
        Set_SCK;
        NOP();
        Clr_SCK;
    }
}
/**
 * @brief  Read one byte via software SPI (bit-bang, MSB first, Mode 0)
 * @param  None
 * @retval Received byte
 */
uint8_t SpiReadByte(void)
{
    uint8_t byte = 0;
    uint8_t i = 8;
    while (i--) {
        Set_SCK;
        NOP();
        byte <<=1;
        if (MISO) byte |= 0x01;
        Clr_SCK;
    }
    return byte;
}
/**
 * @brief  Initialize SPI and GPIO pins for RF transceiver
 * @param  None
 * @retval None
 */
void TRx_IoConfig(void)
{
    // SW_SPI
    /* Set pin directions */
    NSS_Out;        // NSS  as output
    SCK_Out;        // SCK  as output
    MISO_In;        // MISO as input
    MOSI_Out;       // MOSI as output
    NIRQ_In;        // NIRQ as input
    NIRQ_EnaInt;    // Enable NIRQ falling-edge interrupt
    /* Set default pin states */
    Set_NSS;        // NSS  high (deselect)
    Clr_SCK;        // SCK  low  (idle)
    Set_MOSI;       // MOSI high (idle)

    Delay_10us(1);  // Wait for stable
}
/**
 * @brief  Write OpCode and optional data to transceiver via SPI
 * @param  TriggerCMD: OpCode
 *         pBuffer:    Data buffer
 *         DataLen:    Number of bytes to write (0 = OpCode only)
 * @retval 0  Always (reserved for future error code)
 */
uint8_t TRx_Write(uint8_t TriggerCMD, uint8_t *pBuffer, uint16_t DataLen)
{
    uint8_t *p = pBuffer;
    //pBuffer[0] is bit7:0 of uint32_t
    //pBuffer[1] is bit15:8 of uint32_t
    //pBuffer[2] is bit23:16 of uint32_t
    //pBuffer[3] is bit31:24 of uint32_t
    xdata_u16 i;

    RF_SPI_CS_LO();
    RF_SPI_WRITE_BYTE(TriggerCMD);
    if(DataLen)
    {
        if (TriggerCMD == COMM_TXFIFO)
        {
            for (i = 0; i < DataLen; i++)
            {
                RF_SPI_WRITE_BYTE(*p++);
            }
        }
        else
        {
            RF_SPI_WRITE_BYTE(p[0]);
            if (DataLen != 1)
            {
                RF_SPI_WRITE_BYTE(p[1]);
                RF_SPI_WRITE_BYTE(p[2]);
                RF_SPI_WRITE_BYTE(p[3]);
            }
        }
    }

    RF_SPI_CS_HI();
    return 0;
}
/**
 * @brief  Write register address and data to transceiver via SPI
 * @param  TriggerCMD: Write-Reg OpCode
 *         pBuffer:    4-byte data buffer
 *         Addr:       16-bit register address
 * @retval 0  Always (reserved for future error code)
 */
uint8_t TRx_WriteAddr(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t Addr)
{
    //pBuffer[0] is bit7:0 of uint32_t
    //pBuffer[1] is bit15:8 of uint32_t
    //pBuffer[2] is bit23:16 of uint32_t
    //pBuffer[3] is bit31:24 of uint32_t
    RF_SPI_CS_LO();
    RF_SPI_WRITE_BYTE(TriggerCMD);
    RF_SPI_WRITE_BYTE((uint8_t)(Addr >> 8));
    RF_SPI_WRITE_BYTE((uint8_t)(Addr & 0xFF));
    RF_SPI_WRITE_BYTE(pBuffer[0]);
    RF_SPI_WRITE_BYTE(pBuffer[1]);
    RF_SPI_WRITE_BYTE(pBuffer[2]);
    RF_SPI_WRITE_BYTE(pBuffer[3]);

    RF_SPI_CS_HI();
    return 0;
}
/**
 * @brief  Read OpCode response data from transceiver via SPI
 * @param  TriggerCMD: OpCode
 *         pBuffer:    Buffer to store received data
 *         DataLen:    Number of bytes to read
 * @retval 0  Always (reserved for future error code)
 */
uint8_t TRx_Read(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t DataLen)
{
    uint8_t *p = pBuffer;
    //pBuffer[0] is bit7:0 of uint32_t
    //pBuffer[1] is bit15:8 of uint32_t
    //pBuffer[2] is bit23:16 of uint32_t
    //pBuffer[3] is bit31:24 of uint32_t
    xdata_u16 i;

    RF_SPI_CS_LO();
    RF_SPI_WRITE_BYTE(TriggerCMD);
    RF_SPI_WRITE_BYTE(SPI_DUMMY);

    if(DataLen)
    {
        if (TriggerCMD == COMM_RXFIFO)
        {
            for (i = 0; i < DataLen; i++)
                p[i] = RF_SPI_READ_BYTE();
        }
        else
        {
            p[0] = RF_SPI_READ_BYTE();
            p[1] = RF_SPI_READ_BYTE();
            p[2] = RF_SPI_READ_BYTE();
            p[3] = RF_SPI_READ_BYTE();
        }
    }
    RF_SPI_CS_HI();
    return 0;
}
/**
 * @brief  Read register data from transceiver via SPI
 * @param  TriggerCMD: Read-Reg OpCode
 *         pBuffer:    4-byte buffer to store result
 *         Addr:       16-bit register address
 * @retval 0  Always (reserved for future error code)
 */
uint8_t TRx_ReadAddr(uint8_t TriggerCMD, uint8_t* pBuffer, uint16_t Addr)
{
    //pBuffer[0] is bit7:0 of uint32_t
    //pBuffer[1] is bit15:8 of uint32_t
    //pBuffer[2] is bit23:16 of uint32_t
    //pBuffer[3] is bit31:24 of uint32_t
    RF_SPI_CS_LO();
    RF_SPI_WRITE_BYTE(TriggerCMD);
    RF_SPI_WRITE_BYTE((uint8_t)(Addr >> 8));
    RF_SPI_WRITE_BYTE((uint8_t)(Addr & 0xFF));
    RF_SPI_WRITE_BYTE(SPI_DUMMY);
    pBuffer[0] = RF_SPI_READ_BYTE();
    pBuffer[1] = RF_SPI_READ_BYTE();
    pBuffer[2] = RF_SPI_READ_BYTE();
    pBuffer[3] = RF_SPI_READ_BYTE();

    RF_SPI_CS_HI();
    return 0;
}
/**
 * @brief  Check if an IRQ event is pending from the transceiver
 * @param  None
 * @retval 1: IRQ asserted (NIRQ low), 0: no IRQ
 */
uint8_t TRx_IsIRQn(void)
{
    if( RF_NIRQ_VAL() )
        return 0;
    else
        return 1;
}
