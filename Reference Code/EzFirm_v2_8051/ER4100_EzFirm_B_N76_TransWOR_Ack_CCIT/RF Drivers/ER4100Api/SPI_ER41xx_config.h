/* **********************************************************************
 * 
 * @8bits setting  
 * @file     SPI_ER41xx_config.h
 * @version  V4.04 for other crystal
 * @brief    ER41xx series SPI driver
 * config file
 *
 * @copyright (C) COPYRIGHT 2020 ESMT
 * Technology Corp. All rights reserved.
 * ********************************************************************** */
/* Comments ------------------------------------------------------------------*/
// Crystal              : 40MHz
// CAP IO               : 38
/* RF Properties */
// Base frequency       : 915MHz
// Data Rate            : 100000bps
// Deviation            : 50000Hz
// Modulation           : GFSK
// TX Power - PA Type   : 23dBm
// TX Power - Level     : 127
/* Packet Format */
// TX Preamble          : 0xAA, 4byte(s)
// Tx Syncword          : 0x5A0FBE66
// Rx Syncword          : 0x5A0FBE66
// Syncword bit error   : 1bit(s)
/* Interrupt */
// INT_ST_RX
// INT_ST_TX
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_ER41XX_CONFIG_H__
#define __SPI_ER41XX_CONFIG_H__
/* Includes ------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/
/* Predefined Parameter                                                      */
/* --------------------------------------------------------------------------*/
#define CRYSTAL_HZ          40000000
#define PREDEFINED_RX_SZ                128
#define PREDEFINED_RX_SYNCWORD          0x5A0FBE66
#define GLB_DATA_RATE {0xA0,0x86,0x01,0x00}  //0x000186A0 = 100000
#define GLB_DEVIATION {0x50,0xC3,0x00,0x00}  //0x0000C350 = 50000

