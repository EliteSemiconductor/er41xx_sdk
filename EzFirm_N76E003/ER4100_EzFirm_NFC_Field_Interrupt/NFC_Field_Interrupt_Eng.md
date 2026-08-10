# ER4100 EzFirm — NFC_Field_Interrupt

## Overview

Demonstrates NFC field detection using the ER4100 RF transceiver in NFC mode.
The ER4100 acts as an NFC tag and monitors the INT_N pin for interrupts.
When an NFC reader (phone/reader device) approaches or leaves, the DPE triggers INT_N,
and the MCU prints an attach or detach message via UART.

**Key behavior:**
- IRQ source is fixed to `NFC_PWRGOOD` (NFC field presence/absence)
- No read/write operations — field detection only
- Main loop polls INT_N pin; all handling is done in `XTAPP_NFC_Scan()`

---

## Program Flow

```
main()
 ├── InitialUART0_Timer1()      UART 115200 baud
 ├── XTAPP_Init()
 │    ├── TRx_IoConfig()        SPI / INT_N GPIO setup
 │    ├── XTAPP_Strobe()        Verify ER4100 chip ID (0xF100 = 0x90_00_00)
 │    ├── TRx_SW_Reset()
 │    ├── TRx_Init()            Load RF register defaults
 │    ├── Enable NFC IRQ mask   Write 0x00_02_00_00 → 0xF004
 │    └── XTAPP_NFC_Config()
 │         ├── NFC_Disable() / NFC_Enable()
 │         ├── NFC_SetLockBlock_0_15(0, 0, 15)    unlock all
 │         ├── NFC_SetLockBlock_16_247(0, 16, 247) unlock all
 │         ├── NFC_Select_IRQ_Src(NFC_PWRGOOD)    IRQ on field
 │         └── Write header blocks (Block 0~3)
 └── while(1)
      └── XTAPP_NFC_Scan()
           ├── (skip if rf_err_mode = 1)
           ├── Check NIRQ_Value == 0  (INT_N asserted)
           ├── TRx_GetIntStatus()     Read 0xF004
           ├── IS_INT_ST_NFC()?
           │    ├── XTAPP_Get_NFC_Statu_Reg()  Read PWRGOOD bit (0x93FC Byte2 bit1)
           │    ├── PWRGOOD = 1 → print "+--- NFC attached ---+"
           │    └── PWRGOOD = 0 → print "+--- NFC Detached ---+"
           ├── NFC_Clear_NFC_Event()  Clear DPE status, toggle clear/mask bit
           └── TRx_ClearIntFlag()     Clear 0xF004 interrupt flag
```

---

## NFC Memory Map

NFC memory spans from `0x9000` to `0x93FC`, organized as 256 blocks of 4 bytes each.

- MCU Addr (Hex) : absolute register address on ER4100 SPI bus
- Host Block (Dec): block number visible to the NFC reader
- Bytes stored in little-endian order (Byte0 = LSB at lowest address)

**Block layout:**

- Block 0 (0x9000) : UID (Byte0~3)
- Block 1 (0x9004) : UID (Byte0~3)
- Block 2 (0x9008) : BCC1 / Internal / Lock0 / Lock1
  - Lock0 (Byte2, bits 0~7) : lock bits for Block 0~7
  - Lock1 (Byte3, bits 0~7) : lock bits for Block 8~15
- Block 3 (0x900C) : CC0 / CC1 / CC2 / CC3  (Capability Container)
- Block 4~15 (0x9010~0x903C)  : Data0~Data47 (user data)
- Block 16~247 (0x9040~0x93DC): Data48~Data975 (user data)
- Block 248~254 (0x93E0~0x93F8): Dynamic Lock Bits
- Block 255 (0x93FC) : DPE Control / Status register ← key register for this demo

---

## DPE (Digital Protocol Engine)

DPE provides two registers accessible by MCU at `0x93FC`:

- Byte3 — DPE Control (written by MCU)
- Byte2 — DPE Status  (written by NFC host/reader, read by MCU)

### DPE Control (0x93FC Byte3)

