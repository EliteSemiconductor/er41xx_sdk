# ER4100_EzFirm_802154TxRx_Addressing

## Overview

This project demonstrates **IEEE 802.15.4 address filtering** using the ER4100 RF transceiver.
The transmitter sends packets with different destination PANID/Address combinations via UART commands.
The receiver uses hardware address filtering to accept or reject incoming packets based on its configured PANID and short address.

---

## Hardware

- MCU : N76E003 (8051-core, 1T mode, 16 MHz)
- RF  : ER4100 (433 MHz, 802.15.4 mode)
- SPI : Software SPI (GPIO bit-bang)
- UART: 115200 bps, 8N1

---

## Project Structure

```
ER4100_EzFirm_802154TxRx_Addressing/
├── User/
│   └── main.c              # Entry point, UART menu
├── RF Drivers/
│   ├── RF_App.c            # TX/RX application logic, address filter demo
│   └── ER4100Api/
│       ├── SPI_ER41xx.c    # ER4100 low-level driver
│       └── SPI_ER41xx.h
├── Common/
│   └── Common.h
└── Include/
    └── XtLibSrc/           # RF library source
```

---

## Address Configuration

802.15.4 uses two levels of addressing for packet filtering:
- **PAN ID (Personal Area Network ID)**: Identifies the wireless network. A receiver only processes packets whose destination PAN ID matches its own (or is `0xFFFF` broadcast).
- **Short Address**: Identifies an individual device within the network. A receiver only accepts packets whose destination address matches its own (or is `0xFFFF` broadcast).

Defined in `RF_App.c`:

- `SRC_PANID`  = `0xABCD`  — This device's own PAN ID (written as source PAN ID in outgoing frames)
- `SRC_ADDR`   = `0x1234`  — This device's own short address (written as source address in outgoing frames)
- `DES_PANID_MATCH`    = `0xABCD`  — Destination PANID matching this device
- `DES_PANID_MISMATCH` = `0x5555`  — Destination PANID not matching this device
- `DES_PANID_ALL`      = `0xFFFF`  — Broadcast PANID (accepted by all)
- `DES_ADDR_MATCH`     = `0x1234`  — Destination address matching this device
- `DES_ADDR_MISMATCH`  = `0x5678`  — Destination address not matching this device
- `DES_ADDR_ALL`       = `0xFFFF`  — Broadcast address (accepted by all)

`XTAPP_SetDeviceAddr(SRC_PANID, SRC_ADDR)` is called during init to program the ER4100's address filter registers.

---

## UART Commands

After power-up, the following menu is printed via UART (115200 bps):

```
******************************************
* Waiting for RX or UART_CMD...
* CMD: 1=Send Match packet
* CMD: 2=Broadcast
* CMD: 3=Send PANID mismatch packet
* CMD: 4=Send 1ADDR mismatch packet
******************************************
```

- CMD `1` : DestPANID=`0xABCD` / DestADDR=`0x1234` → **Accepted** (full match)
- CMD `2` : DestPANID=`0xFFFF` / DestADDR=`0xFFFF` → **Accepted** (broadcast)
- CMD `3` : DestPANID=`0x5555` / DestADDR=`0x1234` → **Rejected** (PANID mismatch)
- CMD `4` : DestPANID=`0xABCD` / DestADDR=`0x5678` → **Rejected** (ADDR mismatch)

---

## 802.15.4 Packet Format

```
[PHR][MHR][PAYLOAD][CRC]
```

- PHR    (1 byte)  : Length of MHR + PAYLOAD + CRC (max value = 127)
- MHR    (14 bytes): MAC header — FrameCtrl, SeqNum, DestPAN, DestAddr, SrcAddr
- PAYLOAD (variable): Application data
- CRC    (2 bytes) : CRC-16 over MHR + PAYLOAD

> **Note:** MHR + PAYLOAD must not exceed **125 bytes** (802.15.4 max frame length is 127 bytes, minus 2 bytes for CRC).

> **Important:** All multi-byte fields (PANID, address, payload) must be filled in **LSB-first (little-endian)** order.
> Example: PANID `0xABCD` is stored as `0xCD, 0xAB`.

---

## TX Flow

1. User sends a UART character (`'1'` ~ `'4'`)
2. MHR is copied from `SamplePattern[]` to `gXtBuffer`
3. Destination PANID and ADDR bytes are patched at `gXtBuffer[3..6]` (LSB first)
4. Payload bytes are filled from index 15 onward
5. `XTAPP_SendData()` triggers TX via the ER4100
6. MCU waits for TX interrupt, then prints `Tx done.` and restarts RX

---

## RX Flow

1. `XTAPP_Scan()` polls `NIRQ`
2. On RX interrupt, `XTAPP_IrqHdlr()` reads the FIFO
3. Received data and RSSI (in dBm) are printed via UART
4. On RX error, error detail bits are printed (ValidFrame, CrcErr, FormatErr, FormatRej)
5. RX is re-triggered automatically

---

## UART Output Example

```
802154TxRx_Addressing : init done
******************************************
* Waiting for RX or UART_CMD...
* CMD: 1=Send Match packet
* CMD: 2=Broadcast
* CMD: 3=Send PANID mismatch packet
* CMD: 4=Send 1ADDR mismatch packet
******************************************
TX(64):
 0    41 C8 00 CD AB 34 12 02 00 00 AB AA 00 00 00 00
16    01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10
...
Tx done.
RX(64):rssi: -45
 0    41 C8 00 CD AB 34 12 ...
```

---

## Key API

- `XTAPP_Init()`                      : Initialize ER4100, set device address, start RX
- `XTAPP_SetDeviceAddr(pan_id, addr)` : Program PANID and short address filter into ER4100
- `XTAPP_SendData(buf, len)`          : Stop RX, load TX FIFO, trigger TX
- `XTAPP_IrqHdlr()`                  : Handle RX / RX error interrupt
- `XTAPP_Scan()`                      : Main poll loop: check NIRQ and UART input