/* --------------------------------------------------------------------------*/
/* Configuration Register Data                                               */
/* --------------------------------------------------------------------------*/
//====== Initial_Reg_Array ======
#define INIT_SW_RST_REG_1ST  0x0010, {0xFF, 0xFF, 0xFF, 0xFF}
#define INIT_SW_RST_REG_2ND  0x0010, {0x00, 0x00, 0x00, 0x00}
#define INIT_CLK_GATING_EN_REG  0x0000, {0x07, 0x00, 0x00, 0x00}
#define INIT_MOD_RST_REG  0x0014, {0x01, 0x00, 0x00, 0x00}
#define INIT_BIAS_CONFIG1  0x1100, {0x21, 0x38, 0x00, 0x00}
#define INIT_IRC_ADPLL_CONFIG_1  0x1280, {0x90, 0x78, 0x00, 0x00}
#define INIT_IRC_ADPLL_CONFIG_2  0x1284, {0x15, 0x00, 0x20, 0x00}
#define INIT_FREQSYNTH_CONFIG_1  0x1380, {0x85, 0xA7, 0x16, 0x00}
#define INIT_FREQSYNTH_CONFIG_2  0x1384, {0xA8, 0x02, 0xCC, 0x3F}
#define INIT_FREQSYNTH_CONFIG_5  0x1390, {0x08, 0x07, 0x00, 0x00}
#define INIT_XO_CONFIG_1  0x1580, {0x48, 0x85, 0xC2, 0x00}
#define INIT_RX_CONFIG_5  0x1620, {0xE8, 0x05, 0x14, 0x44}
#define INIT_RX_CONFIG_6_1ST  0x1624, {0x35, 0x01, 0x92, 0x02}
#define INIT_NFC_CONFIG  0x1700, {0x41, 0x00, 0x00, 0x02}
#define INIT_GPIO_1_CFG_REG  0x4004, {0x00, 0x00, 0x06, 0x00}
#define INIT_GPIO_DBG_CFG_REG  0x400C, {0x00, 0x00, 0x06, 0x00}
#define INIT_PMU_CONFIG_3  0x1A14, {0x00, 0x00, 0x00, 0x00}
#define INIT_BBP_RXGFSK_CNTL  0xB0A8, {0x0E, 0x37, 0x00, 0x6F}
#define INIT_BBP_AGC18  0xB19C, {0x03, 0x5F, 0x01, 0x00}
#define INIT_BBP_AGC45  0xB208, {0x44, 0x85, 0x06, 0x06}
#define INIT_BBP_AGC1  0xB158, {0x00, 0x12, 0x00, 0x00}
#define INIT_BBP_AGC2  0xB15C, {0x22, 0x22, 0x22, 0x33}
#define INIT_BBP_AGC3  0xB160, {0x33, 0x33, 0x33, 0x33}
#define INIT_BBP_AGC4  0xB164, {0x33, 0x33, 0x33, 0x33}
#define INIT_BBP_AGC5  0xB168, {0x33, 0x33, 0x33, 0x33}
#define INIT_BBP_AGC6  0xB16C, {0x00, 0x00, 0x00, 0x00}
#define INIT_BBP_AGC7  0xB170, {0x20, 0x22, 0x22, 0x33}
#define INIT_BBP_AGC8  0xB174, {0x33, 0x33, 0x33, 0x33}
#define INIT_BBP_AGC9  0xB178, {0x33, 0x33, 0x33, 0x33}
#define INIT_BBP_AGC10  0xB17C, {0x54, 0x76, 0x98, 0x54}
#define INIT_BBP_AGC11  0xB180, {0x06, 0x21, 0x43, 0x21}
#define INIT_BBP_AGC12  0xB184, {0x43, 0x05, 0x21, 0x43}
#define INIT_BBP_AGC13  0xB188, {0x65, 0x87, 0xA9, 0xCB}
#define INIT_BBP_AGC14  0xB18C, {0x55, 0x55, 0x55, 0x66}
#define INIT_BBP_AGC15  0xB190, {0x66, 0x66, 0x66, 0x66}
#define INIT_BBP_AGC16  0xB194, {0x66, 0x76, 0x77, 0x77}
#define INIT_BBP_AGC17  0xB198, {0x77, 0x77, 0x77, 0x77}
#define INIT_BBP_AGC26  0xB1BC, {0x1A, 0x00, 0x32, 0x00}
#define INIT_BBP_AGC27  0xB1C0, {0x4A, 0x00, 0x62, 0x00}
#define INIT_BBP_AGC28  0xB1C4, {0x7A, 0x00, 0x92, 0x00}
#define INIT_BBP_AGC29  0xB1C8, {0xA8, 0x00, 0xC6, 0x00}
#define INIT_BBP_AGC30  0xB1CC, {0xDA, 0x00, 0xF0, 0x00}
#define INIT_BBP_AGC31  0xB1D0, {0x04, 0x01, 0x1C, 0x01}
#define INIT_BBP_AGC32  0xB1D4, {0x34, 0x01, 0x4C, 0x01}
#define INIT_BBP_AGC33  0xB1D8, {0x64, 0x01, 0x7A, 0x01}
#define INIT_BBP_AGC34  0xB1DC, {0x92, 0x01, 0xAC, 0x01}
#define INIT_BBP_AGC35  0xB1E0, {0xC4, 0x01, 0xD6, 0x01}
#define INIT_BBP_AGC36  0xB1E4, {0xEA, 0x01, 0x02, 0x02}
#define INIT_BBP_AGC37  0xB1E8, {0x1A, 0x02, 0x36, 0x02}
#define INIT_BBP_AGC38  0xB1EC, {0x48, 0x02, 0x60, 0x02}
#define INIT_BBP_AGC39  0xB1F0, {0x7C, 0x02, 0x94, 0x02}
#define INIT_BBP_AGC40  0xB1F4, {0xAA, 0x02, 0xC4, 0x02}
#define INIT_BBP_AGC41  0xB1F8, {0xDA, 0x02, 0xEC, 0x02}
#define INIT_BBP_AGC48  0xB250, {0x00, 0x00, 0x00, 0x00}
#define INIT_BBP_AGC49  0xB254, {0x10, 0x32, 0x54, 0x76}
#define INIT_BBP_AGC50  0xB258, {0x77, 0x77, 0x77, 0x77}
#define INIT_DCDC_CONFIG_0  0x1200, {0xA0, 0x05, 0x12, 0x8E}
#define INIT_DCDC_CONFIG_1  0x1204, {0x18, 0x00, 0x00, 0x00}
#define INIT_ANCTL_DCDC_REG0  0x1E50, {0xED, 0x24, 0x31, 0x25}
#define INIT_ANCTL_DCDC_REG1  0x1E54, {0x31, 0x24, 0x31, 0x24}
#define INIT_ANCTL_DCDC_REG2  0x1E58, {0x2D, 0x12, 0x2D, 0x12}
#define INIT_ANCTL_DCDC_REG3  0x1E5C, {0x12, 0x01, 0x00, 0x00}
#define INIT_ANCTL_CONFIG_6  0x1E18, {0x00, 0x14, 0x04, 0x04}
#define INIT_ANCTL_CONFIG_0  0x1E00, {0x01, 0x01, 0x01, 0x00}
#define INIT_ANCTL_CONFIG_BBP  0x1E3C, {0x0D, 0x00, 0x00, 0x00}
#define INIT_ANCTL_CONFIG_7_5  0x1E30, {0x3F, 0xFF, 0xFF, 0xFD}
#define INIT_ANCTL_CONFIG_7_6  0x1E34, {0x01, 0x00, 0x00, 0x00}
#define INIT_ANCTL_FS_REG0  0x1E64, {0x00, 0x00, 0x00, 0x00}
#define INIT_BBP_POP_CLIPPING  0xB30C, {0x01, 0x02, 0x00, 0x00}
#define INIT_PA1G_CONFIG_2  0x1488, {0x01, 0x00, 0x00, 0x00}
#define INIT_RX_POWSET  0x1600, {0x00, 0x02, 0x0A, 0x00}
#define INIT_ANCTL_PA1G_REG0  0x1E60, {0x01, 0x00, 0x00, 0x00}
#define INIT_RX_CONFIG_6_2ND  0x1624, {0x35, 0x01, 0xD2, 0x13}

