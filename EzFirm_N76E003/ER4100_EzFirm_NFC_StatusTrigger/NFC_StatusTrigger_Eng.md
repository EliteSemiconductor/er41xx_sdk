# ER4100 EzFirm — NFC_StatusTrigger

## Overview

Demonstrates NFC field attach/detach detection combined with a USER_CFG4 status trigger.
The ER4100 acts as an NFC tag. After the NFC field is detected (via INT_N interrupt),
the IRQ source is dynamically switched to `NFC_USER_CFG4` so that the MCU can also
receive a notification when the NFC reader writes a specific command.
Field detach is detected by SPI polling of the PWRGOOD bit with a 50-count debounce.

**Key behavior:**
- On startup: IRQ source = `NFC_PWRGOOD`; INT_N fires when a field is detected
- On attach: switches IRQ source to `NFC_USER_CFG4`; reader can now trigger a status event
- STATUS4 trigger: NFC reader writes `A2 FF 00 00 10 04` to set bit4 of DPE Status → INT_N fires
- On detach: detected by SPI polling (50 consecutive PWRGOOD=0); IRQ source reset to `NFC_PWRGOOD`

**Difference vs NFC_Field_Interrupt:**

| | NFC_StatusTrigger | NFC_Field_Interrupt |
|---|---|---|
| Attach detection | INT_N (NFC_PWRGOOD) | INT_N (NFC_PWRGOOD) |
| After attach | switches IRQ src to NFC_USER_CFG4 | stays on NFC_PWRGOOD |
| STATUS4 trigger | yes — INT_N fires on USER_CFG4 | no |
| Detach detection | SPI PWRGOOD poll + 50-count debounce | INT_N (PWRGOOD=0) |

---

## Program Flow

```
main()
 ├── InitialUART0_Timer1()       UART 115200 baud
 ├── XTAPP_Init()
 │    ├── TRx_IoConfig()         SPI / INT_N GPIO setup
 │    ├── XTAPP_Strobe()         Verify ER4100 chip ID (0xF100 = 0x90_00_00)
 │    ├── TRx_SW_Reset()
 │    ├── TRx_Init()             Load RF register defaults
 │    ├── Enable NFC IRQ mask    Write 0x00_02_00_00 → 0xF004
 │    └── XTAPP_NFC_Config()
 │         ├── NFC_Disable() / NFC_Enable()
 │         ├── NFC_SetLockBlock_0_15(0, 0, 15)     unlock all
 │         ├── NFC_SetLockBlock_16_247(0, 16, 247)  unlock all
 │         ├── NFC_Select_IRQ_Src(NFC_PWRGOOD)     wait for field
 │         └── Write header blocks (Block 0~3)
 └── while(1)
      └── XTAPP_NFC_Scan()
           ├── (skip if rf_err_mode = 1)
           ├── if NIRQ_Value == 0  (INT_N asserted)
           │    ├── TRx_GetIntStatus()     Read 0xF004
           │    ├── IS_INT_ST_NFC()?
           │    │    ├── if irq_src == NFC_PWRGOOD && PWRGOOD bit set
           │    │    │    ├── u8NfcPwrGood = 1, print "+--- NFC Attached ---+"
           │    │    │    ├── NFC_Select_IRQ_Src(NFC_USER_CFG4)
           │    │    │    └── XTAPP_NFC_PrintInstructions()
           │    │    └── else if irq_src == NFC_USER_CFG4
           │    │         └── print "+--- STATUS4 Trigger---+"
           │    ├── NFC_Clear_NFC_Event()
           │    └── TRx_ClearIntFlag()
           └── if u8NfcPwrGood  (poll for detach)
                ├── if PWRGOOD bit == 0
                │    ├── u8DettachedCnt++
                │    └── if u8DettachedCnt >= 50
                │         ├── u8NfcPwrGood = 0, print "+--- NFC Detached ---+"
                │         └── NFC_Select_IRQ_Src(NFC_PWRGOOD)
                └── else (field still present)
                     └── u8DettachedCnt = 0
```

