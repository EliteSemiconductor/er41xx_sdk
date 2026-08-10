# ER4100_EzFirm_802154TxRx_PER

## 概述

本专案示范使用 ER4100 RF 收发器在 N76E003（8051）MCU 上进行 IEEE 802.15.4 模式的**封包错误率（PER）量测**。

按下按键（P0.5）触发连续发送 **500 笔封包**，每笔间隔 50 ms。
ER4100 硬件 CRC 验证每笔接收封包，IRQ 处理函数分别统计通过（`IS_INT_ST_RX`）与错误（`IS_INT_ST_RXERR`）笔数。
每收到一笔即通过 UART 打印**即时 PER**，收满 500 笔后打印最终 PER 汇总。

**与 802154TxRx 的差异：**

| | 802154TxRx | 802154TxRx_PER |
|---|---|---|
| TX 触发 | P05 每次发 1 笔 | P05 连续发 500 笔 |
| TX 间隔 | 即时 | 每笔 50 ms |
| RX 处理 | 打印数据 + RSSI | 统计硬件 CRC ok/error、打印即时 PER |
| CRC 验证 | 硬件（pass/fail 事件） | 硬件 — IS_INT_ST_RX（正常）/ IS_INT_ST_RXERR（错误） |
| PER 汇报 | — | 每笔即时 % + 收满 500 笔最终汇总 |

---

## 802.15.4 封包格式

```
[PHR][MHR][PAYLOAD][CRC]
```

[各字段说明]
  PHR      : 1 字节   封包总长度（MHR + PAYLOAD + CRC）
  MHR      : 15 字节  硬编码 MAC Header（帧控制、序列号、PAN ID、地址）
  PAYLOAD  : 可变长   序列号 + 递增填充字节
  CRC      : 2 字节   CRC-16，由硬件自动附加

MHR 模板（`SamplePattern[]`）：
```c
0x41,0xC8,0x00,       // [FrameCtrl:41C8][SeqNum:00]
0xCD,0xAB,0xFF,0xFF,  // [DestPAN_ID:ABCD][DestAddr:FFFF]
0x02,0x00,0x00,0xAB,  // [SrcAddr(LE): 0x000000AAAB000002]
0xAA,0x00,0x00,0x00,
```

> **注意：** 802.15.4 RX 模式下，`TRx_RX_Trigger()` 的长度参数不影响接收，MAC 层从 PHR 自动取得实际封包长度。

---

## RX 验证与 PER 计算

每收到一笔封包，IRQ 处理函数（`XTAPP_IrqHdlr`）检查中断状态：

- `IS_INT_ST_RX`（硬件 CRC 通过）：
  - `gPerRxOk++`
  - 打印 `RX OK #<seq>  rssi: -<val>`

- `IS_INT_ST_RXERR`（硬件 CRC 失败）：
  - 打印 `RX CRC ERR  rssi: -<val>`，附带 `CrcErr` / `FmtErr` 标志
  - 调用 `TRx_RX_FIFOReset()` 恢复 RX FIFO

每次事件后：
- 打印即时 PER：`PER: <err>/<total> = <pct>%`
- 当 `(gPerRxOk + gPerRxErr) >= 500` 时：打印最终汇总并重置两个计数器

**PER 计算公式：**

```
PER(%) = gPerRxErr * 100 / (gPerRxOk + gPerRxErr)
```

> **注意：** 完全丢失的封包不产生 IRQ，不会被计入。
> 若有封包在到达接收端前丢失，最终 total 可能小于 500。

---

## 目录树

```
ER4100_EzFirm_802154TxRx_PER/
│
├── Project/                          # Keil 专案档
│   ├── mac802154TxRx_PER.uvproj     # 主专案档
│   ├── mac802154TxRx_PER.uvopt      # 项目选项
│   ├── STARTUP.A51                  # 8051 启动组语
│   ├── Nu_Link_8051_Driver.ini      # 刻录器设定
│   ├── LST/                         # 编译程序输出列表档 (*.lst, *.map)
│   └── Output/                      # 专案输出 (*.obj, *.hex, *.lnp)
│
├── Include/                          # MCU 系统头文件（只读，勿修改）
│   ├── N76E003.h                    # MCU 缓存器定义
│   ├── SFR_Macro.h                  # SFR 操作宏
│   └── Function_Define.h            # MCU 函数定义
│
├── Common/                           # 共享函式库
│   ├── Common.h                     # 专案总 include 入口：typedef、MCU header、所有模块 header
│   ├── Delay.h / Delay.c            # 延迟函式
│   └── Uart.h / Uart.c              # UART 收发函式
│
├── User/                             # 使用者应用层
│   └── main.c                       # 主程序：初始化、主循环
│
└── RF Drivers/                       # RF 驱动层
    ├── RF_Hal.h / RF_Hal.c          # RF HAL 层：脚位定义、SPI bit-bang 实作
    ├── RF_App.h / RF_App.c          # RF 应用层：初始化、收发、IRQ 处理、PER 计数
    │
    └── ER4100Api/                   # ER4100 底层 SPI API（ESMT 原厂提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、缓存器操作、FIFO 管理
        └── SPI_ER41xx_config.h      # RF 缓存器初始值配置（由 EzGen 工具产生）
```

---

## 层次架构

```
User Application
  main.c
    │  初始化、主循环
    ▼
RF Application Layer
  RF_App.c / RF_App.h
    │  PER TX 连发、IRQ 处理、硬件 CRC 结果处理
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（缓存器配置）
    │  缓存器读写、FIFO 操作、模式切换
    │  呼叫 TRx_Write / TRx_Read 进行实际 SPI 传输
    ▼
RF HAL Layer
  RF_Hal.c / RF_Hal.h
    │  SPI 时序 (Software bit-bang)、脚位定义
    ▼
Hardware
  N76E003 GPIO → ER4100 RF IC (433 MHz)
```

---

## 延伸说明

### Common.h 作为总入口
所有 `.c` 只需 `#include "Common.h"` 即可取得全部 typedef、MCU header 及模块宣告。

### SPI 实作
使用 **Software SPI (GPIO bit-bang)**，脚位定义集中于 `RF_Hal.h`：

[SPI Pin Assignment]
  NIRQ  P03  中断输入
  MOSI  P00  SPI 数据输出
  MISO  P01  SPI 数据输入
  SCK   P10  SPI 频率
  NSS   P15  SPI 片选 (CS)

### 硬件 CRC
ER4100 自动附加及验证 CRC-16。
RX 时，结果反映于中断状态：
- `IS_INT_ST_RX`    — CRC 通过，FIFO 中含有效 payload
- `IS_INT_ST_RXERR` — CRC 失败，重新启动 RX 前须调用 `TRx_RX_FIFOReset()`

### PER 计数变量

- `gPerRxOk`  : 硬件 CRC 通过的封包数
- `gPerRxErr` : 硬件 CRC 失败的封包数
- 打印最终 PER 汇总后两者均重置为 0

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "802154TxRx_PER : init done\r\n"  — 初始化成功
  "802154TxRx_PER : init fail\r\n"  — 初始化失败

### XTAPP_Scan() 初始化保护

`XTAPP_Scan()` 开头检查 `rf_err_mode`，若初始化失败（`rf_err_mode == 1`）则立即返回，防止对未初始化的 RF 芯片进行访问。

---

## Keil 包含路径（Include Paths）

```
..\Include
..\Common
..\User
..\RF Drivers
..\RF Drivers\ER4100Api
```

优先级：`..\Common` 在 `..\User` 之前，确保 `Common/Common.h` 优先于 `User/Common.h`（若存在）。
