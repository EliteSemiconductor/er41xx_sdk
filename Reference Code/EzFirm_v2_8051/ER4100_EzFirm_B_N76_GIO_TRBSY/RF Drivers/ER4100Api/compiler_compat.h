/* **********************************************************************
 * @file     compiler_compat.h
 * @version  V1.0.0
 * @brief    Cross-compiler memory qualifier compatibility macros
 * Supports 8051, ARM, AVR, PIC, and Renesas compilers.
 *
 * @copyright (C) COPYRIGHT 2026 ESMT Technology Corp.
 * ********************************************************************** */

#ifndef __COMPILER_COMPAT_H__
#define __COMPILER_COMPAT_H__

/* Ultimate Compatibility Macro: For Renesas (CC-RL/CA78K0R), 8051, ARM, and other compilers */
#if defined(__SDCC) || defined(__SDCC_51)
  /* 1. Open-source SDCC (8051) */
  #define _D_CODE    __code
#elif defined(__CA51__) || defined(__C51__)
  /* 2. Keil C51 (8051) */
  #define _D_CODE    code
#elif defined(__ICC8051__)
  /* 3. IAR for 8051 */
  #define _D_CODE    __code
#elif defined(__AVR__) && defined(__GNUC__)
  /* 4. Atmel/Microchip AVR 8-bit (e.g., ATmega) */
  #include <avr/pgospace.h>
  #define _D_CODE    PROGMEM
#elif defined(__XC8)
  /* 5. Microchip XC8 (PIC 8-bit) */
  #define _D_CODE    __code
#elif defined(__CCRL__)
  /* 6. Renesas CC-RL Compiler (CS+ for CC / RL78) */
  /* CC-RL relies on standard 'const' with appropriate memory model to store data in Flash */
  #define _D_CODE    
#elif defined(__CA78K0R__)
  /* 7. Renesas CA78K0R Legacy Compiler (CS+ for CA) */
  /* Legacy Harvard architecture requires '__far' qualifier to prevent addressing overflow */
  #define _D_CODE    __far
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__ICCARM__) || defined(__GNUC__) || defined(__XC32)
  /* 8. 32-bit Architectures (ARM Cortex-M, Microchip XC32, Renesas RX/RA, etc.) */
  #define _D_CODE    
#else
  /* 9. Default fallback option */
  #define _D_CODE    const
#endif

#endif /* __COMPILER_COMPAT_H__ */
