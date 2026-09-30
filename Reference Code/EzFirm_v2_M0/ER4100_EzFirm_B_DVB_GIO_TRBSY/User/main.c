/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 TransTxRx application on NANO100
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
extern t_xtapp_config_t   xdata gtXtAppConfigInfo;
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
        printf("GIO SEL-TRBSY : init done\r\n");
    else
        printf("GIO SEL-TRBSY : init fail\r\n");

    printf("\r\nGIO TRBSY Demo\r\n");
    printf("Key in:\r\n");
    printf("'0' = Debug mode\r\n");
    printf("'1' = TR_BSY mode\r\n");
    printf("'2' = START RX\r\n");
    printf("'3' = STOP RX\r\n");
    printf("Trigger source: KEY5 -> TX\r\n");

    while (1)
    {
        /* Control GPIO0 and RX by UART input character: '0'/'1' = GPIO0 disable/enable(TR_BSY), '2'/'3' = RX start/stop */
        if (UART0_IsDataReady())
        {
            uint8_t c = Receive_Data_From_UART0();

            switch (c)
            {
                case '0':
                    printf("GPIO0 switch to Debug mode\r\n");
                    // addr=0x4000(GPIO0), gio_sel=0(TR_BSY, unused since mode=Debug),
                    // gio_inv=0(no inverse), mode=7(Debug mode), ds=2(13.5mA, default),
                    // smt=1(Schmitt trigger enable, default), pull_ctrl=2(no pull, default),
                    // dbg_num=0(GPIO_0_DBG_NUM, default)
                    TRx_GPIO_GeneralContrl(0x4000, (TRx_GPIOSel_EnumDef)0, 0, 7, 2, 1, 2, 0);
                    break;
                case '1':
                    printf("GPIO0 switch to GIO mode : sel TR_BSY\r\n");
                    TRx_GPIO0_Sel(1, (TRx_GPIOSel_EnumDef)TR_BSY);
                    break;
                case '2':
                    printf("START RX\r\n");
                    TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                           (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
                    break;
                case '3':
                    printf("STOP RX\r\n");
                    TRx_RxOff();
                    break;
                default:
                    break;
            }
        }
        /* RF Task Process */
        XTAPP_Scan();
    }
}
