/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 TransTxRx_LongPkt application on NANO100
 *
 * @copyright (C) COPYRIGHT 2024 ESMT
 * Technology Corp. All rights reserved.
*/
/*****************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "Common.h"

/* Definition & Macro -------------------------------------------------------*/

/* Typedef ------------------------------------------------------------------*/

/* Variables ------------------------------------------------------------------*/
const char MainFwBuildTime[] = __DATE__ "," __TIME__;

/* Function prototypes ------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/**
 * @brief  System core clock / pin-mux initialization
 *         (NANO100 needs an explicit clock bring-up step that the 8051
 *          source did not require; pin-mux values are taken from
 *          NANO100/ER41XX_EZ_StdTRx_Transparent, same DVB board)
 * @param  None
 * @retval None
 */
void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();
    /* Enable External XTAL (4~24 MHz), 12MHz on the NANO100 DVB board */
    CLK_EnableXtalRC(CLK_PWRCTL_HXT_EN_Msk);
    CLK_WaitClockReady(CLK_CLKSTATUS_HXT_STB_Msk);
    /* Switch HCLK clock source to HXT, set HCLK frequency 42MHz */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HXT, CLK_HCLK_CLK_DIVIDER(1));
    CLK_SetCoreClock(42000000);
    /* Enable UART1 module clock (log port, through micro USB) */
    CLK_EnableModuleClock(UART1_MODULE);
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_UART_CLK_DIVIDER(1));
    /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* I/O Multi-function: UART1 (PB4/PB5), HXT/ICE (PF0~PF3) */
    SYS->PB_L_MFP = SYS_PB_L_MFP_PB5_MFP_UART1_TX | SYS_PB_L_MFP_PB4_MFP_UART1_RX;
    SYS->PF_L_MFP = SYS_PF_L_MFP_PF3_MFP_HXT_IN | SYS_PF_L_MFP_PF2_MFP_HXT_OUT |
                     SYS_PF_L_MFP_PF1_MFP_ICE_CLK | SYS_PF_L_MFP_PF0_MFP_ICE_DAT;
    /* Lock protected registers */
    SYS_LockReg();
}
/**
 * @brief  Initialize GPIO pins
 * @param  None
 * @retval None
 */
void GPIO_Init(void)
{
    // DVB_SW1
    GPIO_SetMode(DVB_SW1_PORT, DVB_SW1_BIT, GPIO_PMD_INPUT);
    GPIO_ENABLE_PULL_UP(DVB_SW1_PORT, DVB_SW1_BIT);
    // DVB_SW2
    GPIO_SetMode(DVB_SW2_PORT, DVB_SW2_BIT, GPIO_PMD_INPUT);
    GPIO_ENABLE_PULL_UP(DVB_SW2_PORT, DVB_SW2_BIT);
    // DVB_SW3
    GPIO_SetMode(DVB_SW3_PORT, DVB_SW3_BIT, GPIO_PMD_INPUT);
    GPIO_ENABLE_PULL_UP(DVB_SW3_PORT, DVB_SW3_BIT);
    // DVB_SW4
    GPIO_SetMode(DVB_SW4_PORT, DVB_SW4_BIT, GPIO_PMD_INPUT);
    GPIO_ENABLE_PULL_UP(DVB_SW4_PORT, DVB_SW4_BIT);
    // DVB_SW5 => TX trigger button
    GPIO_SetMode(DVB_SW5_PORT, DVB_SW5_BIT, GPIO_PMD_INPUT);
    GPIO_ENABLE_PULL_UP(DVB_SW5_PORT, DVB_SW5_BIT);
}
/**
 * @brief  Main entry
 * @param  None
 * @retval None
 */
int main(void)
{
    SYS_Init();
    InitialUART0_Timer1(UART_BAUD_115200);
    GPIO_Init();

    printf("\r\nMainFw Build At:%s\r\n", MainFwBuildTime);

    if(XTAPP_Init()==0)
        printf("TransTxRx_LongPkt(512byte) : init done\r\n");
    else
        printf("TransTxRx_LongPkt(512byte) : init fail\r\n");
    while (1)
    {
        XTAPP_Scan();
    }
}
