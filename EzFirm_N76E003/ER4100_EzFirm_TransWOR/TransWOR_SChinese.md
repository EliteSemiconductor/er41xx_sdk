# ER4100_EzFirm_TransWOR

## 概述

本专案示范如何使用 ER4100 RF 收发器在 N76E003（8051）MCU 上实现 **WOR（Wake-On-Radio，唤醒接收）**功能。

设备大部分时间处于深度睡眠。ER4100 内部唤醒定时器（WUT）周期性打开短暂的 RX 窗口。收到封包后，MCU 唤醒并读取数据，处理完毕后重新进入 WOR 循环。

重要提示1: 在初始化後第一次呼叫 XTAPP_WOR_Trigger 前一定要等待數秒的延迟，讓芯片內部時脈穩定。

---

## WOR 流程

```
XTAPP_Init()
  │  SPI 初始化 → 软件复位 → TRx_Init() → 3s 稳定延迟
  └─► XTAPP_WOR_Trigger()
        │  设定 WUT 周期（100ms）+ RX 窗口（WOR_CNT × WOR_CNT_UNIT）
        │  清除 RX FIFO → TRx_RX_Trigger()
        └─► TRx_PowerDownMode()  ← 进入深度睡眠

    [ER4100 每 100ms 唤醒一次，打开 80ms RX 窗口]

NIRQ 触发
  └─► XTAPP_IrqHdlr()
        ├─ IS_INT_ST_ANY_RX  → XTAPP_WOR_Disable()
        │                       读取 FIFO + RSSI → dump_buffer()
        │                       XTAPP_WOR_Trigger()  ← 重启 WOR
        └─ 其他中断           → 清除 RX FIFO
                                XTAPP_WOR_Trigger()  ← 重启 WOR
```

---

## WOR 参数

[WOR 参数]
  `WOR_CNT`      : 4  (RX 窗口计数)
  `WOR_CNT_UNIT` : 3  (单位 = 20ms)
  RX 窗口时长    : 80ms  (WOR_CNT × 20ms)
  WUT 周期       : 100ms  (`SET_WUT_PERIORD_ARRAY_100MS`)

修改 `RF_App.c` 中的 `WUT_PERIOD_SEL` 可变更唤醒周期：

```c
#define WUT_PERIOD_SEL(arr)   SET_WUT_PERIORD_ARRAY_100MS(arr)
// 可选: 10MS / 50MS / 100MS / 500MS / 1S / 2S / 4S
```

WUT 寄存器计算公式：`寄存器值 = (周期ms × 32768) / 1000`

> **注意：** WOR 功能需要在 `SPI_ER41xx_config.h` 中启用 `EXTERN_RF_APIS_WUT`：
> ```c
> #define EXTERN_RF_APIS_WUT
> ```
> 未定义此宏时，`TRx_RX_WUTMR_Enable()`、`TRx_SetWUTMR_Timer()` 等 WUT 相关 API 不会被编译进去。

### GPIO1 TR_BSY 输出（RX 窗口量测）

本示例的 `SPI_ER41xx_config.h` 已预先将 **GPIO1 配置为 TR_BSY 输出**。
TR_BSY 在 ER4100 RX 窗口开启期间保持高电平，RX 窗口关闭、芯片回到睡眠后拉低。
使用示波器或逻辑分析仪探测 GPIO1 引脚，可直接观察 RX 窗口的开启时间。

```
reg 0x4004 = {0x06, 0x01, 0x16, 0x00}  ← GPIO1 配置为 TR_BSY
```

---

## 目录结构

```
ER4100_EzFirm_TransWOR/
│
├── Project/                          # Keil 专案档
│   ├── TransWOR.uvproj              # 主专案档
│   ├── TransWOR.uvopt               # 专案选项
│   ├── STARTUP.A51                  # 8051 启动汇编
│   ├── Nu_Link_8051_Driver.ini      # 烧录器设定
│   ├── LST/                         # 编译器输出列表档 (*.lst, *.map)
│   └── Output/                      # 编译输出 (*.obj, *.hex, *.lnp)
│
├── Include/                          # MCU 系统头文件（只读，勿修改）
│   ├── N76E003.h                    # MCU 寄存器定义
│   ├── SFR_Macro.h                  # SFR 操作宏
│   └── Function_Define.h            # MCU 函数定义
│
├── Common/                           # 共享函数库
│   ├── Common.h                     # 总 include 入口：typedef、MCU header、所有模块 header
│   ├── Delay.h / Delay.c            # 延迟函数
│   └── Uart.h / Uart.c              # UART 收发函数
│
├── User/                             # 用户应用层
│   └── main.c                       # 主程序：初始化、主循环
│
└── RF Drivers/                       # RF 驱动层
    ├── RF_Hal.h / RF_Hal.c          # RF HAL 层：引脚定义、SPI bit-bang 实现
    ├── RF_App.h / RF_App.c          # RF 应用层：WOR 初始化、接收、IRQ 处理
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
    │  WOR 触发/停止、封包接收、IRQ 处理
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（寄存器配置）
    │  寄存器读写、FIFO 操作、模式切换
    ▼
RF HAL 层
  RF_Hal.c / RF_Hal.h
    │  SPI 时序（软件 bit-bang）、引脚定义
    ▼
硬件
  N76E003 GPIO → ER4100 RF IC (433 MHz)
```

---

## SPI 引脚定义

[SPI 引脚定义]
  NIRQ  P0.3  输入  ER4100 中断信号
  MOSI  P0.0  输出  SPI 数据输出
  MISO  P0.1  输入  SPI 数据输入
  SCK   P1.0  输出  SPI 时钟
  NSS   P1.5  输出  SPI 片选（低有效）

---

## 主要 API 函数

[主要 API 函数]
  `XTAPP_Init()`         : 初始化 SPI、验证芯片 ID、启动 WOR
  `XTAPP_WOR_Trigger()`  : 设定 WUT 周期 + RX 窗口，进入深度睡眠
  `XTAPP_WOR_Disable()`  : 停止 WOR 定时器（发送或切换到普通 RX 前调用）
  `XTAPP_IrqHdlr()`      : 处理 NIRQ：处理接收封包或唤醒事件
  `XTAPP_ReceiveCheck()` : 读取 RX 数据长度及 FIFO 内容
  `XTAPP_Scan()`         : 主循环轮询：初始化失败时立即返回；NIRQ 低电平时调用 `XTAPP_IrqHdlr()`

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "TransWOR : init done\r\n"  — 初始化成功
  "TransWOR : init fail\r\n"  — 初始化失败

### XTAPP_Scan() 初始化保护

`XTAPP_Scan()` 开头检查 `rf_err_mode`，若初始化失败（`rf_err_mode == 1`）则立即返回，防止对未初始化的 RF 芯片进行访问。

---

## Keil 包含路径

```
..\Include
..\Common
..\User
..\RF Drivers
..\RF Drivers\ER4100Api
```

`..\Common` 须排在 `..\User` 之前，确保 `Common/Common.h` 优先被解析。
