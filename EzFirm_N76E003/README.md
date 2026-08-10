# ER4100 EzFirm SDK

Embedded firmware SDK for the ER4100 RF transceiver, targeting the Nuvoton N76E003 (8051) MCU.
Multiple project variants cover common wireless communication scenarios.

---

## Hardware Platform

- MCU              : Nuvoton N76E003 (8051-based, 16 MHz, 1T mode)
- RF Chip          : ER4100 (ESMT)
- Frequency        : Band 1: 99.3 ~ 125.5 MHz
                     Band 2: 196.5 ~ 253 MHz
                     Band 3: 387 ~ 508 MHz
                     Band 4: 790 ~ 1018 MHz
- Modulation       : FSK / GFSK
- Data Rate        : 0.625 ~ 2000 Kbps (Crystal 40 MHz)
                     0.6 ~ 1920 Kbps (Crystal 38.4 MHz)
- TX Power         : 14 dBm (780 ~ 899 MHz)
                     10 / 23 dBm (other frequency range)
- Crystal          : 38.4 MHz or 40 MHz
- MCU-RF Interface : SPI (Software bit-bang)

**SPI Pin Assignment:**
- MOSI    : P00
- MISO    : P01
- SCLK    : P10
- SS (CS) : P15
- IRQ     : P03

**UART Pin Assignment (UART0, 115200 baud):**
- TXD     : P06
- RXD     : P07

---

## Project Variants

### Transparent Mode

- `ER4100_EzFirm_TransTxRx`
  Standard bidirectional transparent TX/RX, 64-byte payload, P05 triggers TX
- `ER4100_EzFirm_TransTxRx_LongPkt`
  Long packet transparent mode, up to 512-byte payload with FIFO threshold + CRC-16 verification
- `ER4100_EzFirm_TransWOR`
  Wake-On-Radio RX — low-power receiver wakes on RF activity, outputs data via UART
- `ER4100_EzFirm_TransWOR_Ack`
  WOR RX with automatic ACK reply after receiving a packet

### IEEE 802.15.4 Mode

- `ER4100_EzFirm_802154TxRx`
  Basic 802.15.4 bidirectional TX/RX with hardware CRC and address filtering
- `ER4100_EzFirm_802154TxRx_Addressing`
  802.15.4 address filter demo — sends packets with different PANID/ADDR combinations via UART commands to verify accept/reject behavior
- `ER4100_EzFirm_802154TxRx_CCA`
  802.15.4 TX with CCA (Clear Channel Assessment) — checks channel before transmitting
- `ER4100_EzFirm_802154TxRx_PER`
  802.15.4 PER test — P05 sends 500 packets; RX side uses hardware CRC (IS_INT_ST_RX / IS_INT_ST_RXERR) to count ok/error and report running + final PER

### NFC Mode

- `ER4100_EzFirm_NFC_Field_Interrupt`
  Detects NFC field presence/absence via INT_N interrupt; IRQ source set to NFC_PWRGOOD
- `ER4100_EzFirm_NFC_Field_Polling`
  Detects NFC field presence/absence by polling the PWRGOOD bit via SPI; no interrupt used; detach debounced over 50 consecutive reads
- `ER4100_EzFirm_NFC_Menu`
  Interactive UART menu after NFC field attach — read/write data blocks and switch IRQ source to USER_CFG4~7
- `ER4100_EzFirm_NFC_ReadWrite`
  MCU reads and writes NFC tag memory (Block 4~19, 64 bytes) via SPI
- `ER4100_EzFirm_NFC_StatusTrigger`
  Detects field attach via INT_N (NFC_PWRGOOD); switches IRQ source to USER_CFG4 after attach so the NFC reader can trigger a status event; detach detected by SPI PWRGOOD polling with 50-count debounce

### Utility / Test

- `ER4100_EzFirm_SingleTone`
  Transmits a continuous single-tone carrier for RF testing and frequency verification
- `ER4100_EzFirm_RSSI_Scan`
  Measures ambient channel RSSI using CCA mode; formula: `RSSI_dBm = -((4095 - raw) / 8)`
- `ER4100_EzFirm_PowerSaving`
  Low-power operation via P05 button-triggered deep sleep / wakeup cycle
- `ER4100_EzFirm_PowerSaving_WUT`
  Power saving with WUT (Wakeup Timer) — automatic timed wakeup without button input
