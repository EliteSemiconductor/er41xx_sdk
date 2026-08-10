# ER4100_EzFirm_RSSI_Scan

## 概述

本专案示范使用 ER4100 RF 收发器的 **CCA（Clear Channel Assessment）功能** 在 N76E003（8051）MCU 上进行环境 RSSI 量测。

每秒执行一次 `XTAPP_Scan()`，调用 `TRx_GetRSSI_CCA()` 对当前信道能量进行采样，并通过 UART 输出 dBm 数值。本专案不含封包收发（RX/TX），仅量测背景 RF 能量。

RSSI 转换公式：

```
RSSI_dBm = -((4095 - raw) / 8)   [dBm]
```

---

## 测试环境建议

若要观察 RSSI 的变化，需要第二块 ER4100 开发板作为 RF 来源。

**建议使用 `ER4100_EzFirm_SingleTone` 作为 RF 信号来源。**

[测试配置]
  板 A  RSSI_Scan    → 每 1 秒量测一次环境 RSSI，并输出结果
  板 B  SingleTone   → 在相同频率上持续发射连续载波

板 B 发射时，板 A 的 RSSI 读数会显著上升。调整两板之间的距离可产生明显的 RSSI 变化，方便验证量测范围与灵敏度，无需建立封包收发系统。

> 板 B 必须设定与板 A **相同的频道/频率**。两个专案预设使用相同的 `SPI_ER41xx_config.h` 暂存器配置。

---

## 目录树

```
ER4100_EzFirm_RSSI_Scan/
│
├── Project/                          # Keil 专案档
│   ├── RSSI_Scan.uvproj             # 主专案档
│   ├── RSSI_Scan.uvopt              # 专案选项
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
    ├── RF_App.h / RF_App.c          # RF 应用层：初始化、CCA RSSI 扫描循环
    │
    └── ER4100Api/                   # ER4100 底层 SPI API（ESMT 原厂提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、寄存器操作、CCA/RSSI 函数
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
    │  CCA RSSI 量测、UART 输出、1 秒轮询循环
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（寄存器配置）
    │  TRx_GetRSSI_CCA()、TRx_CCA_Config()、寄存器读写
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

### 关键 API 函数

[关键 API 函数]
  `TRx_CCA_Config(u16CCA_RSSITHD)` : 设定 CCA RSSI 阈值（如 TX_CCA_RSSI_80dBmTHD）
  `TRx_GetRSSI_CCA(u16RSSI_dBm)`   : 触发 CCA 量测，返回原始 16-bit RSSI 值

### RSSI 输出格式

每次量测通过 UART 输出一行：

  "rssi(dBm): -XX\r\n"

其中 `XX = (4095 - raw) / 8`。raw 值越大 → dBm 越低（信号越弱）。

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "RSSI_Scan : init done\r\n"  — 初始化成功
  "RSSI_Scan : init fail\r\n"  — 初始化失败

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