//====== TRx_Config_Array ======
//----- PA Default -----
#define ANCTL_CONFIG_40  0x1F00, {0x01, 0x01, 0x02, 0x03}
#define ANCTL_CONFIG_41  0x1F04, {0x04, 0x05, 0x06, 0x07}
#define ANCTL_CONFIG_42  0x1F08, {0x08, 0x0A, 0x0C, 0x0D}
#define ANCTL_CONFIG_43  0x1F0C, {0x0E, 0x0F, 0x10, 0x12}
#define ANCTL_CONFIG_44  0x1F10, {0x13, 0x15, 0x17, 0x19}
#define ANCTL_CONFIG_45  0x1F14, {0x1B, 0x1D, 0x1E, 0x1F}
#define ANCTL_CONFIG_46  0x1F18, {0x21, 0x23, 0x25, 0x27}
#define ANCTL_CONFIG_47  0x1F1C, {0x29, 0x2B, 0x2D, 0x2F}
#define ANCTL_CONFIG_48  0x1F20, {0x31, 0x33, 0x36, 0x38}
#define ANCTL_CONFIG_49  0x1F24, {0x3A, 0x3C, 0x3E, 0x3F}
#define ANCTL_CONFIG_50  0x1F28, {0x41, 0x42, 0x44, 0x46}
#define ANCTL_CONFIG_51  0x1F2C, {0x49, 0x4D, 0x4E, 0x4F}
#define ANCTL_CONFIG_52  0x1F30, {0x51, 0x55, 0x59, 0x5E}
#define ANCTL_CONFIG_53  0x1F34, {0x5F, 0x62, 0x68, 0x6B}
#define ANCTL_CONFIG_54  0x1F38, {0x71, 0x78, 0x7F, 0x7F}
//----- TX RF Properties -----
#define TPM_DMI_CONFIG_1  0x13AC, {0x28, 0x00, 0x30, 0x30}
#define MAC_TX_DATDLV  0xA030, {0x00, 0x00, 0x14, 0x00}
//----- RX RF Properties -----
#define BBP_CORE52  0xB304, {0x00, 0x00, 0x00, 0x00}
#define BBP_AGC24  0xB1B4, {0xE8, 0xFE, 0x00, 0x00}
#define RX_GAIN_FILTSET  0x1604, {0x7D, 0x3B, 0xC3, 0x1F}
#define BBP_AGC43  0xB200, {0x17, 0x00, 0x24, 0x00}
#define BBP_CH_FILT0  0xB400, {0x2A, 0x00, 0x00, 0x00}
#define BBP_CH_FILT1  0xB404, {0xB0, 0xFF, 0x92, 0xFF}
#define BBP_CH_FILT2  0xB408, {0xC4, 0xFF, 0x77, 0x00}
#define BBP_CH_FILT3  0xB40C, {0x5B, 0x01, 0xB7, 0x01}
#define BBP_CH_FILT4  0xB410, {0xDE, 0x00, 0xEA, 0xFE}
#define BBP_CH_FILT5  0xB414, {0x0E, 0xFD, 0xFD, 0xFC}
#define BBP_CH_FILT6  0xB418, {0x9B, 0xFF, 0xCC, 0x03}
#define BBP_CH_FILT7  0xB41C, {0x8D, 0x06, 0xCA, 0x04}
#define BBP_CH_FILT8  0xB420, {0xF7, 0xFD, 0x9C, 0xF5}
#define BBP_CH_FILT9  0xB424, {0x43, 0xF2, 0xD5, 0xF9}
#define BBP_CH_FILT10  0xB428, {0x76, 0x0D, 0x93, 0x27}
#define BBP_CH_FILT11  0xB42C, {0xED, 0x3D, 0xB7, 0x46}
#define BBP_RXLPF_1M_0  0xB108, {0xFF, 0x7F, 0xFF, 0x3F}
#define BBP_RXLPF_1M_1  0xB10C, {0x08, 0xA3, 0x1C, 0x94}
#define BBP_RXLPF_1M_2  0xB110, {0x94, 0x01, 0x00, 0x00}
#define BBP_AGC18  0xB19C, {0x03, 0x5F, 0x01, 0x00}
#define BBP_AGC21  0xB1A8, {0x70, 0x00, 0x00, 0x00}
#define BBP_RXIQCAL  0xB000, {0x06, 0x09, 0x00, 0x00}
#define CTQBPSD_CONFIG_3  0x1188, {0x02, 0x00, 0x00, 0x00}
//----- Others -----
#define RX_CONFIG_5  0x1620, {0xE8, 0x05, 0x12, 0x44}
//----- Synthesizer -----
#define SYNTH_CAL_CONFIG_0  0x1980, {0xC0, 0xCA, 0x89, 0x36}
#define SYNTH_CAL_CONFIG_1  0x1984, {0xA0, 0x86, 0x01, 0x00}
#define SYNTH_CAL_CONFIG_2  0x1988, {0xA0, 0x86, 0x01, 0x00}
#define SYNTH_CAL_TRIG  0x1990, {0x01, 0x00, 0x00, 0x00}
#define ANCTL_CONFIG_1  0x1E04, {0x60, 0x00, 0x80, 0x00}
//----- RX RF Properties -----
// NOTE: must be written after the Synthesizer trigger (0x1980~0x1990), or the 40MHz-based hardware auto-fill will overwrite it.
// Also remember to move MODEM_CLK_REG's entry in RF_REG_CONFIG_ARRAY (below) to after the Synthesizer trigger registers' entries (0x1980/0x1984/0x1988/0x1990).
#define MODEM_CLK_REG  0x0004, {0x0A, 0x01, 0x01, 0x00}
//----- TX Power -----
#define DCDC_CONFIG_1  0x1204, {0x18, 0x00, 0x00, 0x00}
#define ANCTL_DCDC_REG3  0x1E5C, {0x12, 0x01, 0x00, 0x00}
#define PA1G_CONFIG_0  0x1480, {0xC5, 0x9F, 0x61, 0x00}
#define PA1G_CONFIG_1  0x1484, {0x00, 0xD2, 0x13, 0x00}
#define PA1G_CONFIG_2  0x1488, {0x00, 0x00, 0x00, 0x00}
#define ANCTL_PA1G_REG0  0x1E60, {0x01, 0x00, 0x00, 0x00}
#define ANCTL_CONFIG_7  0x1E1C, {0x00, 0x00, 0x00, 0xFF}
#define ASARADC_CONFIG_11  0x10A8, {0x00, 0x00, 0x00, 0xFF}
#define ANCTL_CONFIG_7_2  0x1E20, {0x27, 0x00, 0x00, 0x13}

