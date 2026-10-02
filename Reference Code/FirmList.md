# EzFirm 2.0 Project List

Every example exists for both platforms with the same application logic:

- N76E003 (8051)
  - Projects : `EzFirm_v2_8051/ER4100_EzFirm_B_N76_<Variant>`
  - IDE : Keil C51, `Project/*.uvproj`
  - UART : TXD P06, RXD P07, 115200 bps
  - TX / trigger button : P05 (active low)
- Nano100 (Cortex-M0, DVB board)
  - Projects : `EzFirm_v2_M0/ER4100_EzFirm_B_DVB_<Variant>`
  - IDE : Keil MDK, `Keil/*.uvprojx`
  - UART : UART1 TX PB5, RX PB4, 115200 bps
  - TX / trigger button : DVB_SW5 = PB15 (active low)

---

## Projects at a glance

Format: Variant : mode — trigger — peer board

- [`TransTxRx`](#transtxrx) : Transparent — button — `TransTxRx`
- [`TransTxRx_CCIT`](#transtxrx_ccit) : Transparent + seq + CRC-CCITT — button — `TransTxRx_CCIT`
- [`TransTxRx_CCIT_PER`](#transtxrx_ccit_per) : Transparent + seq + CRC-CCITT — button — `TransTxRx_CCIT_PER`
- [`TransTxRx_LongPkt`](#transtxrx_longpkt) : Transparent, 512-byte packet — button — `TransTxRx_LongPkt`
- [`TransWOR`](#transwor) : Wake-on-Radio RX — none (RX only) — `TransTxRx` as transmitter
- [`TransWOR_Ack_CCIT`](#transwor_ack_ccit) : WOR RX + CRC-CCITT + ACK — none (RX only) — `TransTxRx_CCIT` as transmitter
- [`802154TxRx`](#802154txrx) : IEEE 802.15.4 — button — `802154TxRx`
- [`802154TxRx_Addressing`](#802154txrx_addressing) : 802.15.4 + PAN ID / address filter — UART `1`~`4` — `802154TxRx_Addressing`
- [`SingleTone`](#singletone) : Continuous carrier — button (toggle) — spectrum analyzer
- [`RSSI_Scan`](#rssi_scan) : RSSI (CCA) scan — automatic, every 1 s — any transmitter / `SingleTone`
- [`GIO_TRBSY`](#gio_trbsy) : Transparent + GPIO0 = TR_BSY — button + UART `0`~`3` — `TransTxRx`; oscilloscope on GPIO0

Recommended EzToolkit settings files (in `Tools/`):

- `EzToolkit_settings_TransTxRx.ini` : TransTxRx, TransTxRx_CCIT, TransTxRx_CCIT_PER, SingleTone, RSSI_Scan
- `EzToolkit_settings_longPkt.ini` : TransTxRx_LongPkt (enables TX FIFO almost-empty / RX FIFO almost-full interrupts)
- `EzToolkit_settings_WOR.ini` : TransWOR, TransWOR_Ack_CCIT
- `EzToolkit_settings_802154.ini` : 802154TxRx, 802154TxRx_Addressing
- `EzToolkit_settings_GPIO0_TRBSY.ini` : GIO_TRBSY (GPIO0 in GIO mode)

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
 │    ├─ project-specific start       e.g. start RX / WOR / single tone
 │    └─ dump_rf_config()             print crystal, CapIO, rate, deviation,
 │                                    frequency, power, FIFO/INT/TX mode, syncword
 ├─ print "<Project> : init done" (or "init fail")
 └─ while(1) XTAPP_Scan()
      ├─ NIRQ low → XTAPP_IrqHdlr()   read & clear INT status, handle RX, restart RX
      └─ button / UART → build packet, TX, wait TX done, restart RX
```

If init fails (`rf_err_mode = 1`), `XTAPP_Scan()` returns immediately and nothing else runs —
check SPI wiring and power when you see `init fail`.

To change RF parameters, regenerate `RF Drivers/ER4100Api/SPI_ER41xx_config.h` with
EzToolkit; application-level parameters are the `#define`s in `RF_App.c` / `RF_App.h`.

---

## Transparent mode

### TransTxRx
Basic two-way transparent link.
- **Start-up:** RX on, `RX_TRANSPARENT`, fixed RX length `RF_BUF_SIZE` = 64 bytes.
- **TX:** press the button → sends 64 bytes `00 01 02 … 3F`, waits for TX done, prints
  `TX(64):` + hex dump and `Tx done.`, then returns to RX after the button is released.
- **RX:** on RX interrupt, reads FIFO and RSSI and prints `RX(64): rssi: -XX` + hex dump.
- **Use:** flash two boards, press the button on either one, watch the other's UART log.

### TransTxRx_CCIT
TransTxRx with a software frame format and integrity check.
- **Frame:** `[len][seq_hi][seq_lo][payload…][crc_lo][crc_hi]`
  - `len` = seq (2) + payload + CRC (2), excluding itself
  - CRC-CCITT (poly 0x8408, init 0x0000) over seq + payload
- **TX:** button → payload 59 bytes (`RF_BUF_SIZE − 5`), sequence number increments each press.
- **RX:** prints `RX seq=N`, then `CRC OK` + hex dump, or `CRC FAIL` / `RX: bad length` /
  `RX: len mismatch`.
- **Use:** two boards running TransTxRx_CCIT.

### TransTxRx_CCIT_PER
Packet Error Rate test using the TransTxRx_CCIT frame.
- **TX:** button → sends `PER_TX_COUNT` = 100 frames, `PER_TX_INTERVAL_MS` = 30 ms apart,
  printing `.` per frame.
- **RX:** for every frame prints pass / error counts, RSSI, and the running PER
  `(100 − pass) / 100` as a percentage with two decimals.
- **Use:** reset the RX board first, then press the button on the TX board once; read the
  final PER on the RX board. Change count / interval in `RF_App.h`.

### TransTxRx_LongPkt
Long packet (512 bytes, larger than the 128-byte FIFO) using FIFO thresholds.
- **Requires** the `longPkt` `.ini` (TX FIFO almost-empty and RX FIFO almost-full interrupts
  enabled, threshold 64 bytes = `RF_RX_ALMOST_FULL_THR` / `RF_TX_ALMOST_EMPTY_THR`).
- **Frame:** `[len_hi][len_lo][pkt_num][data…][crc_lo][crc_hi]`, total `RF_TX_SIZE` = 512.
- **TX:** preload a full FIFO, start TX with the total length, refill 64 bytes on every
  almost-empty interrupt until TX done (≈250 ms timeout guard, prints `TX Timeout!`).
- **RX:** drain 64 bytes on every almost-full interrupt, read the rest on RX done, check
  CRC, print `PktNum:` and the 512-byte dump on success.
- **Use:** two boards running TransTxRx_LongPkt.

### TransWOR
Wake-on-Radio receiver (low-power periodic listening).
- **Start-up:** waits 3 s for the ER4100 to stabilize, then starts WOR.
- **WOR timing:** wake-up period `WUT_PERIOD_SEL` = 1 s (10 / 50 / 100 / 500 ms, 1 / 2 / 4 s
  selectable); RX window = `WOR_CNT` × `WOR_CNT_UNIT` = 4 × 20 ms = 80 ms.
- **RX:** on a received packet, stops WOR, prints `RX(…)` + RSSI + dump, then restarts WOR.
  Other interrupts just reset the RX FIFO and restart WOR.
- **Use:** transmitter = a board running **TransTxRx** with the same settings. A packet is
  received only if it falls inside an RX window; press the TX button again if nothing shows.

### TransWOR_Ack_CCIT
WOR receiver that checks the CCIT frame and replies.
- Same WOR timing as TransWOR.
- **RX:** CRC-CCITT check (same frame as TransTxRx_CCIT). On `CRC OK`: wait 200 ms, send the
  3-byte reply `"ACK"`, print `Tx done.`, restart WOR. Bad frames are dropped silently.
- **Use:** transmitter = a board running **TransTxRx_CCIT**; its UART log shows the
  received `ACK`.

---

## IEEE 802.15.4 mode

### 802154TxRx
- **Start-up:** RX on, `RX_802154`. The RX length comes from the PHR, so the length argument
  is 0.
- **TX:** button → 64-byte frame = MHR sample pattern (`M802154_MHR_PATTERN`) + payload; the
  first payload byte is a sequence number. Hardware adds preamble, syncword, PHR and CRC.
- **RX:** `INT_ST_RX` → print frame + RSSI. `INT_ST_RXERR` → print `RX Error` with
  `Valid802154 / CrcErr / FormatErr / FormatRej` flags (register 0xA004), reset FIFO.
- **Use:** two boards running 802154TxRx.

### 802154TxRx_Addressing
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

---

## Test / tool

### SingleTone
- **Start-up:** single tone (unmodulated carrier) starts automatically at the configured
  frequency and power.
- **Button:** toggles the carrier off / on (`Stop Single Tone!` / `Start Single Tone!`).
- **Use:** frequency-offset (CapIO) calibration and output power measurement with a
  spectrum analyzer.

### RSSI_Scan
- **Start-up:** no RX packet handling; every 1 s reads RSSI in CCA mode and prints
  `rssi(dBm): -XX`.
- **Use:** check background noise / interference on the channel, or measure a nearby
  transmitter (e.g. a SingleTone board).

### GIO_TRBSY
Shows how to route an internal status signal to an ER4100 GPIO.
- **Base:** TransTxRx behaviour (button = TX 64 bytes).
- **UART commands:**
  - `0` : GPIO0 → Debug mode (TR_BSY output off)
  - `1` : GPIO0 → GIO mode, select TR_BSY (`TRx_GPIO0_Sel(1, TR_BSY)`)
  - `2` : Start RX
  - `3` : Stop RX

- **Use:** probe ER4100 GPIO0 with an oscilloscope. After `1`, GPIO0 is high while TX/RX is
  in progress — press the button to see the TX burst, or `2` to see the RX window.
