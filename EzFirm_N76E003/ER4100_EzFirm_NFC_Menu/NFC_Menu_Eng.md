# ER4100 EzFirm — NFC_Menu

## Overview

Demonstrates NFC tag read/write operation using the ER4100 RF transceiver in NFC mode.
The ER4100 acts as an NFC tag; an NFC reader (phone/reader device) can access its memory blocks.
The MCU configures and monitors the tag via the SPI interface.

On NFC field attach, the MCU prints an interactive UART menu. The developer can issue commands
to read/write data blocks, read DPE registers, or configure the IRQ source to test
host-triggered status events (USER_CFG4~7).

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
- Block 4~15 (0x9010~0x903C)  : Data0~Data47 (user data, locked by Lock1)
- Block 16~247 (0x9040~0x93DC): Data48~Data975 (user data, locked by Dynamic Lock Bits)
- Block 248~254 (0x93E0~0x93F8): Dynamic Lock Bits
  - Each 32-bit word covers 32 blocks
  - 0x93E0 bit0~31 : controls Block 16~47
  - 0x93E4 bit0~31 : controls Block 48~79
  - ... (each register covers 32 blocks)
  - 0x93F8 bit0~31 : controls Block 208~239
- Block 255 (0x93FC) : Special register
  - Byte0 (RFU)       : reserved
  - Byte1 (Dynamic Lock, bit0~7) : controls Block 240~247
  - Byte2 (DPE Status): NFC interrupt status (written by NFC host, read by MCU)
  - Byte3 (DPE Control): NFC interrupt control (written by MCU)

---

## DPE (Digital Protocol Engine)

DPE provides two registers accessible by MCU at `0x93FC`:

- Byte3 — DPE Control (written by MCU)
- Byte2 — DPE Status  (written by NFC host/reader, read by MCU)

### DPE Control (0x93FC Byte3)

- Bit 7~5 : reserved
- Bit 4    : `DPE_rst`        — when 1, DPE is held in reset state
- Bit 3    : `clear/mask IRQ` — when set to 1, any active interrupt is cleared and remains masked until this bit is set back to 0
- Bit 2~0  : `irq_src[2:0]`  — selects which DPE Status bit triggers INT_N to MCU (direct index 0~7)
  - `irq_src = 1` → PWRGOOD (NFC field detected / removed)
  - `irq_src = 4` → USER_CFG4

### DPE Status (0x93FC Byte2)

Set by DPE when the corresponding event occurs; triggers INT_N to MCU when the selected bit index matches `irq_src`.
MCU reads this byte to determine which event fired, then clears it via `NFC_Clear_NFC_Event()`:
1. Write 0x00 to Byte2 via SPI to clear the status flags
2. Set the `clear/mask` bit (Byte3 bit3) to release the INT_N line
3. Wait 1 ms, then clear the `clear/mask` bit to re-arm the interrupt

- Bit 7 : USER_CFG7        — set by DPE on user event, triggers MCU interrupt
- Bit 6 : USER_CFG6        — set by DPE on user event, triggers MCU interrupt
- Bit 5 : USER_CFG5        — set by DPE on user event, triggers MCU interrupt
- Bit 4 : USER_CFG4        — set by DPE on user event, triggers MCU interrupt
- Bit 3 : `TX_REPLY`       — DPE is transmitting a reply
- Bit 2 : `RX_CMD`         — DPE is receiving a command
- Bit 1 : `PWRGOOD`        — NFC field is detected
- Bit 0 : `IGNORED`        — no purpose

---

## NFC IRQ Source Macros

Defined in `RF_NFC_APP.h`, used as input to `NFC_Select_IRQ_Src()`.
These are **direct index values** (0~7), not bitmasks:

- `NFC_IGNORED`   = 0 — no interrupt
- `NFC_PWRGOOD`   = 1 — interrupt on NFC field detected / removed
- `NFC_RX_CMD`    = 2 — interrupt on command received
- `NFC_TX_REPLY`  = 3 — interrupt on reply transmitted
- `NFC_USER_CFG4` = 4 — user-defined
- `NFC_USER_CFG5` = 5 — user-defined
- `NFC_USER_CFG6` = 6 — user-defined
- `NFC_USER_CFG7` = 7 — user-defined

