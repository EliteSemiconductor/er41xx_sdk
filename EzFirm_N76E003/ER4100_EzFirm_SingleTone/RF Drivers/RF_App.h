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
/* Typedef ------------------------------------------------------------------*/
/* Extend Variables ---------------------------------------------------------*/

/* Function -----------------------------------------------------------------*/

/* Configuration & Init */
uint8_t XTAPP_Init(void);
/* Main handler */
void XTAPP_Scan(void);

#endif /* __RF_APP_H__ */

/*** (C) COPYRIGHT 2024 ESMT Technology Corp. ***/
