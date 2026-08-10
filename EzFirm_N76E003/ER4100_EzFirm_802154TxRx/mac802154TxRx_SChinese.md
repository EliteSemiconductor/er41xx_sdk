# ER4100_EzFirm_802154TxRx

## 概述

本专案示范使用 ER4100 RF 收发器在 N76E003（8051）MCU 上进行 **IEEE 802.15.4 模式双向收发**。

设备持续开启 RX 监听；按下按键（P0.5）可手动触发 TX，发送符合 802.15.4 格式的封包（含 PHR、MHR、Payload、CRC）。收到封包后通过 UART 输出数据及 RSSI。

---

## 802.15.4 封包格式

```
[PHR][MHR][PAYLOAD][CRC]
```

[字段说明]
  PHR      : 1 byte  封包总长度（MHR + PAYLOAD + CRC 之和）
  MHR      : 15 bytes 硬编码 MAC Header（FrameCtrl、SeqNum、PAN ID、地址）
  PAYLOAD  : 可变长度 有效载荷（RF_TX_SIZE - MHR - CRC），**必须以 LSB（低位字节）优先填入**
  CRC      : 2 bytes  CRC-16，由硬件自动计算并附加

MHR 样板（`SamplePattern[]`）：
```c
0x41,0xC8,0x00,       // [FrameCtrl:41C8][SeqNum:00]
0xCD,0xAB,0xFF,0xFF,  // [DestPAN_ID:ABCD][DestAddr:FFFF]
0x02,0x00,0x00,0xAB,  // [SrcAddr(LE): 0x000000AAAB000002]
0xAA,0x00,0x00,0x00,
```

> **注意：** 802.15.4 模式接收时，`TRx_RX_Trigger()` 的长度参数不影响接收，MAC 层从 PHR 自动取得封包长度。

---

## 目录树

```
ER4100_EzFirm_802154TxRx/
│
├── Project/                          # Keil 专案档
│   ├── mac802154TxRx.uvproj         # 主专案档
│   ├── mac802154TxRx.uvopt          # 专案选项
│   ├── STARTUP.A51                  # 8051 启动汇编
│   ├── Nu_Link_8051_Driver.ini      # 烧录器设定
│   ├── LST/                         # 编译器输出列表档 (*.lst, *.map)
│   └── Output/                      # 专案输出 (*.obj, *.hex, *.lnp)
│
├── Include/                          # MCU 系统头文件（只读，勿修改）
│   ├── N76E003.h                    # MCU 寄存器定义
│   ├── SFR_Macro.h                  # SFR 操作宏
│   └── Function_Define.h            # MCU 函数定义
│
├── Common/                           # 共享函数库
│   ├── Common.h                     # 专案总 include 入口：typedef、MCU header、所有模块 header
│   ├── Delay.h / Delay.c            # 延迟函数
│   └── Uart.h / Uart.c              # UART 收发函数
│
├── User/                             # 用户应用层
│   └── main.c                       # 主程序：初始化、主循环
│
└── RF Drivers/                       # RF 驱动层
    ├── RF_Hal.h / RF_Hal.c          # RF HAL 层：引脚定义、SPI bit-bang 实现
    ├── RF_App.h / RF_App.c          # RF 应用层：初始化、收发、IRQ 处理
    │
    └── ER4100Api/                   # ER4100 底层 SPI API（ESMT 原厂提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、寄存器操作、FIFO 管理
        └── SPI_ER41xx_config.h      # RF 寄存器初始值配置（由 EzGen 工具生成）
```

---

## 层次架构

```
用户应用层
  main.c
    │  初始化、主循环
    ▼
RF 应用层
  RF_App.c / RF_App.h
    │  封包收发、IRQ 处理、RX buffer 管理
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（寄存器配置）
    │  寄存器读写、FIFO 操作、模式切换
    │  调用 TRx_Write / TRx_Read 进行实际 SPI 传输
    ▼
RF HAL 层
  RF_Hal.c / RF_Hal.h
    │  SPI 时序（软件 bit-bang）、引脚定义
    ▼
硬件
  N76E003 GPIO → ER4100 RF IC (433 MHz)
```

---

## 延伸说明

### Common.h 作为总入口
所有 `.c` 只需 `#include "Common.h"` 即可取得全部 typedef、MCU header 及模块声明。

### SPI 实现
使用 **Software SPI (GPIO bit-bang)**，引脚定义集中于 `RF_Hal.h`：

[SPI 引脚定义]
  NIRQ  P03  中断输入
  MOSI  P00  SPI 数据输出
  MISO  P01  SPI 数据输入
  SCK   P10  SPI 时钟
  NSS   P15  SPI 片选 (CS)

### TX 流程

按下 P05 后：
1. 将 `SamplePattern[]`（MHR）复制到 `gXtBuffer`
2. 以递增序号 `gu8TxSeqNum++` 填入 Payload 第一个 Byte
3. Payload 其余 Bytes 依序填入索引值（`i`）
4. 调用 `XTAPP_SendData()` 触发 TX
5. 等待 NIRQ 表示 TX 完成（`while(NIRQ_Value)`）
6. 清除中断标志，重新启动 RX

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "802154TxRx : init done\r\n"  — 初始化成功
  "802154TxRx : init fail\r\n"  — 初始化失败

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

优先级：`..\Common` 在 `..\User` 之前，确保 `Common/Common.h` 优先被引用。
