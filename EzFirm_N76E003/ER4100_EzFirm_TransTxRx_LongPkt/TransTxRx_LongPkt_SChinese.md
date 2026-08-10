# ER4100_EzFirm_TransTxRx_LongPkt

## 概述

本专案示范使用 ER4100 RF 收发器在 N76E003（8051）MCU 上进行**长包透明模式收发**（Long Packet Transparent Mode，最大 2047 字节）。

标准透明模式受限于 FIFO 大小（128 字节），长包模式透过分段搬移 FIFO 突破此限制。接收端在 FIFO almost full 时将中间数据移出，发送端分批补填 FIFO 直到整包传完。封包内嵌 CRC-16/CCITT-Kermit 供接收端验证完整性。

---

## 目录树

```
ER4100_EzFirm_TransTxRx_LongPkt/
│
├── Project/                              # Keil 专案档
│   ├── TransTxRx_LongPkt.uvproj         # 主专案档
│   ├── TransTxRx_LongPkt.uvopt          # 专案选项
│   ├── STARTUP.A51                      # 8051 启动组语
│   ├── Nu_Link_8051_Driver.ini          # 烧录器设定
│   ├── LST/                             # 编译器输出列表档 (*.lst, *.map)
│   └── Output/                          # 专案输出 (*.obj, *.hex, *.lnp)
│
├── Include/                              # MCU 系统头文件（只读，勿修改）
│   ├── N76E003.h                        # MCU 寄存器定义
│   ├── SFR_Macro.h                      # SFR 操作宏
│   └── Function_Define.h               # MCU 函数定义
│
├── Common/                               # 共享函式库
│   ├── Common.h / Common.c             # 专案总 include 入口：typedef、MCU header、所有模块 header
│   ├── Delay.h / Delay.c               # 延迟函式
│   └── Uart.h / Uart.c                 # UART 收发函式
│
├── User/                                 # 使用者应用层
│   └── main.c                          # 主程序：初始化、主循环
│
├── RF Drivers/                           # RF 驱动层
│   ├── RF_Hal.h / RF_Hal.c             # RF HAL 层：脚位定义、SPI bit-bang 实作
│   ├── RF_App.h / RF_App.c             # RF 应用层：初始化、长包收发、IRQ 处理、CRC 验证
│   │
│   └── ER4100Api/                       # ER4100 底层 SPI API（ESMT 原厂提供）
│       ├── SPI_ER41xx.h / .c           # SPI 指令集、寄存器操作、FIFO 管理
│       └── SPI_ER41xx_config.h         # RF 寄存器初始值配置（由 EzGen 工具产生，当前使用）
│
└── EzGen_settings.ini                    # EzGen 工具配置档
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
    │  长包收发、IRQ 处理、RX buffer 管理、CRC 验证
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（寄存器配置）
    │  寄存器读写、FIFO 操作、模式切换
    │  调用 TRx_Write / TRx_Read 进行实际 SPI 传输
    ▼
RF HAL Layer
  RF_Hal.c / RF_Hal.h
    │  SPI 时序 (Software bit-bang)、脚位定义
    ▼
Hardware
  N76E003 GPIO → ER4100 RF IC (433 MHz)
```

---

## 封包结构

长包模式下封包格式（共 `RF_BUF_SIZE` 字节，预设 512 字节）：

[封包结构]
  `[0]`            : Length high byte  (`RF_BUF_SIZE >> 8`)
  `[1]`            : Length low byte   (`RF_BUF_SIZE & 0xFF`)
  `[2]`            : Packet number     (每次 TX 自动递增)
  `[3]` ~ `[N-3]` : Payload data      (有效载荷)
  `[N-2]`          : CRC16 low byte   (CRC-16/CCITT-Kermit，低字节先)
  `[N-1]`          : CRC16 high byte  (CRC-16/CCITT-Kermit，高字节后)

> CRC 计算范围：`buf[1]` 至 `buf[N-4]`（共 `N-3` 字节，N = RF_BUF_SIZE）

---

## RF_App.c 函数说明

[RF_App.c 函数说明]
  `XTAPP_Strobe()`               : 读取 Chip ID，确认 RF 芯片存在
  `XTAPP_Init()`                 : 初始化 RF 收发器，设定 RX 模式并启动接收
  `XTAPP_Scan()`                 : 主扫描循环；初始化失败时立即返回；NIRQ 触发 RX 处理，P05 按键触发 TX
  `XTAPP_IrqHdlr()`             : IRQ 处理：调用长包接收检查，CRC 通过时 dump buffer
  `XTAPP_LongPktReceiveCheck()` : 长包接收状态机：FIFO almost full 时搬移中间数据，EOP 时读取剩余并验证 CRC
  `XTAPP_CheckRxCRC()`          : 从封包 header 取得长度，重算 CRC 并与封包尾端比对，输出结果至 UART
  `XTAPP_LongPktTransmit()`     : 长包发送：先填满 FIFO 后触发 TX，循环补填直到整包传完
  `crc16CcitKermit()`            : CRC-16/CCITT-Kermit 算法（多项式 0x8408，初始值 0x0000）
  `dump_buffer()`                : 将 buffer 内容以 HEX 格式输出至 UART（TX/RX 均适用）
  `dump_reg()`                   : 读取单一寄存器并以 HEX 格式输出至 UART

### XTAPP_LongPktReceiveCheck 回传值

[XTAPP_LongPktReceiveCheck 回传值]
  `0` : 封包尚未接收完整（中间数据搬移）
  `1` : 封包接收完整，CRC 验证通过
  `2` : 封包接收完整，CRC 验证失败或长度无效

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
  SCK   P10  SPI 时钟
  NSS   P15  SPI 片选 (CS)

### RF_BUF_SIZE 与 RX 触发长度
`RF_App.c` 中的 `RF_BUF_SIZE` 为传入 `TRx_RX_Trigger()` 的期望接收长度：
- **802.15.4 模式**：此值不影响接收，MAC 层从 PHR 自动取得封包长度
- **Standard transparent 模式**：须填写实际期望长度（1 ~ 128 字节）
- **Long packet transparent 模式**：须填写实际期望长度（1 ~ 2047 字节）

### 长包收发门限

[长包收发门限]
  `RF_RX_ALMOST_FULL_THR`  : 64  (对应寄存器 0xA0A4 bit23:16，RX FIFO almost full)
  `RF_TX_ALMOST_EMPTY_THR` : 64  (对应寄存器 0xA0A4 bit7:0，TX FIFO almost empty)

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "TransTxRx_LongPkt : init done\r\n"  — 初始化成功
  "TransTxRx_LongPkt : init fail\r\n"  — 初始化失败

---

## Keil 包含路径（Include Paths）

```
..\Include
..\Common
..\User
..\RF Drivers
..\RF Drivers\ER4100Api
```

优先级：`..\Common` 在 `..\User` 之前，确保 `Common/Common.h` 优先被引用。
