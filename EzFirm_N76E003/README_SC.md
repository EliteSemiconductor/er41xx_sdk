# ER4100 EzFirm SDK

基于 Nuvoton N76E003（8051）MCU 的 ER4100 射频收发器嵌入式固件 SDK。
提供多个示例工程，覆盖常见无线通信应用场景。

---

## 硬件平台

- MCU              : Nuvoton N76E003（8051 架构，16 MHz，1T 模式）
- RF 芯片          : ER4100（ESMT）
- 频率范围         : Band 1：99.3 ~ 125.5 MHz
                     Band 2：196.5 ~ 253 MHz
                     Band 3：387 ~ 508 MHz
                     Band 4：790 ~ 1018 MHz
- 调制方式         : FSK / GFSK
- 数据速率         : 0.625 ~ 2000 Kbps（晶振 40 MHz）
                     0.6 ~ 1920 Kbps（晶振 38.4 MHz）
- 发射功率         : 14 dBm（780 ~ 899 MHz）
                     10 / 23 dBm（其他频段）
- 晶振             : 38.4 MHz 或 40 MHz
- MCU-RF 接口      : SPI（软件 bit-bang）

**SPI 引脚分配：**
- MOSI    : P00
- MISO    : P01
- SCLK    : P10
- SS (CS) : P15
- IRQ     : P03

**UART 引脚分配（UART0，115200 baud）：**
- TXD     : P06
- RXD     : P07

---

## 示例工程列表

### 透明传输模式

- `ER4100_EzFirm_TransTxRx`
  标准双向透明 TX/RX，64 字节 payload，P05 触发发送
- `ER4100_EzFirm_TransTxRx_LongPkt`
  长包透明模式，最大 512 字节 payload，含 FIFO 阈值控制与 CRC-16 校验
- `ER4100_EzFirm_TransWOR`
  Wake-On-Radio 接收——低功耗接收端在检测到 RF 活动时唤醒，通过 UART 输出数据
- `ER4100_EzFirm_TransWOR_Ack`
  WOR 接收并在收包后自动回复 ACK
- `ER4100_EzFirm_TransTxRx_PER`
  透明模式 PER 测试——P05 触发发送 500 笔，每笔含 XOR checksum；接收端校验并实时显示及最终汇报 PER

### IEEE 802.15.4 模式

- `ER4100_EzFirm_802154TxRx`
  基本 802.15.4 双向 TX/RX，含硬件 CRC 与地址过滤
- `ER4100_EzFirm_802154TxRx_Addressing`
  802.15.4 地址过滤演示——通过 UART 指令发送不同 PANID/ADDR 组合的封包，验证接受/拒绝行为
- `ER4100_EzFirm_802154TxRx_CCA`
  802.15.4 带 CCA（空闲信道评估）的发送——发送前先检测信道
- `ER4100_EzFirm_802154TxRx_PER`
  802.15.4 PER 测试——P05 触发发送 500 笔；接收端利用硬件 CRC（IS_INT_ST_RX / IS_INT_ST_RXERR）统计并实时显示及最终汇报 PER

### NFC 模式

- `ER4100_EzFirm_NFC_Field_Interrupt`
  通过 INT_N 中断检测 NFC 磁场进入与离开；IRQ 来源设为 NFC_PWRGOOD
- `ER4100_EzFirm_NFC_Field_Polling`
  通过 SPI 直接轮询 PWRGOOD 位检测磁场；不使用中断；离开检测需连续 50 次确认（去抖动）
- `ER4100_EzFirm_NFC_Menu`
  NFC 磁场附着后通过 UART 交互菜单进行数据块读写及 IRQ 来源切换（USER_CFG4~7）
- `ER4100_EzFirm_NFC_ReadWrite`
  MCU 通过 SPI 对 NFC 标签内存（Block 4~19，64 字节）进行读写
- `ER4100_EzFirm_NFC_StatusTrigger`
  通过 INT_N（NFC_PWRGOOD）检测磁场附着；附着后切换 IRQ 来源为 USER_CFG4，由 NFC 读写器写入特定指令触发状态事件；离开通过 SPI 轮询 PWRGOOD 位，需连续 50 次确认（去抖动）

### 测试 / 工具

- `ER4100_EzFirm_SingleTone`
  持续发射单音载波，用于 RF 测试与频率校验
- `ER4100_EzFirm_RSSI_Scan`
  使用 CCA 模式测量环境信道 RSSI；公式：`RSSI_dBm = -((4095 - raw) / 8)`
- `ER4100_EzFirm_PowerSaving`
  通过 P05 按键触发进入深度睡眠 / 唤醒循环
