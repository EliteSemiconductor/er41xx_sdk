# ER4100 EzFirm — NFC_ReadWrite

## Overview

Demonstrates NFC tag read/write operation using the ER4100 RF transceiver in NFC mode.
The ER4100 acts as an NFC tag; an NFC reader (phone/reader device) can access its memory blocks.
The MCU configures and monitors the tag via the SPI interface.

On NFC field attach, the MCU:
1. Prints RF command instructions for the developer to test with a reader/phone.
2. Runs a write/verify test (`XTAPP_ReadWriteTest_MCU_Side`) on data blocks 4~19 (64 bytes).

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
  - `irq_src = 6` → USER_CFG6

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

- `XTAPP_Get_NFC_Statu_Reg()`
  Reads DPE Status byte (0x93FC Byte2) and returns the `PWRGOOD` bit.
  Returns non-zero if NFC field is present, 0 if absent.

- `XTAPP_NFC_PrintInstructions()`
  Prints RF command examples to UART for the developer to test with a reader/phone.
  Covers Write (0xA2), Read (0x30), and Fast Read (0x3A) with example command bytes.

- `XTAPP_ReadWriteTest_MCU_Side()`
  MCU-side read/write/verify test on data blocks 4~19 (64 bytes, `BUFFER_SIZE`):
  - Step 1: Read current content of data blocks into `gXtBuffer`
  - Step 2: Increment every byte by 1, write back to NFC memory
  - Step 3: Read back from NFC memory and verify against `gXtBuffer` (written values)
  Prints `MCU Verify PASS` or `MCU Verify FAIL` with per-byte mismatch details.

- `XTAPP_ReadWriteTest_Phone_Side()`
  Called from main loop. Polls NIRQ; on NFC interrupt:
  - If PWRGOOD set (field attached): prints `NFC attached`, calls `XTAPP_NFC_PrintInstructions()`
  - If PWRGOOD clear (field removed): prints `NFC Detached`
  - Calls `NFC_Clear_NFC_Event()` to clear the interrupt

---

## RF Command Reference

CRC-16 is generated and verified by hardware automatically — do not include it in the command bytes.

### Write (0xA2)

```
A2 | BK | D0 D1 D2 D3
```

- `BK`    = block number (4~247 for user data blocks)
- `D0~D3` = 4 bytes to write (little-endian, D0 at lowest address)

Example:
```
A2 06 AA BB CC DD   => write 0xAABBCCDD to block 6
```

### Read (0x30)

```
30 | BK
```

- `BK` = block number (0~255)

Example:
```
30 05   => read block 5
```

### Fast Read (0x3A)

```
3A | STA | END
```

- `STA` = start block (0~255)
- `END` = end block (0~255), inclusive
- Recommended max: 63 blocks per command

Example:
```
3A 04 13   => read block 4~19 (16 blocks, 64 bytes)
```
