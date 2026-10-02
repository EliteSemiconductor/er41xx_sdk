/******************************************************************************/
/*
 * @file     RF_Hal.c
 * @version  V1.0.0
 * @brief    HAL driver to access transceiver via hardware SPI0 and GPIO (NANO100)
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
 * @brief  Write one byte via hardware SPI0 (MSB first, Mode 0)
 * @param  spi_port: SPI peripheral base (SPI0)
 *         byte:     data to write
 * @retval None
 */
void SpiWriteByte(SPI_T *spi_port, uint8_t byte)
{
    SPI_WRITE_TX0(spi_port, byte);
    SPI_TRIGGER(spi_port);
    while (SPI_IS_BUSY(spi_port));
}
/**
 * @brief  Read one byte via hardware SPI0 (MSB first, Mode 0)
 * @param  spi_port: SPI peripheral base (SPI0)
 * @retval Received byte
 */
uint8_t SpiReadByte(SPI_T *spi_port)
{
    uint8_t byte;
    SPI_WRITE_TX0(spi_port, 0xFF);
    SPI_TRIGGER(spi_port);
    while (SPI_IS_BUSY(spi_port));
    byte = (uint8_t)SPI_READ_RX0(spi_port);
    return byte;
}
/**
 * @brief  Initialize SPI0 and GPIO pins for RF transceiver
 * @param  None
 * @retval None
 */
void TRx_IoConfig(void)
{
    /* ER4100 shutdown/interrupt pins */
    GPIO_SetMode(RF_SDN_PORT, RF_SDN_BIT, GPIO_PMD_OUTPUT);
    RF_SDN_PIN = 1;             // 1 to leave shutdown mode, 0 to enter shutdown mode
    GPIO_SetMode(RF_NIRQ_PORT, RF_NIRQ_BIT, GPIO_PMD_INPUT);

    /* SPI0 hardware bring-up: module clock, pin-mux (PC0~PC3), master mode */
    SYS_UnlockReg();
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0_S_HCLK, 0);
    SYS->PC_L_MFP = SYS_PC_L_MFP_PC3_MFP_SPI0_MOSI0 | SYS_PC_L_MFP_PC2_MFP_SPI0_MISO0 |
                     SYS_PC_L_MFP_PC1_MFP_SPI0_SCLK | SYS_PC_L_MFP_PC0_MFP_SPI0_SS0;
    SYS_LockReg();

    /* Master, MSB first, 8-bit transaction, SPI Mode-0, clock 4MHz */
    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 4000000);

    Delay_10us(1);  // Wait for stable
}
/**
 * @brief  Write OpCode and optional data to transceiver via SPI
 * @param  TriggerCMD: OpCode
 *         pBuffer:    Data buffer
 *         DataLen:    Number of bytes to write (0 = OpCode only)
 * @retval 0  Always (reserved for future error code)
 */
uint8_t TRx_Write(uint8_t TriggerCMD, const uint8_t *pBuffer, uint16_t DataLen)
{
    const uint8_t *p = pBuffer;
    //pBuffer[0] is bit7:0 of uint32_t
    //pBuffer[1] is bit15:8 of uint32_t
    //pBuffer[2] is bit23:16 of uint32_t
    //pBuffer[3] is bit31:24 of uint32_t
    xdata_u16 i;

    RF_SPI_CS_LO();
    RF_SPI_WRITE_BYTE(TriggerCMD);
    if (DataLen)
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
uint8_t TRx_WriteAddr(uint8_t TriggerCMD, const uint8_t* pBuffer, uint16_t Addr)
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

    if (DataLen)
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
    if (RF_NIRQ_VAL())
        return 0;
    else
        return 1;
}