- `ER4100_EzFirm_PowerSaving_WUT`
  带 WUT（唤醒定时器）的低功耗模式——无需按键，定时自动唤醒

---

## 目录结构

所有 EzFirm 工程采用相同的顶层目录布局：

```
ER4100_EzFirm_<Variant>/
├── Common/          # 共享工具：延时、UART、通用类型定义
├── Include/         # MCU 系统头文件（N76E003.h、SFR_Macro.h 等）
├── RF Drivers/      # RF 应用层、HAL 层及 ER4100 SPI API
├── User/            # 用户应用入口（main.c）
├── Project/         # Keil uVision 工程文件及构建输出（*.hex）
```

---

## 软件架构

所有 EzFirm 工程采用相同的四层架构：

```
+-------------------------------+
|      用户应用层               |  User/main.c
|  初始化、主循环               |
+---------------+---------------+
                |
+---------------v---------------+
|    RF 应用层                  |  RF Drivers/RF_App.c
|  TX/RX 控制、中断处理         |
+---------------+---------------+
                |
+---------------v---------------+
|    底层 SPI 驱动              |  RF Drivers/ER4100Api/SPI_ER41xx.c
|  寄存器读写、FIFO、模式控制   |
+---------------+---------------+
                |
+---------------v---------------+
|    RF HAL 层                  |  RF Drivers/RF_Hal.c
|  SPI bit-bang、引脚映射       |
+---------------+---------------+
                |
           +----v----+
           |  ER4100  |
           | RF 芯片  |
           +----------+
```

> **NFC 工程说明：** NFC 系列工程的 RF 应用层文件命名为 `RF_NFC_APP.c/.h`，
> 以与其他模式的 `RF_App.c/.h` 区分。

---

## RF 配置参数

**支持频段：**
- Band 1 : 99.3 ~ 125.5 MHz
- Band 2 : 196.5 ~ 253 MHz
- Band 3 : 387 ~ 508 MHz
- Band 4 : 790 ~ 1018 MHz

**通用 RF 参数：**
- 调制方式                   : FSK / GFSK
- 数据速率                   : 0.625 ~ 2000 Kbps（晶振 40 MHz）
                               0.6 ~ 1920 Kbps（晶振 38.4 MHz）
- 发射功率                   : 14 dBm @ 780 ~ 899 MHz
                               10 / 23 dBm @ 其他频段
- 晶振                       : 38.4 MHz 或 40 MHz
- 前导码                     : 0xAA，8 字节
- 位序                       : MSB first
- 同步字容错                 : 1 bit

---

## 构建环境

- **IDE：** Keil uVision（8051 工具链）
- **烧录器：** Nu-Link（Nuvoton）
- **烧录器驱动配置：** `Project/Nu_Link_8051_Driver.ini`
- **输出文件：** `Project/Output/*.hex`

构建步骤：
1. 用 Keil uVision 打开 `Project/*.uvproj`
2. 全部编译（F7）
3. 通过 Nu-Link 烧录 `Project/Output/*.hex`

---

## EzCodeGen 配置工具

每个工程包含一个 EzCodeGen `.ini` 配置文件，用于生成 `SPI_ER41xx_config.h` 中的 RF 寄存器表。
如需重新生成 `SPI_ER41xx_config.h`，请用 ER4100 EzCodeGen 工具打开对应 `.ini` 文件并导出头文件。

EzCodeGen 是 EzGen 改名后的延续版本。当前版本：**EzCodeGen V2.2**（前一版本为 EzGen V2.1）。
自 V2.1 起，生成的 `SPI_ER41xx_config.h` 有以下变化：
- **内嵌 `_D_CODE` 兼容巨集** — 生成的头文件现直接定义 `_D_CODE`
  （涵盖 SDCC / Keil C51 / IAR / AVR / XC8 / Renesas CC-RL / CA78K0R / ARM 等），
  与原先透过共用 `compiler_compat.h` 提供的巨集相同。所有阵列
  （`channel_pll_table`、`Initial_Reg_Array`、`TRx_Config_Array`、`TRx_ConfigEx_Array`）
  改用 `_D_CODE const` 宣告，取代舊版硬编码的 `code const`（仅適用於 Keil C51）。
- **`TRx_Config_Array[]` 順序調整並新增區塊** — 新增一段 **PA（功率放大器）設定**
  （暫存器 `0x1f00`~`0x1f38`，共 15 筆），排在 TX/RX 設定之前。新的陣列順序為：
  PA 設定 → TX 設定 → RX 設定 → Freq/Deviation/DataRate → Freq（Xtal）→ Power 設定。

本專案內全部 18 個 EzFirm 專案均已更新為 `_D_CODE` 風格的設定檔。
