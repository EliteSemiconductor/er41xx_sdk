/******************************************************************************/
/*
 * @file     main.c
 * @version  V1.0.0
 * @brief    Main entry point for ER4100 TransTxRx application on N76E003
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
extern t_xtapp_config_t   xdata gtXtAppConfigInfo;
/* Function prototypes ------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/
/**
 * @brief  Initialize GPIO pins
 * @param  None
 * @retval None
 */
void GPIO_Init(void)
{
    P05_PushPull_Mode;              // P0.5 => TX trigger button
}
/**
 * @brief  Main entry
 * @param  None
 * @retval None
 */
void main(void)
{
    InitialUART0_Timer1(UART_BAUD_115200);
    GPIO_Init();
    if(XTAPP_Init()==0)
        UART0_SendStr("GIO SEL-TRBSY : init done\r\n");
    else
        UART0_SendStr("GIO SEL-TRBSY : init fail\r\n");

    UART0_SendStr("\r\nGIO TRBSY Demo\r\n");
    UART0_SendStr("Key in:\r\n");
    UART0_SendStr("'0' = Debug mode\r\n");
    UART0_SendStr("'1' = TR_BSY mode\r\n");
    UART0_SendStr("'2' = START RX\r\n");
    UART0_SendStr("'3' = STOP RX\r\n");
    UART0_SendStr("Trigger source: P05 -> TX\r\n");

    while (1)
    {
        /* Control GPIO0 and RX by UART input character: '0'/'1' = GPIO0 disable/enable(TR_BSY), '2'/'3' = RX start/stop */
        if (UART0_IsDataReady())
        {
            uint8_t c = Receive_Data_From_UART0();

            switch (c)
            {
                case '0':
                    UART0_SendStr("GPIO0 switch to Debug mode\r\n");
                    // addr=0x4000(GP10IO0), gio_sel=0(TR_BSY, unused since mode=Debug),
                    // gio_inv=0(no inverse), mode=7(Debug mode), ds=2(13.5mA, default),
                    // smt=1(Schmitt trigger enable, default), pull_ctrl=2(no pull, default),
                    // dbg_num=0(GPIO_0_DBG_NUM, default)
                    TRx_GPIO_GeneralContrl(0x4000, 0, 0, 7, 2, 1, 2, 0);
                    break;
                case '1':
                    UART0_SendStr("GPIO0 switch to GIO mode : sel TR_BSY\r\n");
                    TRx_GPIO0_Sel(1, TR_BSY);
                    break;
                case '2':
                    UART0_SendStr("START RX\r\n");
                    TRx_RX_Trigger(gtXtAppConfigInfo.max_rx_size,
                           (gtXtAppConfigInfo.rx_mode == RX_802154 ? 1 : 0));   // Restart RX
                    break;
                case '3':
                    UART0_SendStr("STOP RX\r\n");
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