---

## STATUS4 Trigger

When attached, the IRQ source is switched to `NFC_USER_CFG4` (bit 4 of DPE Status).
To trigger this event, the NFC reader/phone must write the following NFC command:

```
A2 FF 00 00 10 04
```

- `A2`    : WRITE command (ISO 15693 / NFC Forum T5T)
- `FF`    : block address = 0xFF (Block 255 = DPE register at 0x93FC)
- `00 00 10 04` : data bytes — bit4 of byte[2] (DPE Status) = USER_CFG4 = 1

When DPE detects USER_CFG4 rising, it asserts INT_N. The MCU then prints:

```
+--- STATUS4 Trigger---+
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
- Bit 4 : USER_CFG4  ← polled in this demo after field attach
- Bit 3 : TX_REPLY   — DPE is transmitting a reply
- Bit 2 : RX_CMD     — DPE is receiving a command
- Bit 1 : PWRGOOD    — NFC field is detected
- Bit 0 : IGNORED    — no purpose

---

## NFC IRQ Source Macros

Used as input to `NFC_Select_IRQ_Src()`.
These are **direct index values** (0~7), not bitmasks:

- `NFC_IGNORED`   = 0 — no interrupt
- `NFC_PWRGOOD`   = 1 — interrupt on NFC field detected  ← used on startup / after detach
- `NFC_RX_CMD`    = 2 — interrupt on command received
- `NFC_TX_REPLY`  = 3 — interrupt on reply transmitted
- `NFC_USER_CFG4` = 4 — host-configurable  ← used after field attach

---

## API Functions

- `NFC_Enable()`
  Enables NFC mode (write 0x00000000 to reg 0x0030).

- `NFC_Disable()`
  Disables NFC mode (write 0x00000001 to reg 0x0030).

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  Sets `irq_src[2:0]` in DPE Control (0x93FC Byte3) to select which event triggers INT_N.
  Input: direct index 0~7 (use `NFC_IGNORED` ~ `NFC_USER_CFG7`).
  Called twice per attach/detach cycle: switch to `NFC_USER_CFG4` on attach; back to `NFC_PWRGOOD` on detach.

- `NFC_Get_IRQ_Src_Select()`
  Returns current `irq_src[2:0]` value from DPE Control byte.
  Used in `XTAPP_NFC_Scan()` to distinguish attach from STATUS4 interrupt.

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
  - Sets IRQ source to `NFC_PWRGOOD` (wait for field on startup)
  - Writes default header blocks (Block 0~3)

- `XTAPP_Init()`
  Initializes RF transceiver and NFC. Returns 0 on success, 1 on failure.
  Writes `0x00020000` to 0xF004 to enable NFC interrupt mask.

- `XTAPP_Get_NFC_Statu_Reg()`
  Reads DPE Status byte (0x93FC Byte2) and returns the PWRGOOD bit.
  Returns non-zero if NFC field is present.

- `XTAPP_NFC_PrintInstructions()`
  Prints UART instructions explaining how to send the STATUS4 trigger command from an NFC reader/phone.

- `XTAPP_NFC_Scan()`
  Main loop handler. Handles both INT_N events and SPI PWRGOOD polling:
  - **Attach** (irq_src == NFC_PWRGOOD, PWRGOOD bit set): prints `"+--- NFC Attached ---+"`,
    switches IRQ source to `NFC_USER_CFG4`, prints instructions
  - **STATUS4** (irq_src == NFC_USER_CFG4): prints `"+--- STATUS4 Trigger---+"`
  - **Detach** (polling): increments `u8DettachedCnt` each time PWRGOOD is low while attached;
    declares detach after 50 consecutive readings; prints `"+--- NFC Detached ---+"`;
    resets IRQ source to `NFC_PWRGOOD`