- `ER4100_EzFirm_TransTxRx_PER`
  Transparent mode PER test — P05 sends 500 packets with XOR checksum; RX verifies checksum and reports running + final PER

---

## Directory Structure

All EzFirm projects share the same top-level layout:

```
ER4100_EzFirm_<Variant>/
├── Common/          # Shared utilities: delays, UART, common typedefs
├── Include/         # MCU system headers (N76E003.h, SFR_Macro.h, etc.)
├── RF Drivers/      # RF application layer, HAL, and ER4100 SPI API
├── User/            # User application entry point (main.c)
├── Project/         # Keil uVision project files and build output (*.hex)
```

---

## Software Architecture

All EzFirm projects share the same four-layer architecture:

```
+-------------------------------+
|      User Application         |  User/main.c
|  init, main loop              |
+---------------+---------------+
                |
+---------------v---------------+
|    RF Application Layer       |  RF Drivers/RF_App.c
|  TX/RX control, IRQ handling  |
+---------------+---------------+
                |
+---------------v---------------+
|    Low-Level SPI Driver       |  RF Drivers/ER4100Api/SPI_ER41xx.c
|  Register R/W, FIFO, modes   |
+---------------+---------------+
                |
+---------------v---------------+
|       RF HAL Layer            |  RF Drivers/RF_Hal.c
|  SPI bit-bang, pin mapping    |
+---------------+---------------+
                |
           +----v----+
           |  ER4100  |
           | RF Chip  |
           +----------+
```

---

## RF Configuration

**Supported Frequency Bands:**
- Band 1 : 99.3 ~ 125.5 MHz
- Band 2 : 196.5 ~ 253 MHz
- Band 3 : 387 ~ 508 MHz
- Band 4 : 790 ~ 1018 MHz

**Common RF Parameters:**
- Modulation                   : FSK / GFSK
- Data Rate                    : 0.625 ~ 2000 Kbps (Crystal 40 MHz)
                                 0.6 ~ 1920 Kbps (Crystal 38.4 MHz)
- TX Power                     : 14 dBm @ 780 ~ 899 MHz
                                 10 / 23 dBm @ other frequency range
- Crystal                      : 38.4 MHz or 40 MHz
- Preamble                     : 0xAA, 8 bytes
- Bit Order                    : MSB first
- Syncword Bit Error Tolerance : 1 bit

---

## Build Environment

- **IDE:** Keil uVision (8051 toolchain)
- **Programmer:** Nu-Link (Nuvoton)
- **Programmer Driver Config:** `Project/Nu_Link_8051_Driver.ini`
- **Output:** `Project/Output/*.hex`

To build:
1. Open `Project/*.uvproj` in Keil uVision
2. Build All (F7)
3. Flash `Project/Output/*.hex` via Nu-Link

---

## EzCodeGen Configuration Tool

Each project includes an EzCodeGen `.ini` file used to generate the RF register table in `SPI_ER41xx_config.h`.
To regenerate `SPI_ER41xx_config.h`, open the `.ini` file with the ER4100 EzCodeGen tool and export the header.

EzCodeGen is the renamed successor to EzGen. Current version: **EzCodeGen V2.2** (previously EzGen V2.1).
Changes in the generated `SPI_ER41xx_config.h` since V2.1:
- **`_D_CODE` compatibility macro embedded inline** — the generated header now defines `_D_CODE` directly
  (SDCC / Keil C51 / IAR / AVR / XC8 / Renesas CC-RL / CA78K0R / ARM, etc.), matching the same macro
  previously provided via the shared `compiler_compat.h`. All arrays (`channel_pll_table`,
  `Initial_Reg_Array`, `TRx_Config_Array`, `TRx_ConfigEx_Array`) are declared with `_D_CODE const`
  instead of the old hard-coded `code const` (Keil C51 only) qualifier.
- **`TRx_Config_Array[]` reordered and extended** — a new **PA (Power Amplifier) setting block**
  (registers `0x1f00`~`0x1f38`, 15 entries) is generated ahead of the TX/RX sections. New array order:
  PA setting → TX setting → RX setting → Freq/Deviation/DataRate → Freq (Xtal) → Power config.

All 18 EzFirm projects in this repository have been updated to the `_D_CODE`-based config style.
