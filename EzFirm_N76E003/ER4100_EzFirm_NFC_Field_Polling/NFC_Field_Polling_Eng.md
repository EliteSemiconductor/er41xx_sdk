# ER4100 EzFirm — NFC_Field_Polling

## Overview

Demonstrates NFC field detection using the ER4100 RF transceiver in NFC mode.
The ER4100 acts as an NFC tag. The MCU detects field presence and absence by
directly reading the PWRGOOD bit in the DPE Status register via SPI — no interrupt is used.

**Key behavior:**
- IRQ source set to `NFC_IGNORED`; INT_N pin is not used
- Main loop calls `XTAPP_NFC_Scan()` every cycle to poll the PWRGOOD bit
- Detach uses a debounce counter (`DETACHED_CNT_THR = 50`) to avoid false triggers

**Difference vs NFC_Field_Interrupt:**

| | NFC_Field_Polling | NFC_Field_Interrupt |
|---|---|---|
| Detection method | SPI register poll | INT_N pin |
| IRQ source | `NFC_IGNORED` | `NFC_PWRGOOD` |
| 0xF004 mask | `0x00000000` (disabled) | `0x00020000` (enabled) |
| Detach debounce | 50 consecutive reads | None |

---

## Program Flow

```
main()
 ├── InitialUART0_Timer1()       UART 115200 baud
 ├── XTAPP_Init()
 │    ├── TRx_IoConfig()         SPI GPIO setup
 │    ├── XTAPP_Strobe()         Verify ER4100 chip ID (0xF100 = 0x90_00_00)
 │    ├── TRx_SW_Reset()
 │    ├── TRx_Init()             Load RF register defaults
 │    ├── Disable NFC IRQ mask   Write 0x00_00_00_00 → 0xF004
 │    └── XTAPP_NFC_Config()
 │         ├── NFC_Disable() / NFC_Enable()
 │         ├── NFC_SetLockBlock_0_15(0, 0, 15)     unlock all
 │         ├── NFC_SetLockBlock_16_247(0, 16, 247)  unlock all
 │         ├── NFC_Select_IRQ_Src(NFC_IGNORED)     no interrupt
 │         └── Write header blocks (Block 0~3)
 └── while(1)
      └── XTAPP_NFC_Scan()
           ├── (skip if rf_err_mode = 1)
           ├── if !u8NfcPwrGood && XTAPP_Get_NFC_Statu_Reg()
           │    └── u8NfcPwrGood = 1, print "+--- NFC Attached ---+"
           └── else if u8NfcPwrGood
                ├── if !XTAPP_Get_NFC_Statu_Reg()
                │    ├── u8DettachedCnt++
                │    └── if u8DettachedCnt >= DETACHED_CNT_THR(50)
                │         └── u8NfcPwrGood = 0, print "+--- NFC Detached ---+"
                └── else  (field still present)
                     └── u8DettachedCnt = 0   (reset debounce)
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

Written by the NFC host (reader/phone). In polling mode, the MCU reads this byte directly via SPI.

- Bit 7 : USER_CFG7
- Bit 6 : USER_CFG6
- Bit 5 : USER_CFG5
- Bit 4 : USER_CFG4
- Bit 3 : TX_REPLY   — DPE is transmitting a reply
- Bit 2 : RX_CMD     — DPE is receiving a command
- Bit 1 : PWRGOOD    — NFC field is detected  ← polled directly in this demo
- Bit 0 : IGNORED    — no purpose

---

## API Functions

- `NFC_Enable()`
  Enables NFC mode (write 0x00000000 to reg 0x0030).

- `NFC_Disable()`
  Disables NFC mode (write 0x00000001 to reg 0x0030).

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  Sets `irq_src[2:0]` in DPE Control (0x93FC Byte3).
  Set to `NFC_IGNORED` in this demo — INT_N is not used.

- `NFC_Get_IRQ_Src_Select()`
  Returns current `irq_src[2:0]` value from DPE Control byte.

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
  - Sets IRQ source to `NFC_IGNORED` (polling mode — INT_N not used)
  - Writes default header blocks (Block 0~3)

- `XTAPP_Init()`
  Initializes RF transceiver and NFC. Returns 0 on success, 1 on failure.
  Writes `0x00000000` to 0xF004 to disable NFC interrupt mask.

- `XTAPP_Get_NFC_Statu_Reg()`
  Reads DPE Status byte (0x93FC Byte2) and returns the PWRGOOD bit.
  Returns non-zero if NFC field is present.

- `XTAPP_NFC_Scan()`
  Main loop handler. Detects NFC field attach/detach by polling PWRGOOD via SPI:
  - **Attach**: transitions `u8NfcPwrGood` 0→1 on first PWRGOOD high; prints `"+--- NFC Attached ---+"`
  - **Detach**: increments `u8DettachedCnt` each time PWRGOOD is low while attached;
    declares detach and prints `"+--- NFC Detached ---+"` only after 50 consecutive low readings
  - Resets `u8DettachedCnt` to 0 whenever PWRGOOD goes high again (debounce reset)
