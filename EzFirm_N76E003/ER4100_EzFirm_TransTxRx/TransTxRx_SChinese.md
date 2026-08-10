# ER4100_EzFirm_TransTxRx

## 概述

本专案示范使用 ER4100 RF 收发器在 N76E003（8051）MCU 上进行基本的**透明模式双向收发**。

设备持续开启 RX 监听，收到封包后通过 UART 输出数据及 RSSI；按下按键（P0.5）可手动触发 TX 发送。

---

## 目录树

```
ER4100_EzFirm_TransTxRx/
│
├── Project/                          # Keil 专案档
│   ├── TransTxRx.uvproj             # 主专案档
│   ├── TransTxRx.uvopt              # 项目选项
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
    ├── RF_App.h / RF_App.c          # RF 应用层：初始化、收发、IRQ 处理
    │
    └── XtLibSrc/                    # ER4100 底层 SPI API（ESMT 原厂提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、缓存器操作、FIFO 管理
        └── SPI_ER41xx_config.h      # RF 缓存器初始值配置（由 EzGen 工具产生，当前使用）
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
    │  封包收发、IRQ 处理、RX buffer 管理
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

### RF_RX_SIZE
`RF_App.c` 中的 `RF_RX_SIZE` 为传入 `TRx_RX_Trigger()` 的期望接收长度：
- **802.15.4 模式**：此值不影响接收，MAC 层从 PHR 自动取得封包长度
- **Transparent 模式**：须填写实际期望长度（standard: 1~128、long packet: 1~2047）

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "TransTxRx : init done\r\n"  — 初始化成功
  "TransTxRx : init fail\r\n"  — 初始化失败

### XTAPP_Scan() 初始化保护

`XTAPP_Scan()` 开头检查 `rf_err_mode`，若初始化失败（`rf_err_mode == 1`）则立即返回，防止对未初始化的 RF 芯片进行访问。

---

## Keil 包含路径（Include Paths）

```
..\Include
..\Common
..\User
..\RF Drivers
..\RF Drivers\XtLibSrc
```

优先级：`..\Common` 在 `..\User` 之前，确保 `Common/Common.h` 优先于 `User/Common.h`（若存在）。
