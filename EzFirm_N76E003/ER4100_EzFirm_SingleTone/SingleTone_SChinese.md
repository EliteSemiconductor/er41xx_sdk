# ER4100_EzFirm_SingleTone

## 概述

本专案示范使用 ER4100 RF 收发器在 N76E003（8051）MCU 上进行 **Single Tone（单频连续波）** 测试。

按下按键（P0.5）启动 Single Tone 模式，ER4100 发射连续载波；再次按下则停止。可用于测量 RF 频率、功率或调试天线。本专案不含封包收发（RX/TX），纯为射频特性测量使用。

---

## 目录树

```
ER4100_EzFirm_SingleTone/
│
├── Project/                          # Keil 专案档
│   ├── SingleTone.uvproj            # 主专案档
│   ├── SingleTone.uvopt             # 专案选项
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
    ├── RF_App.h / RF_App.c          # RF 应用层：初始化、Single Tone 控制
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
    │  Single Tone 触发/停止、按键扫描
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（寄存器配置）
    │  寄存器读写、模式切换
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

## Single Tone 操作流程

```
上电
  └─► XTAPP_Init()
        │  SPI 初始化 → Chip ID 确认 → TRx_Init()
        └─► rf_single_tone_state = st_start（默认上电自动触发）

首次扫描循环
  └─► XTAPP_Scan()
        │  state == st_start
        │  UART: "Single Tone Start\r\n"
        │  TRx_SingleTone_Trigger()   ← 开始发射连续载波
        └─► rf_single_tone_state = st_launching

按下 P05（发射中）
  └─► XTAPP_Scan()
        │  非 stop 状态 → st_stop
        │  UART: "Single Tone Stop\r\n"
        └─► TRx_SingleTone_Disable()

按下 P05（已停止）
  └─► XTAPP_Scan()
        │  st_stop → st_start
        │  UART: "Single Tone Start\r\n"
        └─► TRx_SingleTone_Trigger()
```

状态机：`st_stop` ↔ `st_start` → `st_launching`（发射中）

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

### GPIO — 应用引脚

[GPIO — 应用引脚]
  P05  Push-pull  Single Tone 控制按键（低电平触发，按一下启动，再按一下停止）

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "SingleTone : init done\r\n"  — 初始化成功
  "SingleTone : init fail\r\n"  — 初始化失败

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