- Bit 7~5 : reserved
- Bit 4    : `DPE_rst`        — when 1, DPE is held in reset state
- Bit 3    : `clear/mask IRQ` — when set to 1, active interrupt is cleared and masked; clear by writing 0
- Bit 2~0  : `irq_src[2:0]`  — selects which DPE Status bit triggers INT_N to MCU (direct index 0~7)

### DPE Status (0x93FC Byte2)

Written by the NFC host (reader/phone); triggers INT_N to MCU when the selected bit index matches `irq_src`.

- Bit 7 : USER_CFG7
- Bit 6 : USER_CFG6
- Bit 5 : USER_CFG5
- Bit 4 : USER_CFG4
- Bit 3 : TX_REPLY   — DPE is transmitting a reply
- Bit 2 : RX_CMD     — DPE is receiving a command
- Bit 1 : PWRGOOD    — NFC field is detected  ← used in this demo
- Bit 0 : IGNORED    — no purpose

---

## NFC IRQ Source Macros

Used as input to `NFC_Select_IRQ_Src()`.
These are **direct index values** (0~7), not bitmasks:

- `NFC_IGNORED`   = 0 — no interrupt
- `NFC_PWRGOOD`   = 1 — interrupt on NFC field detected  ← selected in this demo
- `NFC_RX_CMD`    = 2 — interrupt on command received
- `NFC_TX_REPLY`  = 3 — interrupt on reply transmitted
- `NFC_USER_CFG4` = 4 — host-configurable
- `NFC_USER_CFG5` = 5 — host-configurable
- `NFC_USER_CFG6` = 6 — host-configurable
- `NFC_USER_CFG7` = 7 — host-configurable

---

## API Functions

- `NFC_Enable()`
  Enables NFC mode (write 0x00000000 to reg 0x0030).

- `NFC_Disable()`
  Disables NFC mode (write 0x00000001 to reg 0x0030).

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  Sets `irq_src[2:0]` in DPE Control (0x93FC Byte3) to select which event triggers INT_N.
  Input: direct index 0~7 (use `NFC_IGNORED` ~ `NFC_USER_CFG7`).

- `NFC_Get_IRQ_Src_Select()`
  Returns current `irq_src[2:0]` value from DPE Control byte.

- `NFC_Clear_NFC_Event()`
  Clears the active NFC interrupt:
  1. Write 0x00 to DPE Status byte (Byte2) to acknowledge event
  2. Set `clear/mask` bit (Byte3 bit3) to trigger interrupt clear
  3. Wait 1 ms
  4. Clear `clear/mask` bit to re-enable interrupt

- `NFC_SetLockBlock_0_15(uint8_t value, uint16_t start_block, uint8_t end_block)`
  Sets or clears lock bits for Block 0~15 via Lock0/Lock1 bytes in Block 2 (0x9008).

- `NFC_SetLockBlock_16_247(uint8_t value, uint16_t start_block, uint8_t end_block)`
  Sets or clears dynamic lock bits for Block 16~247 via registers 0x93E0~0x93FC.

---

## Application Layer Functions (RF_NFC_APP.c)

- `XTAPP_NFC_Config()`
  Initializes NFC tag:
  - Disables then re-enables NFC engine
  - Unlocks all blocks (Block 0~247)
  - Sets IRQ source to `NFC_PWRGOOD`
  - Writes default header blocks (Block 0~3)

- `XTAPP_Init()`
  Initializes RF transceiver and NFC. Returns 0 on success, 1 on failure.

- `XTAPP_Get_NFC_Statu_Reg()`
  Reads DPE Status byte (0x93FC Byte2) and returns the PWRGOOD bit.
  Returns non-zero if NFC field is present.

- `XTAPP_NFC_Scan()`
  Main loop handler. Polls INT_N pin and processes NFC field events:
  - Checks `NIRQ_Value == 0` (INT_N asserted low)
  - Reads interrupt status register (0xF004) to confirm NFC interrupt source
  - Reads PWRGOOD bit to determine field attached or detached
  - Prints `"+--- NFC attached ---+"` or `"+--- NFC Detached ---+"`
  - Calls `NFC_Clear_NFC_Event()` then `TRx_ClearIntFlag()` to reset interrupt state
