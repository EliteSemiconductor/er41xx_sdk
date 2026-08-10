# ER4100_EzFirm_802154TxRx_Addressing

## 概述

本专案示范使用 ER4100 RF 收发器进行 **IEEE 802.15.4 地址过滤** 功能验证。
发送端透过 UART 指令发送不同目标 PANID / 地址组合的封包，
接收端则根据自身设定的 PANID 与短地址决定是否接收该封包。

---

## 硬件规格

- MCU : N76E003（8051 核心，1T 模式，16 MHz）
- RF  : ER4100（433 MHz，802.15.4 模式）
- SPI : 软体 SPI（GPIO bit-bang）
- UART: 115200 bps，8N1

---

## 专案结构

```
ER4100_EzFirm_802154TxRx_Addressing/
├── User/
│   └── main.c              # 程式入口，UART 选单
├── RF Drivers/
│   ├── RF_App.c            # TX/RX 应用层，地址过滤示范
│   └── ER4100Api/
│       ├── SPI_ER41xx.c    # ER4100 底层驱动
│       └── SPI_ER41xx.h
├── Common/
│   └── Common.h
└── Include/
    └── XtLibSrc/           # RF 库原始码
```

---

## 地址配置

802.15.4 使用两层地址进行封包过滤：
- **PAN ID（Personal Area Network ID）**：网络识别码，用于区分不同的无线网络。只有 PAN ID 相符的装置才会处理该封包（或目标为 `0xFFFF` 广播）。
- **短地址（Short Address）**：网络内个别装置的识别码。接收端会检查目标地址是否与自身地址相符（或为 `0xFFFF` 广播）。

定义于 `RF_App.c`：

- `SRC_PANID`  = `0xABCD`  — 本装置的 PAN ID（发送封包时填入来源 PAN ID）
- `SRC_ADDR`   = `0x1234`  — 本装置的短地址（发送封包时填入来源地址）
- `DES_PANID_MATCH`    = `0xABCD`  — 目标 PANID 与本装置相符
- `DES_PANID_MISMATCH` = `0x5555`  — 目标 PANID 与本装置不符
- `DES_PANID_ALL`      = `0xFFFF`  — 广播 PANID（所有装置接受）
- `DES_ADDR_MATCH`     = `0x1234`  — 目标地址与本装置相符
- `DES_ADDR_MISMATCH`  = `0x5678`  — 目标地址与本装置不符
- `DES_ADDR_ALL`       = `0xFFFF`  — 广播地址（所有装置接受）

初始化时呼叫 `XTAPP_SetDeviceAddr(SRC_PANID, SRC_ADDR)` 将地址写入 ER4100 过滤寄存器。

---

## UART 指令

上电后透过 UART（115200 bps）印出选单：

```
******************************************
* Waiting for RX or UART_CMD...
* CMD: 1=Send Match packet
* CMD: 2=Broadcast
* CMD: 3=Send PANID mismatch packet
* CMD: 4=Send 1ADDR mismatch packet
******************************************
```

- 指令 `1` : DestPANID=`0xABCD` / DestADDR=`0x1234` → **接收**（完全相符）
- 指令 `2` : DestPANID=`0xFFFF` / DestADDR=`0xFFFF` → **接收**（广播）
- 指令 `3` : DestPANID=`0x5555` / DestADDR=`0x1234` → **拒绝**（PANID 不符）
- 指令 `4` : DestPANID=`0xABCD` / DestADDR=`0x5678` → **拒绝**（地址不符）

---

## 802.15.4 封包格式

```
[PHR][MHR][PAYLOAD][CRC]
```

- PHR    （1 byte）  ：MHR + PAYLOAD + CRC 的总长度（最大值 = 127）
- MHR    （14 bytes）：MAC 表头 — FrameCtrl、SeqNum、DestPAN、DestAddr、SrcAddr
- PAYLOAD（可变）    ：应用数据
- CRC    （2 bytes） ：MHR + PAYLOAD 的 CRC-16

> **注意：** MHR + PAYLOAD 总长度不可超过 **125 bytes**（802.15.4 最大帧长 127 bytes，减去 CRC 2 bytes）。

> **重要：** 802.15.4 帧中所有多字节字段（PANID、地址、Payload）均须以 **LSB 优先（小端序）** 方式填入。
> 范例：PANID `0xABCD` 存放为 `0xCD, 0xAB`。

---

## TX 流程

1. 使用者透过 UART 输入指令字元（`'1'` ～ `'4'`）
2. 将 `SamplePattern[]` 中的 MHR 复制到 `gXtBuffer`
3. 在 `gXtBuffer[3..6]` 以 LSB 优先方式填入目标 PANID 与地址
4. 从索引 15 开始填入 Payload 数据
5. 呼叫 `XTAPP_SendData()` 触发 ER4100 TX
6. 等待 TX 中断完成后印出 `Tx done.`，并重新启动 RX

---

## RX 流程

1. `XTAPP_Scan()` 轮询 `NIRQ`
2. RX 中断发生时，`XTAPP_IrqHdlr()` 读取 FIFO 数据
3. 透过 UART 印出接收数据与 RSSI（dBm）
4. 发生 RX 错误时印出错误详情（ValidFrame、CrcErr、FormatErr、FormatRej）
5. 自动重新启动 RX

---

## UART 输出范例

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

## 主要 API

- `XTAPP_Init()`                      ：初始化 ER4100，设定装置地址，启动 RX
- `XTAPP_SetDeviceAddr(pan_id, addr)` ：将 PANID 与短地址写入 ER4100 过滤寄存器
- `XTAPP_SendData(buf, len)`          ：停止 RX，写入 TX FIFO，触发发送
- `XTAPP_IrqHdlr()`                  ：处理 RX / RX 错误中断
- `XTAPP_Scan()`                      ：主轮询循环：检查 NIRQ 与 UART 输入