//====== TRx_ConfigEx_Array ======
//----- XTAL Config -----
#define XO_CONFIG_1  0x1580, {0x48, 0x26, 0xD3, 0x00}
//----- GPIO -----
#define GPIO_0_CFG_REG  0x4000, {0x00, 0x01, 0x16, 0x00}
#define GPIO_1_CFG_REG  0x4004, {0x00, 0x00, 0x16, 0x00}
//----- PCR Management -----
#define PMU_CTL_REG  0x0020, {0x04, 0x00, 0x00, 0x00}
#define PMU_TIM_REG  0x0028, {0x00, 0x00, 0x00, 0x00}
#define PMU_CTL  0x1A00, {0xFF, 0xFF, 0xFF, 0xFF}
#define PMU_CONFIG_0  0x1A08, {0xFF, 0xFF, 0xFF, 0xFF}
//----- Interrupt -----
#define INT_EN_REG  0xF004, {0x05, 0x00, 0x00, 0x00}
//----- TX Preamble -----
#define MAC_TX_CFG2  0xA02C, {0x03, 0xAA, 0x00, 0x00}
//----- TX FIFO Mode -----
#define MAC_TX_CFG  0xA028, {0x03, 0x00, 0x00, 0x00}
//----- TX Syncword -----
#define MAC_TX_CFG3  0xA034, {0x03, 0x00, 0x00, 0x00}
#define MAC_TX_CFG4  0xA038, {0x66, 0xBE, 0x0F, 0x5A}
//----- RX Syncword -----
#define BBP_CORR_CODE_BT_BLE  0xB100, {0x66, 0xBE, 0x0F, 0x5A}
#define BBP_CORR_ERR_CNT  0xB104, {0x01, 0xCC, 0x07, 0x00}
//----- FIFO Data Order -----
#define FIFO_FEA_REG  0xA0A8, {0x00, 0x00, 0x00, 0x00}