DPE Status bitmasks (`NFC_PWRGOOD_MASK`, `NFC_USER_CFG4_MASK`, ...) are also defined
in `RF_NFC_APP.h` for reading individual event flags from the Status byte.

---

## Key API Functions (RF_NFC_APP.c)

### Low-Level NFC Control

- `NFC_Enable()`
  Enables NFC mode (writes 0x00000000 to reg 0x0030).

- `NFC_Disable()`
  Disables NFC mode (writes 0x00000001 to reg 0x0030).

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  Sets `irq_src[2:0]` in DPE Control (0x93FC Byte3) to select which event triggers INT_N.
  Input: direct index 0~7 (use `NFC_IGNORED` ~ `NFC_USER_CFG7`).

- `NFC_Get_IRQ_Src_Select()`
  Returns current `irq_src[2:0]` value from DPE Control byte.

- `NFC_Clear_NFC_Event()`
  Clears the active NFC interrupt by setting then clearing the `clear/mask` bit (Byte3 bit3).
  A 1 ms delay is inserted between set and clear to allow DPE to finish clearing the interrupt.

- `NFC_Set_Impedance(uint8_t u8IMP_0_7)`
  Sets NFC antenna impedance (0~7) via reg 0x1700 bits[12:10].

- `NFC_Get_Impedance()`
  Returns current NFC antenna impedance setting (0~7).

- `NFC_SetLockBlock_0_15(uint8_t value, uint16_t start_block, uint8_t end_block)`
  Sets or clears lock bits for Block 0~15 via Lock0/Lock1 bytes in Block 2 (0x9008).

- `NFC_SetLockBlock_16_247(uint8_t value, uint16_t start_block, uint8_t end_block)`
  Sets or clears dynamic lock bits for Block 16~247 via registers 0x93E0~0x93FC.
  Protects DPE registers in 0x93FC (only bit0~7 of Byte1 may be modified).

### Application Layer

- `XTAPP_NFC_Config()`
  Initializes NFC tag:
  - Resets DPE, then disables and re-enables NFC engine
  - Unlocks all blocks (Block 0~247) for reader access
  - Sets DPE to wait for NFC field (`NFC_PWRGOOD`)
  - Writes default header blocks (Block 0~3)

- `XTAPP_NFC_IRQ_Parser()`
  Called from `XTAPP_Scan` on NFC interrupt. Reads DPE block, dispatches on `irq_src`:
  - `NFC_PWRGOOD`: sets `u8NfcPwrGood = 1`, switches irq_src to `NFC_IGNORED` to prevent
    continuous re-trigger while field is present, then prints the UART menu
  - `NFC_USER_CFG4~7`: prints DPE Status byte value, switches irq_src back to `NFC_IGNORED`
  - Always calls `NFC_Clear_NFC_Event()` at end; returns the `irq_src` index that fired

- `XTAPP_NFC_Task()`
  Processes one UART command character (non-blocking, checks `RI` flag directly):
  - `'1'` : MCU writes test pattern (0x01~0x40) to data blocks (Block 4~19)
  - `'2'` : MCU reads data blocks (Block 4~19) and prints contents
  - `'3'` : Print DPE Control and Status register bytes
  - `'4'` : Set irq_src = USER_CFG4, then wait for reader to trigger status4
  - `'5'` : Set irq_src = USER_CFG5, then wait for reader to trigger status5
  - `'6'` : Set irq_src = USER_CFG6, then wait for reader to trigger status6
  - `'7'` : Set irq_src = USER_CFG7, then wait for reader to trigger status7
  - `'a'` : Dump all blocks (Block 0~255) to UART

- `XTAPP_Scan()`
  Main loop RF handler:
  1. Returns immediately if `rf_err_mode == 1`
  2. On NIRQ assert: reads and clears interrupt status; if NFC interrupt, calls `XTAPP_NFC_IRQ_Parser()`
  3. Enters `while(u8NfcPwrGood)` loop: polls PWRGOOD bit for field detach
     (debounced by `DETTACHED_CNT_THR`); calls `XTAPP_NFC_Task()` each iteration
  4. On detach: clears `u8NfcPwrGood`, re-arms `irq_src = NFC_PWRGOOD`, prints `NFC Detached`
