# ER4100 SDK_EZ  (v2.0)

Distribution root for the ESMT ER4100 series Sub-GHz RF transceiver SDK: the PC-side
EzToolkit, EzFirm 2.0 firmware examples for two MCU platforms, and supporting documents.

> **SDK_EZ v2.0 changes:** EzGen / EzCodeGen has been replaced by **EzToolkit**, and
> EzFirm moves to **2.0**. The previous `EzGen/` and `EzFirm_N76E003/` folders have been
> removed. See [`revise.txt`](revise.txt) for details.

---

## Contents

| Folder | Description |
|---|---|
| [`Tools/`](Tools/) | **ER41xx EzToolkit V2.0** (`ER41xx EzToolkit_V2.0_20260929.exe`) plus example `.ini` settings files (common / 802.15.4). |
| [`Reference Code/EzFirm_v2_8051/`](Reference%20Code/EzFirm_v2_8051/) | EzFirm 2.0 examples for the Nuvoton **N76E003** (8051), Keil C51 projects. |
| [`Reference Code/EzFirm_v2_M0/`](Reference%20Code/EzFirm_v2_M0/) | EzFirm 2.0 examples for the Nuvoton **Nano100** (Cortex-M0) on the DVB board, Keil MDK projects; includes `Nano100Lib`. |
| [`Document/`](Document/) | SDK_EZ User Guide (`ESAP-SPHYNX-002-SDK_EZ-User-Guide`). |
| `Datasheet/` | ER4100 datasheet. |
| `Application Note/` | Application notes. |
| [`revise.txt`](revise.txt) | Revision notes. |

---

## EzToolkit

EzToolkit is a Windows GUI development tool (.NET Framework 4.8 / WinForms) for configuring,
generating registers for, and testing ER4100 series chips. Together with the DVB evaluation
board (USB-to-UART), it covers RF parameter setup, register generation, TX/RX testing,
frequency-offset calibration and freezing parameters for mass production, so you no longer
have to look up and calculate register values by hand.

**Parameter configuration panel**
- **RF PARAM**: crystal, frequency / band, TX power (PA Config / Power LV), GFSK modulation
  and packet parameters (Preamble / Syncword / error tolerance bits)
- **FREQ TABLE**: multi-channel frequency table
- **TX FIFO MODE**: Transparent / Transparent + Presync / 802.15.4
- **GPIO**: per-pin mode / direction / pull-up/down / inversion, including interrupt events
  (`INT_ST_*`)
- **PCR**: power management (PCRMU) and SHD hardware shutdown control

**DVB Tester panel** (DVB evaluation board required)
- **Packet TX**: single-packet TX test with random payload, CRC calculation, count /
  interval / Infinite mode
- **Continuous TX**: SingleTone / Pattern (Bit_01010101, PRBS9)
- **Packet RX**: receive statistics (OK / Err), RSSI scan
- **State Ctrl**: Idle / DeepSleep / Shutdown

**REG TABLE**
- View all registers live, by category or filter
- Read / write registers on the DVB over UART (including batch **Write Multi**)
- One-click **Generate** of the C initialization arrays (`Initial_Reg_Array` + channel
  table), ready to drop into an EzFirm project

Parameters can be saved to / loaded from `.ini` files for reuse across projects and for
freezing mass-production settings.

---

## EzFirm 2.0 Examples

Each example is provided for both platforms:
`ER4100_EzFirm_B_N76_<Variant>` (N76E003) and `ER4100_EzFirm_B_DVB_<Variant>` (Nano100).

| Variant | Description |
|---|---|
| `TransTxRx` | Basic Transparent-mode TX/RX |
| `TransTxRx_CCIT` | Transparent TX/RX with sequence number and software CRC-CCITT check |
| `TransTxRx_CCIT_PER` | Packet Error Rate test based on `TransTxRx_CCIT` |
| `802154TxRx` | IEEE 802.15.4 TX/RX |
| `GIO_TRBSY` | GPIO demo: switch GPIO0 to TR_BSY output via UART commands to observe TX/RX busy timing |

Project layout: `User/` (main), `RF Drivers/` (`RF_App`, `RF_Hal`, `ER4100Api/SPI_ER41xx*`),
`Common/` (UART, delay), plus `Project/` (8051, `.uvproj`) or `Keil/` (M0, `.uvprojx`).

---

## Quick Start

1. Connect the DVB board and open **EzToolkit** from `Tools/`. Load one of the example `.ini`
   files, adjust the RF parameters, and verify the link with the DVB Tester panel.
2. In REG TABLE, click **Generate** and replace the project's
   `RF Drivers/ER4100Api/SPI_ER41xx_config.h` with the result.
3. Open the Keil project and Build All (F7):
   - N76E003: `Project/*.uvproj` (Keil C51)
   - Nano100: `Keil/*.uvprojx` (Keil MDK-ARM)
4. Flash the output via Nu-Link.

See the SDK_EZ User Guide in [`Document/`](Document/) for full instructions.
