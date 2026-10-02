# ER4100 SDK_EZ（v2.0）

SDK_EZ 让开发者能在低阶微控制器平台上轻松开发 ESMT ER41xx 系列 Sub-GHz 射频收发芯片的
代码，由两部分组成：

- **EzToolkit** — PC 端图形接口工具。配置 RF 参数、生成寄存器头文件
  （`SPI_ER41xx_config.h`），并可通过 UART 控制 ER4100 DVB 评估板进行收发测试。
- **EzFirm** — ER4100 固件示例，每个工程对应芯片的一项功能，可导入 EzToolkit 生成的
  头文件。

> **SDK_EZ v2.0 变更：** EzGen / EzCodeGen 已由 **EzToolkit** 取代，EzFirm 升级至 **2.0**
> （支持 N76E003 与 Nano100 两个平台）。原 `EzGen/` 与 `EzFirm_N76E003/` 目录已移除，
> 详见 [`revise_sc.txt`](revise_sc.txt)。

---

## 内容说明

- [`Tools/`](Tools/) : `ER41xx EzToolkit_V2.0_20260929.exe` 及示例 `.ini` 设定文件
- [`Reference Code/EzFirm_v2_8051/`](Reference%20Code/EzFirm_v2_8051/) : 基于 Nuvoton **N76E003**（8051）的 EzFirm 2.0，Keil C51
- [`Reference Code/EzFirm_v2_M0/`](Reference%20Code/EzFirm_v2_M0/) : 基于 DVB 板上 Nuvoton **Nano100**（Cortex-M0）的 EzFirm 2.0，Keil MDK；附 `Nano100Lib`
- [`Application Note/`](Application%20Note/) : **ER41xx SDK_EZ 使用手册**（ESAP-SPHYNX-026，v2.0，简体中文）
- [`revise_sc.txt`](revise_sc.txt) : 版本说明

---

## EzToolkit

Windows GUI 辅助开发工具（.NET Framework 4.8 / WinForms）。搭配 DVB 评估板（USB 转 UART）
及 EzFirm，可完成射频参数配置、寄存器生成、收发测试、频偏校准与量产参数固化。

**Parameter 配置面板**
- **RF PARAM** — 晶振（38.4 / 40 MHz、Gain、CapIO）、频率、发射功率（PA Config／Power LV）、
  GFSK 调制（速率／频偏）、帧参数（Preamble／Syncword／同步容错位／LSB-MSB 顺序）
- **FREQ TABLE** — 多信道频率表，Generate 时一并打包进初始化数组
- **TX FIFO MODE** — Transparent + Presync（推荐）／Transparent／802.15.4
- **GPIO** — 每个引脚独立配置 Mode／GIO Sel／上下拉／反相；GIO 功能（TR_BSY、TR_FSH、
  RX_SYNC、TX_EN 等）与中断事件（`INT_ST_*`）
- **PCR** — 电源管理（PCRMU）与 SHD 硬件关断

**DVB Tester 测试面板**（需连接 DVB 评估板）
- **Packet TX** — Length／Seq／Payload／CRC，随机 Payload，发送包数／间隔／Infinite
- **Continuous TX** — SingleTone 或 Pattern（Bit_01010101、PRBS9）
- **Packet RX** — OK／Err 计数统计、RSSI 扫描
- **State Ctrl** — Idle／DeepSleep／Shutdown

**REG TABLE**
- 按分类／过滤查看寄存器实时数值
- 通过 UART 读写 DVB 寄存器（Shift + 鼠标左键多选，**Write Multi** 批量写入）
- **Generate** — 生成 C 语言初始化数组（`Initial_Reg_Array` + 信道表），供 EzFirm 使用

参数可另存／读取为 `.ini` 设定文件。`Tools/` 中的示例：

- `EzToolkit_settings_TransTxRx.ini` : `TransTxRx`／`TransTxRx_CCIT`／`TransTxRx_CCIT_PER`
- `EzToolkit_settings_longPkt.ini` : `TransTxRx_LongPkt`
- `EzToolkit_settings_WOR.ini` : `TransWOR`／`TransWOR_Ack_CCIT`
- `EzToolkit_settings_802154.ini` : `802154TxRx`／`802154TxRx_Addressing`
- `EzToolkit_settings_GPIO0_TRBSY.ini` : `GIO_TRBSY`

---

## EzFirm 2.0 示例

每个示例均提供两个平台版本：
`ER4100_EzFirm_B_N76_<Variant>`（N76E003）与 `ER4100_EzFirm_B_DVB_<Variant>`（Nano100）。

- `TransTxRx` : 基本透明模式收发
- `TransTxRx_CCIT` : 透明模式收发，带序号与软件 CRC-CCITT 校验
- `TransTxRx_CCIT_PER` : 基于 `TransTxRx_CCIT` 的误包率（PER）测试
- `TransTxRx_LongPkt` : 长包收发（512 字节 payload），通过 FIFO 阈值管理实现
- `TransWOR` : 无线唤醒（WOR）接收
- `TransWOR_Ack_CCIT` : WOR 接收，带 CRC-CCITT 校验并自动回复 `"ACK"`
- `802154TxRx` : IEEE 802.15.4 收发
- `802154TxRx_Addressing` : IEEE 802.15.4 PAN ID／地址过滤（匹配帧与广播帧）
- `SingleTone` : 持续发射单音载波，用于 RF 测试
- `RSSI_Scan` : 背景射频能量（RSSI）扫描，每 1 秒输出一次
- `GIO_TRBSY` : 通过 UART 命令将 GPIO0 切换为 TR_BSY 输出，观察收发忙碌时序

各示例的使用方式（按键、UART 命令、对端板、程序流程）请见
[`Reference Code/FirmList_SC.md`](Reference%20Code/FirmList_SC.md)。

工程结构：`User/`（main）、`RF Drivers/`（`RF_App`、`RF_Hal`、`ER4100Api/SPI_ER41xx*`）、
`Common/`（UART、延时），以及 `Project/`（8051，`.uvproj`）或 `Keil/`（M0，`.uvprojx`）。

---

## 快速开始

1. 连接 DVB 板，打开 `Tools/` 中的 **EzToolkit**，载入与工程对应的示例 `.ini`。调整 RF
   参数，并在 DVB Tester 面板验证收发。
2. 在 REG TABLE 点击 **Generate**，将生成的文件放到工程的 `RF Drivers/ER4100Api/` 目录
   （替换 `SPI_ER41xx_config.h`）。
3. 打开 Keil 工程并全部编译（F7）：
   - N76E003：`Project/*.uvproj`（Keil C51）
   - Nano100：`Keil/*.uvprojx`（Keil MDK-ARM）
4. 通过 Nu-Link 烧录输出文件。

### 注意事项

- **PA Config 必须与 PCB 匹配网络一致。** 硬件为 L-PA 时强行设定 23 dBm（H-PA）会导致
  阻抗失配，可能损坏 PA。降功率请保持 PA Config 不变，调低 Power LV。
- **CapIO** 为频偏校准参数，量产前须用频谱仪校准并固定。
- DVB 评估板仅支持 **Transparent + Presync** 模式；DVB Band 仅用于评估板，对量产固件无作用。
- FREQ TABLE 仅存储频率，速率、频偏、功率、同步字均沿用 RF PARAM 全局参数。
- GPIO1 的 SHD 功能需在 PCR 开启 Enable + Shutdown 才生效。内部测试类 GPIO 模式
  （RX_RF／RX_BBP／NFC／Debug）量产禁止使用。

完整说明请参考 [`Application Note/`](Application%20Note/) 中的 SDK_EZ 使用手册。