/* --------------------------------------------------------------------------*/
/* Configuration Register Array                                              */
/* --------------------------------------------------------------------------*/
#define END_REG_INT 0xFFFF
#define END_REG_CHAR 0xFF
#define END_REG_DATA END_REG_INT, {END_REG_CHAR, END_REG_CHAR, END_REG_CHAR, END_REG_CHAR}

#define RF_REG_INIT_ARRAY { \
          INIT_SW_RST_REG_1ST, \
          INIT_SW_RST_REG_2ND, \
          INIT_CLK_GATING_EN_REG, \
          INIT_MOD_RST_REG, \
          INIT_BIAS_CONFIG1, \
          INIT_IRC_ADPLL_CONFIG_1, \
          INIT_IRC_ADPLL_CONFIG_2, \
          INIT_FREQSYNTH_CONFIG_1, \
          INIT_FREQSYNTH_CONFIG_2, \
          INIT_FREQSYNTH_CONFIG_5, \
          INIT_XO_CONFIG_1, \
          INIT_RX_CONFIG_5, \
          INIT_RX_CONFIG_6_1ST, \
          INIT_NFC_CONFIG, \
          INIT_GPIO_1_CFG_REG, \
          INIT_GPIO_DBG_CFG_REG, \
          INIT_PMU_CONFIG_3, \
          INIT_BBP_RXGFSK_CNTL, \
          INIT_BBP_AGC18, \
          INIT_BBP_AGC45, \
          INIT_BBP_AGC1, \
          INIT_BBP_AGC2, \
          INIT_BBP_AGC3, \
          INIT_BBP_AGC4, \
          INIT_BBP_AGC5, \
          INIT_BBP_AGC6, \
          INIT_BBP_AGC7, \
          INIT_BBP_AGC8, \
          INIT_BBP_AGC9, \
          INIT_BBP_AGC10, \
          INIT_BBP_AGC11, \
          INIT_BBP_AGC12, \
          INIT_BBP_AGC13, \
          INIT_BBP_AGC14, \
          INIT_BBP_AGC15, \
          INIT_BBP_AGC16, \
          INIT_BBP_AGC17, \
          INIT_BBP_AGC26, \
          INIT_BBP_AGC27, \
          INIT_BBP_AGC28, \
          INIT_BBP_AGC29, \
          INIT_BBP_AGC30, \
          INIT_BBP_AGC31, \
          INIT_BBP_AGC32, \
          INIT_BBP_AGC33, \
          INIT_BBP_AGC34, \
          INIT_BBP_AGC35, \
          INIT_BBP_AGC36, \
          INIT_BBP_AGC37, \
          INIT_BBP_AGC38, \
          INIT_BBP_AGC39, \
          INIT_BBP_AGC40, \
          INIT_BBP_AGC41, \
          INIT_BBP_AGC48, \
          INIT_BBP_AGC49, \
          INIT_BBP_AGC50, \
          INIT_DCDC_CONFIG_0, \
          INIT_DCDC_CONFIG_1, \
          INIT_ANCTL_DCDC_REG0, \
          INIT_ANCTL_DCDC_REG1, \
          INIT_ANCTL_DCDC_REG2, \
          INIT_ANCTL_DCDC_REG3, \
          INIT_ANCTL_CONFIG_6, \
          INIT_ANCTL_CONFIG_0, \
          INIT_ANCTL_CONFIG_BBP, \
          INIT_ANCTL_CONFIG_7_5, \
          INIT_ANCTL_CONFIG_7_6, \
          INIT_ANCTL_FS_REG0, \
          INIT_BBP_POP_CLIPPING, \
          INIT_PA1G_CONFIG_2, \
          INIT_RX_POWSET, \
          INIT_ANCTL_PA1G_REG0, \
          INIT_RX_CONFIG_6_2ND, \
          END_REG_DATA \
}

