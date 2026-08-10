# ER4100_EzFirm_TransTxRx_PER

## 概述

本专案示范使用 ER4100 RF 收发器在 N76E003（8051）MCU 上进行透明模式的**封包错误率（PER）量测**。

按下按键（P0.5）触发连续发送 **500 笔封包**，每笔间隔 50 ms。
接收端通过软件 XOR checksum 验证每笔封包，并累计正确/错误数。
每收到一笔即通过 UART 打印**即时 PER**，收满 500 笔后打印最终 PER 汇总。

**与 TransTxRx 的差异：**

| | TransTxRx | TransTxRx_PER |
|---|---|---|
| TX 触发 | P05 每次发 1 笔 | P05 连续发 500 笔 |
| TX 间隔 | 即时 | 每笔 50 ms |
| Payload | 用户数据 | seq[2] + pattern + XOR checksum |
| RX 处理 | 打印数据 + RSSI | 验证 checksum、统计 ok/error、打印即时 PER |
| PER 汇报 | — | 每笔即时 % + 收满 500 笔最终汇总 |

---

## TX 封包格式

每笔发送封包的透明模式 payload：

```
[seq_hi][seq_lo][data_0]...[data_N-2][XOR]
```

- `seq_hi / seq_lo` : 2 字节大端序列号（0~499）
- `data_0 ~ data_N-2` : 递增填充字节（`i & 0xFF`）
- `XOR` : 最后 1 字节——payload 前 N-1 字节的 XOR 校验值

---

## RX 验证与 PER 计算

每收到一笔封包，IRQ 处理函数（`XTAPP_IrqHdlr`）执行以下流程：

1. 从 FIFO 读取 payload
2. 重新计算前 N-1 字节的 XOR 值
3. 与接收到的 `payload[N-1]` 比较
   - 相符 → `gPerRxOk++`，打印 `RX OK #<seq>  rssi: -<val>`
   - 不符 → 打印 `RX CRC ERR #<seq>  rssi: -<val>`
4. `gPerRxTotal++`
5. 打印即时 PER：`PER: <err>/<total> = <pct>%`
6. 当 `gPerRxTotal >= 500` 时：打印最终汇总并重置两个计数器

**PER 计算公式：**

```
PER(%) = (gPerRxTotal - gPerRxOk) * 100 / gPerRxTotal
```

> **注意：** 完全丢失的封包不产生 IRQ，不会被计入。
> 若有封包在到达接收端前丢失，最终 total 可能小于 500。

---

## 目录树

```
ER4100_EzFirm_TransTxRx_PER/
│
├── Project/                          # Keil 专案档
│   ├── TransTxRx_PER.uvproj         # 主专案档
│   ├── TransTxRx_PER.uvopt          # 项目选项
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
    └── XtLibSrc/                    # ER4100 底层 SPI API（ESMT 原厂提供）
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
    │  PER TX 连发、IRQ 处理、XOR checksum 验证
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
`RF_App.c` 中的 `RF_RX_SIZE` 为传入 `TRx_RX_Trigger()` 的期望接收长度。
透明模式下须与 TX payload 长度一致。

### PER 计数变量

- `gPerRxOk`    : XOR checksum 验证通过的封包数
- `gPerRxTotal` : 收到的封包总数（ok + error）
- 打印最终 PER 汇总后两者均重置为 0

### 开机 UART 日志

开机时 `main.c` 通过 UART 输出专案名称及初始化结果：

  "TransTxRx_PER : init done\r\n"  — 初始化成功
  "TransTxRx_PER : init fail\r\n"  — 初始化失败

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
