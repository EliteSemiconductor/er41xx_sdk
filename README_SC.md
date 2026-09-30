# ER4100 SDK_EZ（v2.0）

ESMT ER4100 系列 Sub-GHz 射频收发芯片 SDK 的发布根目录：PC 端 EzToolkit、支持两种 MCU
平台的 EzFirm 2.0 固件示例，以及相关文档。

> **SDK_EZ v2.0 变更：** EzGen / EzCodeGen 已由 **EzToolkit** 取代，EzFirm 升级至 **2.0**。
> 原 `EzGen/` 与 `EzFirm_N76E003/` 目录已移除，详见 [`revise_sc.txt`](revise_sc.txt)。

---

## 内容说明

| 目录 | 说明 |
|---|---|
| [`Tools/`](Tools/) | **ER41xx EzToolkit V2.0**（`ER41xx EzToolkit_V2.0_20260929.exe`）及示例 `.ini` 设定文件（common / 802.15.4）。 |
| [`Reference Code/EzFirm_v2_8051/`](Reference%20Code/EzFirm_v2_8051/) | 基于 Nuvoton **N76E003**（8051）的 EzFirm 2.0 示例，Keil C51 工程。 |
| [`Reference Code/EzFirm_v2_M0/`](Reference%20Code/EzFirm_v2_M0/) | 基于 DVB 板上 Nuvoton **Nano100**（Cortex-M0）的 EzFirm 2.0 示例，Keil MDK 工程；附 `Nano100Lib`。 |
| [`Document/`](Document/) | SDK_EZ 用户指南（`ESAP-SPHYNX-002-SDK_EZ-User-Guide`）。 |
| `Datasheet/` | ER4100 数据手册。 |
| `Application Note/` | 应用笔记。 |
| [`revise_sc.txt`](revise_sc.txt) | 版本说明。 |

---

## EzToolkit

EzToolkit 是随 SDK_EZ 提供的 Windows GUI 辅助开发工具（.NET Framework 4.8 / WinForms），
用于 ER4100 系列芯片的参数配置、寄存器生成与收发测试。搭配 DVB 评估板（USB 转 UART）
可完成射频参数配置、寄存器生成、收发测试、频偏校准与量产参数固化，取代人工查表计算
寄存器的流程。

**Parameter 配置面板**
- **RF PARAM**：晶振、频率／频段、发射功率（PA Config／Power LV）、GFSK 调制与封包参数
  （Preamble／Syncword／容错位）
- **FREQ TABLE**：多信道频率表管理
- **TX FIFO MODE**：Transparent／Transparent + Presync／802.15.4 三种发射封包模式
- **GPIO**：每个引脚可独立配置模式／方向／上下拉／反相，含中断事件（`INT_ST_*`）
- **PCR**：电源管理（PCRMU）与 SHD 硬件关断控制

**DVB Tester 测试面板**（需连接 DVB 评估板）
- **Packet TX**：单包发射测试，支持随机 Payload、CRC 计算、发送包数／间隔／Infinite 模式
- **Continuous TX**：SingleTone／Pattern（Bit_01010101、PRBS9）连续波测试
- **Packet RX**：接收计数统计（OK／Err）、RSSI 扫描
- **State Ctrl**：Idle／DeepSleep／Shutdown 电源状态切换

**REG TABLE 寄存器操作**
- 按分类／过滤查看所有寄存器实时数值
- 通过 UART 直接读写 DVB 上的寄存器（含 **Write Multi** 批量写入）
- 一键 **Generate** 生成 C 语言初始化数组（`Initial_Reg_Array` + 信道表），可直接并入
  EzFirm 固件工程

参数可另存／读取为 `.ini` 设定文件，方便工程间复用与量产参数固化。

---

## EzFirm 2.0 示例

每个示例均提供两个平台版本：
`ER4100_EzFirm_B_N76_<Variant>`（N76E003）与 `ER4100_EzFirm_B_DVB_<Variant>`（Nano100）。

| Variant | 说明 |
|---|---|
| `TransTxRx` | 基本透明模式收发 |
| `TransTxRx_CCIT` | 透明模式收发，带序号与软件 CRC-CCITT 校验 |
| `TransTxRx_CCIT_PER` | 基于 `TransTxRx_CCIT` 的误包率（PER）测试 |
| `802154TxRx` | IEEE 802.15.4 收发 |
| `GIO_TRBSY` | GPIO 示例：通过 UART 命令将 GPIO0 切换为 TR_BSY 输出，观察收发忙碌时序 |

工程结构：`User/`（main）、`RF Drivers/`（`RF_App`、`RF_Hal`、`ER4100Api/SPI_ER41xx*`）、
`Common/`（UART、延时），以及 `Project/`（8051，`.uvproj`）或 `Keil/`（M0，`.uvprojx`）。

---

## 快速开始

1. 连接 DVB 板，打开 `Tools/` 中的 **EzToolkit**。载入示例 `.ini` 文件，调整 RF 参数，
   并用 DVB Tester 面板验证收发。
2. 在 REG TABLE 点击 **Generate**，用生成结果替换工程中的
   `RF Drivers/ER4100Api/SPI_ER41xx_config.h`。
3. 打开 Keil 工程并全部编译（F7）：
   - N76E003：`Project/*.uvproj`（Keil C51）
   - Nano100：`Keil/*.uvprojx`（Keil MDK-ARM）
4. 通过 Nu-Link 烧录输出文件。

完整说明请参考 [`Document/`](Document/) 中的 SDK_EZ 用户指南。