#define RF_REG_CONFIG_ARRAY { \
          ANCTL_CONFIG_40, \
          ANCTL_CONFIG_41, \
          ANCTL_CONFIG_42, \
          ANCTL_CONFIG_43, \
          ANCTL_CONFIG_44, \
          ANCTL_CONFIG_45, \
          ANCTL_CONFIG_46, \
          ANCTL_CONFIG_47, \
          ANCTL_CONFIG_48, \
          ANCTL_CONFIG_49, \
          ANCTL_CONFIG_50, \
          ANCTL_CONFIG_51, \
          ANCTL_CONFIG_52, \
          ANCTL_CONFIG_53, \
          ANCTL_CONFIG_54, \
          TPM_DMI_CONFIG_1, \
          MAC_TX_DATDLV, \
          BBP_CORE52, \
          BBP_AGC24, \
          RX_GAIN_FILTSET, \
          BBP_AGC43, \
          BBP_CH_FILT0, \
          BBP_CH_FILT1, \
          BBP_CH_FILT2, \
          BBP_CH_FILT3, \
          BBP_CH_FILT4, \
          BBP_CH_FILT5, \
          BBP_CH_FILT6, \
          BBP_CH_FILT7, \
          BBP_CH_FILT8, \
          BBP_CH_FILT9, \
          BBP_CH_FILT10, \
          BBP_CH_FILT11, \
          BBP_RXLPF_1M_0, \
          BBP_RXLPF_1M_1, \
          BBP_RXLPF_1M_2, \
          BBP_AGC18, \
          BBP_AGC21, \
          BBP_RXIQCAL, \
          CTQBPSD_CONFIG_3, \
          RX_CONFIG_5, \
          SYNTH_CAL_CONFIG_0, \
          SYNTH_CAL_CONFIG_1, \
          SYNTH_CAL_CONFIG_2, \
          SYNTH_CAL_TRIG, \
          ANCTL_CONFIG_1, \
          MODEM_CLK_REG, \
          DCDC_CONFIG_1, \
          ANCTL_DCDC_REG3, \
          PA1G_CONFIG_0, \
          PA1G_CONFIG_1, \
          PA1G_CONFIG_2, \
          ANCTL_PA1G_REG0, \
          ANCTL_CONFIG_7, \
          ASARADC_CONFIG_11, \
          ANCTL_CONFIG_7_2, \
          END_REG_DATA \
}

#define RF_REG_CONFIG_EX_ARRAY { \
          XO_CONFIG_1, \
          GPIO_0_CFG_REG, \
          GPIO_1_CFG_REG, \
          PMU_CTL_REG, \
          PMU_TIM_REG, \
          PMU_CTL, \
          PMU_CONFIG_0, \
          INT_EN_REG, \
          MAC_TX_CFG2, \
          MAC_TX_CFG, \
          MAC_TX_CFG3, \
          MAC_TX_CFG4, \
          BBP_CORR_CODE_BT_BLE, \
          BBP_CORR_ERR_CNT, \
          FIFO_FEA_REG, \
          END_REG_DATA \
}
/* --------------------------------------------------------------------------*/
/* Hopping Register Data                                                     */
/* --------------------------------------------------------------------------*/
#define NUM_OF_CH_TABLE     2
//====== Register data of channel_pll_table ======
#define CHANNEL_PLL_TABLE_CH0_FREQ {0x08, 0xA6, 0xD3, 0x19}
#define CHANNEL_PLL_TABLE_CH1_FREQ {0xA8, 0x2C, 0xD5, 0x19}

/* --------------------------------------------------------------------------*/
/* Hopping Register Array                                                    */
/* --------------------------------------------------------------------------*/
//====== Array of channel_pll_table ======
#define CHANNEL_PLL_TABLE_ARRAY { \
          CHANNEL_PLL_TABLE_CH0_FREQ, \
          CHANNEL_PLL_TABLE_CH1_FREQ, \
}

#endif /*__SPI_ER41XX_CONFIG_H__*/
/*** (C) COPYRIGHT 2020 ESMT Technology Corp. ***/
