# EzFirm 2.0 Project List

**Contents**

- [Projects at a glance](#projects-at-a-glance)
- [Common program flow](#common-program-flow)
- [Project details](#project-details)
  - [Transparent mode (6 projects)](#transparent-mode-6-projects)
  - [IEEE 802.15.4 mode (3 projects)](#ieee-802154-mode-3-projects)
  - [Power saving / wakeup timer (4 projects)](#power-saving--wakeup-timer-4-projects)
  - [Test / tool (3 projects)](#test--tool-3-projects)
- [Revision history](#revision-history)

Every example exists for both platforms with the same application logic:

- N76E003 (8051)
  - Projects : `EzFirm_v2_8051/ER4100_EzFirm_B_N76_<Variant>`
  - IDE : Keil C51, `Project/*.uvproj`
  - UART : TXD P06, RXD P07, 115200 bps
  - TX / trigger button : P05 (active low)
  - ER4100 shutdown pin (GPIO1) : P04, used by PowerSaving_Shutdown
- Nano100 (Cortex-M0, DVB board)
  - Projects : `EzFirm_v2_M0/ER4100_EzFirm_B_DVB_<Variant>`
  - IDE : Keil MDK, `Keil/*.uvprojx`
  - UART : UART1 TX PB5, RX PB4, 115200 bps
  - TX / trigger button : DVB_SW5 = PB15 (active low)
  - ER4100 shutdown pin (GPIO1) : `RF_SDN_PIN` = PB10, used by PowerSaving_Shutdown

---

## Projects at a glance

Format: Variant : function

- [`TransTxRx`](#transtxrx) : Basic two-way transparent TX / RX
- [`TransTxRx_CCIT`](#transtxrx_ccit) : Transparent TX / RX with a length + sequence number + CRC-CCITT frame
- [`TransTxRx_CCIT_PER`](#transtxrx_ccit_per) : Packet error rate (PER) test using the CCIT frame
- [`TransTxRx_LongPkt`](#transtxrx_longpkt) : 512-byte long packet TX / RX using FIFO thresholds
- [`TransWOR`](#transwor) : Wake-on-Radio (WOR) low-power receiver
- [`TransWOR_Ack_CCIT`](#transwor_ack_ccit) : WOR receiver with CRC-CCITT check and ACK reply
- [`802154TxRx`](#802154txrx) : IEEE 802.15.4 TX / RX
- [`802154TxRx_Addressing`](#802154txrx_addressing) : 802.15.4 with hardware PAN ID / short address filtering
- [`802154TxRx_CCA`](#802154txrx_cca) : 802.15.4 TX with CCA (listen before talk)
- [`SingleTone`](#singletone) : Continuous carrier for frequency / output power measurement
- [`RSSI_Scan`](#rssi_scan) : Periodic RSSI scan of the channel
- [`GIO_TRBSY`](#gio_trbsy) : Output the TX / RX busy status (TR_BSY) on ER4100 GPIO0
- [`PowerSaving`](#powersaving) : Enter / leave deep sleep by command
- [`PowerSaving_Shutdown`](#powersaving_shutdown) : Shut down the ER4100 by its GPIO1 pin
- [`PowerSaving_WUT`](#powersaving_wut) : Deep sleep with periodic wake-up by the wakeup timer
- [`WakeupTimer(WUT)`](#wakeuptimerwut) : Wakeup timer in active mode, period / mode set by UART

Recommended EzToolkit settings files (in `Tools/`):

- `EzToolkit_settings_TransTxRx.ini` : TransTxRx, TransTxRx_CCIT, TransTxRx_CCIT_PER, SingleTone, RSSI_Scan
- `EzToolkit_settings_longPkt.ini` : TransTxRx_LongPkt (enables TX FIFO almost-empty / RX FIFO almost-full interrupts)
- `EzToolkit_settings_WOR.ini` : TransWOR, TransWOR_Ack_CCIT
- `EzToolkit_settings_802154.ini` : 802154TxRx, 802154TxRx_Addressing
- `EzToolkit_settings_802154_TXCCA.ini` : 802154TxRx_CCA (same as 802154 with the `INT_ST_TXERR` interrupt enabled)
- `EzToolkit_settings_GPIO0_TRBSY.ini` : GIO_TRBSY (GPIO0 in GIO mode)
- `EzToolkit_settings_PowerSaving.ini` : PowerSaving, PowerSaving_Shutdown, PowerSaving_WUT (RX / TX interrupts off, `INT_ST_WAKEUP` on, GPIO1 no pull)
- `EzToolkit_settings_WakeupTimer.ini` : WakeupTimer(WUT) (RX / TX interrupts off, `INT_ST_TMRTHD` on)

Both boards of a pair must use the **same** RF settings (frequency, data rate, syncword, …).

---

## Common program flow

All projects share the same skeleton (`User/main.c` → `RF Drivers/RF_App.c`):

```
main()
 ├─ UART init (115200) + GPIO init
 ├─ XTAPP_Init()
 │    ├─ TRx_IoConfig()               SPI / NIRQ pins
 │    ├─ XTAPP_Strobe()               read chip ID 0xF100, retry up to 1 s
 │    ├─ TRx_SW_Reset() + TRx_Init()  load SPI_ER41xx_config.h (from EzToolkit)
 │    ├─ project-specific start       e.g. start RX / WOR / single tone / WUT
 │    └─ dump_rf_config()             print crystal, CapIO, rate, deviation,
 │                                    frequency, power, FIFO/INT/TX mode, syncword
 ├─ init OK   → print "<Project> : init done" + usage / command menu
 ├─ init fail → print "<Project> : init fail" only
 └─ while(1) XTAPP_Scan()
      ├─ NIRQ low → XTAPP_IrqHdlr()   read & clear INT status, handle RX, restart RX
      └─ button / UART → build packet, TX, wait TX done, restart RX
```

If init fails (`rf_err_mode = 1`), `XTAPP_Scan()` returns immediately and nothing else runs —
check SPI wiring and power when you see `init fail`.

To change RF parameters, regenerate `RF Drivers/ER4100Api/SPI_ER41xx_config.h` with
EzToolkit; application-level parameters are the `#define`s in `RF_App.c` / `RF_App.h`.

---

## Project details

Start-up behaviour, operation and peer board of each project, grouped by category.

### Transparent mode (6 projects)

#### TransTxRx
Basic two-way transparent link.
- **Start-up:** RX on, `RX_TRANSPARENT`, fixed RX length `RF_BUF_SIZE` = 64 bytes.
- **TX:** press the button → sends 64 bytes `00 01 02 … 3F`, waits for TX done, prints
  `TX(64):` + hex dump and `Tx done.`, then returns to RX after the button is released.
- **RX:** on RX interrupt, reads FIFO and RSSI and prints `RX(64): rssi: -XX` + hex dump.
- **Use:** flash two boards, press the button on either one, watch the other's UART log.

#### TransTxRx_CCIT
TransTxRx with a software frame format and integrity check.
- **Frame:** `[len][seq_hi][seq_lo][payload…][crc_lo][crc_hi]`
  - `len` = seq (2) + payload + CRC (2), excluding itself
  - CRC-CCITT (poly 0x8408, init 0x0000) over seq + payload
- **TX:** button → payload 59 bytes (`RF_BUF_SIZE − 5`), sequence number increments each press.
- **RX:** prints `RX seq=N`, then `CRC OK` + hex dump, or `CRC FAIL` / `RX: bad length` /
  `RX: len mismatch`.
- **Use:** two boards running TransTxRx_CCIT.

#### TransTxRx_CCIT_PER
Packet Error Rate test using the TransTxRx_CCIT frame.
- **TX:** button → sends `PER_TX_COUNT` = 100 frames, `PER_TX_INTERVAL_MS` = 30 ms apart,
  printing `.` per frame.
- **RX:** for every frame prints pass / error counts, RSSI, and the running PER
  `(100 − pass) / 100` as a percentage with two decimals.
- **Use:** reset the RX board first, then press the button on the TX board once; read the
  final PER on the RX board. Change count / interval in `RF_App.h`.

#### TransTxRx_LongPkt
Long packet (512 bytes, larger than the 128-byte FIFO) using FIFO thresholds.
- **Requires** the `longPkt` `.ini` (TX FIFO almost-empty and RX FIFO almost-full interrupts
  enabled, threshold 64 bytes = `RF_RX_ALMOST_FULL_THR` / `RF_TX_ALMOST_EMPTY_THR`).
- **Frame:** `[len_hi][len_lo][pkt_num][data…][crc_lo][crc_hi]`, total `RF_TX_SIZE` = 512.
- **TX:** preload a full FIFO, start TX with the total length, refill 64 bytes on every
  almost-empty interrupt until TX done (≈250 ms timeout guard, prints `TX Timeout!`).
- **RX:** drain 64 bytes on every almost-full interrupt, read the rest on RX done, check
  CRC, print `PktNum:` and the 512-byte dump on success.
- **Use:** two boards running TransTxRx_LongPkt.

#### TransWOR
Wake-on-Radio receiver (low-power periodic listening).
- **Start-up:** waits 3 s for the ER4100 to stabilize, then starts WOR.
- **WOR timing:** wake-up period `WUT_PERIOD_SEL` = 1 s (10 / 50 / 100 / 500 ms, 1 / 2 / 4 s
  selectable); RX window = `WOR_CNT` × `WOR_CNT_UNIT` = 4 × 20 ms = 80 ms.
- **RX:** on a received packet, stops WOR, prints `RX(…)` + RSSI + dump, then restarts WOR.
  Other interrupts just reset the RX FIFO and restart WOR.
- **Use:** transmitter = a board running **TransTxRx** with the same settings. A packet is
  received only if it falls inside an RX window; press the TX button again if nothing shows.

#### TransWOR_Ack_CCIT
WOR receiver that checks the CCIT frame and replies.
- Same WOR timing as TransWOR.
- **RX:** CRC-CCITT check (same frame as TransTxRx_CCIT). On `CRC OK`: wait 200 ms, send the
  3-byte reply `"ACK"`, print `Tx done.`, restart WOR. Bad frames are dropped silently.
- **Use:** transmitter = a board running **TransTxRx_CCIT**; its UART log shows the
  received `ACK`.

---

### IEEE 802.15.4 mode (3 projects)

#### 802154TxRx
- **Start-up:** RX on, `RX_802154`. The RX length comes from the PHR, so the length argument
  is 0.
- **TX:** button → 64-byte frame = MHR sample pattern (`M802154_MHR_PATTERN`) + payload; the
  first payload byte is a sequence number. Hardware adds preamble, syncword, PHR and CRC.
- **RX:** `INT_ST_RX` → print frame + RSSI. `INT_ST_RXERR` → print `RX Error` with
  `Valid802154 / CrcErr / FormatErr / FormatRej` flags (register 0xA004), reset FIFO.
- **Use:** two boards running 802154TxRx.

#### 802154TxRx_Addressing
Hardware PAN ID / short address filtering.
- **Start-up:** this device is `SRC_PANID` = 0xABCD, `SRC_ADDR` = 0x1234 (written to 0xA070 /
  0xA074, filter enabled in 0xA00C). A command menu is printed.
- **TX via UART command:**
  - `1` : destination 0xABCD / 0x1234 (match) → received
  - `2` : destination 0xFFFF / 0xFFFF (broadcast) → received
  - `3` : destination 0x5555 / 0x1234 (PAN ID mismatch) → rejected (`RX Error`, `FormatRej`)
  - `4` : destination 0xABCD / 0x5678 (address mismatch) → rejected (`RX Error`, `FormatRej`)

- **Use:** two boards; type `1`–`4` in one board's terminal, watch the other's log.
  Addresses are set in `RF_App.h`.

#### 802154TxRx_CCA
802154TxRx with CCA (Clear Channel Assessment, listen before talk) before every TX.
- **Start-up:** same as 802154TxRx, plus `TRx_CCA_Config(0xFFF − 8 × CCA_THRESHOLD)` writes
  the CCA threshold (register 0xA040). `CCA_THRESHOLD` = 80 means −80 dBm; the value is
  printed as `CCA Fail Threshold(dBm) : -80`.
- **Interrupt:** `INT_EN_REG` must enable `INT_ST_TXERR` (0x0F instead of 0x07), so a CCA
  failure raises NIRQ. `EzToolkit_settings_802154_TXCCA.ini` already enables it.
- **TX:** button → same 64-byte frame as 802154TxRx, sent with `TRx_TX_Trigger(len, 1)`
  (CCA enabled). The chip measures the channel energy first:
  - below the threshold → transmits, `INT_ST_TX` → prints
    `Done. Ambient-RSSI(dBm): -XX` and the frame
  - above the threshold → does not transmit, `INT_ST_TXERR` → prints
    `Fail. Ambient-RSSI(dBm): -XX`

  The ambient RSSI is read from register 0xB1FC (`TRx_GetRSSI_Data()`). RX restarts after
  the button is released.
- **Use:** a second board running 802154TxRx receives the frames. To see CCA block TX,
  run SingleTone on a third board at the same frequency near the CCA board.
  Change the threshold with `CCA_THRESHOLD` in `RF_App.c`.
- **Note:** `TRx_GetRSSI_CCA()` is the RSSI scan API used by RSSI_Scan; CCA TX does not
  call it.

---

### Power saving / wakeup timer (4 projects)

These projects do not use RX / TX. Measure the ER4100 supply current with a current meter
to compare the power modes.

#### PowerSaving
Deep sleep entered and left by command.
- **Start-up:** the chip is initialized and stays in active mode; the usage is printed.
- **Button:**
  - active → `TRx_PowerDownMode()` puts the ER4100 into deep sleep, prints `>>DeepSleep`
  - deep sleep → `TRx_NOP()` (any SPI command) wakes the chip up; after the wake-up delay
    the `INT_ST_WAKEUP` interrupt prints `>>Wakeup`
- **Use:** compare the current in active mode and in deep sleep.

#### PowerSaving_Shutdown
Lowest-power mode: the ER4100 is shut down by its GPIO1 pin.
- **Hardware:** ER4100 GPIO1 (default mode = shutdown input, low = shutdown) is driven by the
  MCU — 8051 P04, M0 `RF_SDN_PIN` (PB10). The pin stays high in normal operation.
- **Start-up:** after `TRx_Init()`, `TRx_PCRMU_ShutDown_Enable()` enables the shutdown
  function in PCR. Without this step, driving GPIO1 low has no effect.
- **Button:**
  - active → pin low, the ER4100 enters shutdown, prints `>>Shutdown`
  - shutdown → pin high, the ER4100 boots up again. Registers are not retained, so
    `XTAPP_ChipInit()` repeats strobe / reset / init / shutdown enable, then prints
    `>>Wakeup` (or `>>Wakeup fail`)
- **Use:** measure the shutdown current.

#### PowerSaving_WUT
Deep sleep with automatic wake-up by the wakeup timer (WUT).
- **Start-up:** prints `Enter deep sleep and wakeup after WUT timing...`, starts the WUT in
  one-shot mode with `WUT_PERIOD_SEL` = 2 s, and enters deep sleep.
- **Wake-up:** `INT_ST_WAKEUP` → prints `Wakeup and wait for 1sec...`, stays active for 1 s,
  prints `Enter deep sleep again`, restarts the one-shot WUT and enters deep sleep again.
  The cycle repeats.
- **WUT period:** register value = period (ms) × 32768 / 1000. Macros are provided for
  10 / 50 / 100 / 500 ms and 1 / 2 / 4 s; change `WUT_PERIOD_SEL` in `RF_App.c`.
- **`INT_ST_TMRTHD`** is optional here. It is reported only when its INT mask is enabled;
  then a timer expiry raises a TMRTHD event first, followed by a WAKEUP event.
- **Use:** observe the periodic current profile (1 s active / 2 s deep sleep).

#### WakeupTimer(WUT)
Wakeup timer running while the chip stays in active mode, controlled by UART.
- **Start-up:** stops any running WUT, enables `INT_ST_TMRTHD` in the INT mask and prints the
  command menu. The timer starts after the first command.
- **UART commands:**
  - `0` ~ `4` : periodic mode, 10 ms / 50 ms / 100 ms / 500 ms / 1 s
  - `5` ~ `7` : one-shot mode, 1 s / 2 s / 4 s
  - `p` : stop the timer and print the menu again

  Every period / mode command restarts the timer, resets the event counter and prints
  e.g. `WUT starting : period = 1s, mode = Periodic`.
- **Event:** each timer expiry (`INT_ST_TMRTHD`) prints
  `WUT event : N, key in 'p' to stop periodic`. In one-shot mode it prints `WUT event : 1`
  followed by `One-shot done`.
- **Use:** check WUT timing with a timestamped UART log, or reuse
  `XTAPP_WUT_Config()` / `XTAPP_WUT_Stop()` in your own application.

---

### Test / tool (3 projects)

#### SingleTone
- **Start-up:** single tone (unmodulated carrier) starts automatically at the configured
  frequency and power.
- **Button:** toggles the carrier off / on (`Stop Single Tone!` / `Start Single Tone!`).
- **Use:** frequency-offset (CapIO) calibration and output power measurement with a
  spectrum analyzer.

#### RSSI_Scan
- **Start-up:** turns on RX in non-packet mode to collect the ambient signal; every 1 s reads
  the RSSI and prints `rssi(dBm): -XX`.
- **Use:** check background noise / interference on the channel, or measure a nearby
  transmitter (e.g. a SingleTone board).

#### GIO_TRBSY
Shows how to route an internal status signal to an ER4100 GPIO.
- **Base:** TransTxRx behaviour (button = TX 64 bytes).
- **UART commands:**
  - `0` : GPIO0 → Debug mode (TR_BSY output off)
  - `1` : GPIO0 → GIO mode, select TR_BSY (`TRx_GPIO0_Sel(1, TR_BSY)`)
  - `2` : Start RX
  - `3` : Stop RX

- **Use:** a second board running TransTxRx can be used as the peer. Probe ER4100 GPIO0 with
  an oscilloscope. After `1`, GPIO0 is high while TX/RX is
  in progress — press the button to see the TX burst, or `2` to see the RX window.

---

## Revision history

- 2026-10-07
  - Add PowerSaving, PowerSaving_Shutdown, PowerSaving_WUT, WakeupTimer(WUT)
  - Usage / command menu is printed only when init succeeds
- 2026-09-30
  - Initial release (EzFirm 2.0)
