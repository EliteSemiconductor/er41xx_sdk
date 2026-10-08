# EzFirm 2.0 工程列表

**目录**

- [工程总览](#工程总览)
- [共通程序流程](#共通程序流程)
- [工程说明](#工程说明)
  - [透明模式（6 个工程）](#透明模式6-个工程)
  - [IEEE 802.15.4 模式（3 个工程）](#ieee-802154-模式3-个工程)
  - [省电／唤醒定时器（4 个工程）](#省电唤醒定时器4-个工程)
  - [测试／工具（3 个工程）](#测试工具3-个工程)
- [修订记录](#修订记录)

每个示例均提供两个平台版本，应用逻辑相同：

- N76E003（8051）
  - 工程 : `EzFirm_v2_8051/ER4100_EzFirm_B_N76_<Variant>`
  - IDE : Keil C51，`Project/*.uvproj`
  - UART : TXD P06，RXD P07，115200 bps
  - TX／触发按键 : P05（低电平有效）
  - ER4100 关机引脚（GPIO1）: P04，用于 PowerSaving_Shutdown
- Nano100（Cortex-M0，DVB 板）
  - 工程 : `EzFirm_v2_M0/ER4100_EzFirm_B_DVB_<Variant>`
  - IDE : Keil MDK，`Keil/*.uvprojx`
  - UART : UART1 TX PB5，RX PB4，115200 bps
  - TX／触发按键 : DVB_SW5 = PB15（低电平有效）
  - ER4100 关机引脚（GPIO1）: `RF_SDN_PIN` = PB10，用于 PowerSaving_Shutdown

---

## 工程总览

格式：Variant : 功能

- [`TransTxRx`](#transtxrx) : 基本双向透明传输
- [`TransTxRx_CCIT`](#transtxrx_ccit) : 透明传输，帧含长度 + 序号 + CRC-CCITT
- [`TransTxRx_CCIT_PER`](#transtxrx_ccit_per) : 使用 CCIT 帧的误包率（PER）测试
- [`TransTxRx_LongPkt`](#transtxrx_longpkt) : 利用 FIFO 阈值收发 512 字节长包
- [`TransWOR`](#transwor) : 无线唤醒（WOR）低功耗接收
- [`TransWOR_Ack_CCIT`](#transwor_ack_ccit) : WOR 接收 + CRC-CCITT 校验 + 回复 ACK
- [`802154TxRx`](#802154txrx) : IEEE 802.15.4 收发
- [`802154TxRx_Addressing`](#802154txrx_addressing) : 802.15.4 + 硬件 PAN ID／短地址过滤
- [`802154TxRx_CCA`](#802154txrx_cca) : 802.15.4 先听后发（CCA）发射
- [`SingleTone`](#singletone) : 持续载波，用于频率／输出功率量测
- [`RSSI_Scan`](#rssi_scan) : 周期性扫描信道 RSSI
- [`GIO_TRBSY`](#gio_trbsy) : 将收发忙碌状态（TR_BSY）输出到 ER4100 GPIO0
- [`PowerSaving`](#powersaving) : 以命令进入／退出深度睡眠
- [`PowerSaving_Shutdown`](#powersaving_shutdown) : 以 ER4100 GPIO1 引脚关机
- [`PowerSaving_WUT`](#powersaving_wut) : 深度睡眠，由唤醒定时器周期唤醒
- [`WakeupTimer(WUT)`](#wakeuptimerwut) : active 模式下的唤醒定时器，以 UART 设定周期／模式

建议使用的 EzToolkit 设定文件（位于 `Tools/`）：

- `EzToolkit_settings_TransTxRx.ini` : TransTxRx、TransTxRx_CCIT、TransTxRx_CCIT_PER、SingleTone、RSSI_Scan
- `EzToolkit_settings_longPkt.ini` : TransTxRx_LongPkt（开启 TX FIFO 将空／RX FIFO 将满中断）
- `EzToolkit_settings_WOR.ini` : TransWOR、TransWOR_Ack_CCIT
- `EzToolkit_settings_802154.ini` : 802154TxRx、802154TxRx_Addressing
- `EzToolkit_settings_802154_TXCCA.ini` : 802154TxRx_CCA（与 802154 相同，另开启 `INT_ST_TXERR` 中断）
- `EzToolkit_settings_GPIO0_TRBSY.ini` : GIO_TRBSY（GPIO0 为 GIO 模式）
- `EzToolkit_settings_PowerSaving.ini` : PowerSaving、PowerSaving_Shutdown、PowerSaving_WUT（关闭 RX／TX 中断，开启 `INT_ST_WAKEUP`，GPIO1 无上下拉）
- `EzToolkit_settings_WakeupTimer.ini` : WakeupTimer(WUT)（关闭 RX／TX 中断，开启 `INT_ST_TMRTHD`）

成对使用的两块板必须使用**相同**的 RF 设定（频率、速率、同步字等）。

---

## 共通程序流程

所有工程架构相同（`User/main.c` → `RF Drivers/RF_App.c`）：

```
main()
 ├─ UART 初始化（115200）+ GPIO 初始化
 ├─ XTAPP_Init()
 │    ├─ TRx_IoConfig()               SPI／NIRQ 引脚
 │    ├─ XTAPP_Strobe()               读取芯片 ID 0xF100，最多重试约 1 秒
 │    ├─ TRx_SW_Reset() + TRx_Init()  载入 SPI_ER41xx_config.h（由 EzToolkit 生成）
 │    ├─ 各工程的启动动作               例如开启 RX／WOR／单音／WUT
 │    └─ dump_rf_config()             打印晶振、CapIO、速率、频偏、频率、功率、
 │                                    FIFO／中断／TX 模式、同步字
 ├─ 初始化成功 → 打印 "<Project> : init done" + 使用说明／命令菜单
 ├─ 初始化失败 → 仅打印 "<Project> : init fail"
 └─ while(1) XTAPP_Scan()
      ├─ NIRQ 为低 → XTAPP_IrqHdlr()  读取并清除中断状态、处理 RX、重启 RX
      └─ 按键／UART → 组包、发射、等待 TX 完成、重启 RX
```

初始化失败（`rf_err_mode = 1`）时，`XTAPP_Scan()` 会直接返回，不执行任何动作——
看到 `init fail` 请检查 SPI 接线与供电。

修改 RF 参数请用 EzToolkit 重新生成 `RF Drivers/ER4100Api/SPI_ER41xx_config.h`；
应用层参数则为 `RF_App.c`／`RF_App.h` 中的 `#define`。

---

## 工程说明

按类别说明各工程的启动行为、操作方式与对端。

### 透明模式（6 个工程）

#### TransTxRx
基本双向透明传输。
- **启动：** 开启 RX，`RX_TRANSPARENT`，固定接收长度 `RF_BUF_SIZE` = 64 字节。
- **TX：** 按下按键 → 发送 64 字节 `00 01 02 … 3F`，等待 TX 完成，打印 `TX(64):` + 十六进制
  内容及 `Tx done.`，松开按键后回到 RX。
- **RX：** RX 中断时读取 FIFO 与 RSSI，打印 `RX(64): rssi: -XX` + 十六进制内容。
- **用法：** 两块板烧录本工程，任一块按键，观察另一块的 UART 日志。

#### TransTxRx_CCIT
在 TransTxRx 基础上加入软件帧格式与校验。
- **帧格式：** `[len][seq_hi][seq_lo][payload…][crc_lo][crc_hi]`
  - `len` = 序号（2）+ payload + CRC（2），不含自身
  - CRC-CCITT（多项式 0x8408，初值 0x0000），计算范围为序号 + payload
- **TX：** 按键 → payload 59 字节（`RF_BUF_SIZE − 5`），每按一次序号加 1。
- **RX：** 打印 `RX seq=N`，接着为 `CRC OK` + 十六进制内容，或 `CRC FAIL`／`RX: bad length`／
  `RX: len mismatch`。
- **用法：** 两块板均烧录 TransTxRx_CCIT。

#### TransTxRx_CCIT_PER
使用 TransTxRx_CCIT 帧格式的误包率（PER）测试。
- **TX：** 按键 → 连续发送 `PER_TX_COUNT` = 100 包，间隔 `PER_TX_INTERVAL_MS` = 30 ms，
  每包打印一个 `.`。
- **RX：** 每收到一包打印 pass／error 计数、RSSI，以及目前的 PER
  `(100 − pass) / 100`（百分比，两位小数）。
- **用法：** 先复位 RX 板，再在 TX 板按一次按键，于 RX 板读取最终 PER。包数／间隔可在
  `RF_App.h` 修改。

#### TransTxRx_LongPkt
利用 FIFO 阈值收发长包（512 字节，超过 128 字节 FIFO）。
- **需要**使用 `longPkt` 的 `.ini`（开启 TX FIFO 将空与 RX FIFO 将满中断，阈值 64 字节 =
  `RF_RX_ALMOST_FULL_THR`／`RF_TX_ALMOST_EMPTY_THR`）。
- **帧格式：** `[len_hi][len_lo][pkt_num][data…][crc_lo][crc_hi]`，总长 `RF_TX_SIZE` = 512。
- **TX：** 先填满 FIFO，以总长度启动 TX，每次将空中断补 64 字节，直到 TX 完成
  （约 250 ms 超时保护，超时打印 `TX Timeout!`）。
- **RX：** 每次将满中断读出 64 字节，RX 完成时读出剩余数据并校验 CRC，成功则打印
  `PktNum:` 与 512 字节内容。
- **用法：** 两块板均烧录 TransTxRx_LongPkt。

#### TransWOR
无线唤醒（WOR）接收端（低功耗周期侦听）。
- **启动：** 等待 3 秒让 ER4100 稳定，然后启动 WOR。
- **WOR 时序：** 唤醒周期 `WUT_PERIOD_SEL` = 1 s（可选 10／50／100／500 ms、1／2／4 s）；
  RX 窗口 = `WOR_CNT` × `WOR_CNT_UNIT` = 4 × 20 ms = 80 ms。
- **RX：** 收到数据包时停止 WOR，打印 `RX(…)` + RSSI + 内容，然后重启 WOR。
  其他中断仅重置 RX FIFO 并重启 WOR。
- **用法：** 发射端为烧录 **TransTxRx** 且设定相同的板子。数据包需落在 RX 窗口内才会被
  接收；若无显示，请再按一次 TX 按键。

#### TransWOR_Ack_CCIT
会校验 CCIT 帧并回复的 WOR 接收端。
- WOR 时序与 TransWOR 相同。
- **RX：** 进行 CRC-CCITT 校验（帧格式同 TransTxRx_CCIT）。`CRC OK` 时：等待 200 ms，
  发送 3 字节 `"ACK"`，打印 `Tx done.`，重启 WOR。错误帧直接丢弃。
- **用法：** 发射端为烧录 **TransTxRx_CCIT** 的板子，其 UART 日志会显示收到的 `ACK`。

---

### IEEE 802.15.4 模式（3 个工程）

#### 802154TxRx
- **启动：** 开启 RX，`RX_802154`。接收长度由 PHR 决定，因此长度参数为 0。
- **TX：** 按键 → 64 字节帧 = MHR 样本（`M802154_MHR_PATTERN`）+ payload，payload 第一个
  字节为序号。前导码、同步字、PHR 与 CRC 由硬件自动加入。
- **RX：** `INT_ST_RX` → 打印帧内容 + RSSI。`INT_ST_RXERR` → 打印 `RX Error` 及
  `Valid802154／CrcErr／FormatErr／FormatRej` 标志（寄存器 0xA004），并重置 FIFO。
- **用法：** 两块板均烧录 802154TxRx。

#### 802154TxRx_Addressing
硬件 PAN ID／短地址过滤。
- **启动：** 本机为 `SRC_PANID` = 0xABCD、`SRC_ADDR` = 0x1234（写入 0xA070／0xA074，
  于 0xA00C 开启过滤），并打印命令菜单。
- **通过 UART 命令发射：**
  - `1` : 目标 0xABCD／0x1234（匹配）→ 接收
  - `2` : 目标 0xFFFF／0xFFFF（广播）→ 接收
  - `3` : 目标 0x5555／0x1234（PAN ID 不匹配）→ 拒收（`RX Error`，`FormatRej`）
  - `4` : 目标 0xABCD／0x5678（地址不匹配）→ 拒收（`RX Error`，`FormatRej`）

- **用法：** 两块板；在其中一块的终端输入 `1`–`4`，观察另一块的日志。
  地址设定于 `RF_App.h`。

#### 802154TxRx_CCA
在 802154TxRx 基础上，每次发射前先做 CCA（Clear Channel Assessment，先听后发）。
- **启动：** 与 802154TxRx 相同，另外以 `TRx_CCA_Config(0xFFF − 8 × CCA_THRESHOLD)` 写入
  CCA 门槛（寄存器 0xA040）。`CCA_THRESHOLD` = 80 代表 −80 dBm，开机打印
  `CCA Fail Threshold(dBm) : -80`。
- **中断：** `INT_EN_REG` 必须开启 `INT_ST_TXERR`（0x0F，原为 0x07），CCA 失败时 NIRQ 才会
  拉低。`EzToolkit_settings_802154_TXCCA.ini` 已开启此中断。
- **TX：** 按键 → 发送与 802154TxRx 相同的 64 字节帧，使用 `TRx_TX_Trigger(len, 1)`
  （开启 CCA）。芯片先量测信道能量：
  - 低于门槛 → 发射，`INT_ST_TX` → 打印 `Done. Ambient-RSSI(dBm): -XX` 及帧内容
  - 高于门槛 → 不发射，`INT_ST_TXERR` → 打印 `Fail. Ambient-RSSI(dBm): -XX`

  环境 RSSI 由寄存器 0xB1FC 读取（`TRx_GetRSSI_Data()`）。松开按键后重新开启 RX。
- **用法：** 另一块板烧录 802154TxRx 负责接收。若要观察 CCA 阻挡发射，可在第三块板
  烧录 SingleTone，以相同频率靠近 CCA 板发射。门槛可在 `RF_App.c` 的 `CCA_THRESHOLD`
  修改。
- **注意：** `TRx_GetRSSI_CCA()` 是 RSSI_Scan 使用的 RSSI 扫描 API，CCA 发射不需要调用。

---

### 省电／唤醒定时器（4 个工程）

以下工程不使用 RX／TX。请以电流表量测 ER4100 的供电电流，比较各省电模式。

#### PowerSaving
以命令进入与退出深度睡眠。
- **启动：** 芯片初始化后保持 active 模式，并打印使用说明。
- **按键：**
  - active → `TRx_PowerDownMode()` 使 ER4100 进入深度睡眠，打印 `>>DeepSleep`
  - 深度睡眠 → `TRx_NOP()`（任意 SPI 命令）唤醒芯片；经过唤醒延迟后，
    `INT_ST_WAKEUP` 中断打印 `>>Wakeup`
- **用法：** 比较 active 模式与深度睡眠的电流。

#### PowerSaving_Shutdown
最低功耗模式：以 ER4100 的 GPIO1 引脚关机。
- **硬件：** ER4100 GPIO1（默认模式为关机输入，低电平 = 关机）由 MCU 控制——8051 为 P04，
  M0 为 `RF_SDN_PIN`（PB10）。正常工作时保持高电平。
- **启动：** `TRx_Init()` 之后以 `TRx_PCRMU_ShutDown_Enable()` 在 PCR 中开启关机功能；
  未执行此步骤时，将 GPIO1 拉低不会有作用。
- **按键：**
  - active → 引脚拉低，ER4100 进入关机，打印 `>>Shutdown`
  - 关机 → 引脚拉高，ER4100 重新开机。寄存器内容不会保留，因此由 `XTAPP_ChipInit()`
    重新执行 strobe／reset／init／开启关机功能，然后打印 `>>Wakeup`（或 `>>Wakeup fail`）
- **用法：** 量测关机电流。

#### PowerSaving_WUT
深度睡眠，由唤醒定时器（WUT）自动唤醒。
- **启动：** 打印 `Enter deep sleep and wakeup after WUT timing...`，以单次模式启动 WUT
  （`WUT_PERIOD_SEL` = 2 s），并进入深度睡眠。
- **唤醒：** `INT_ST_WAKEUP` → 打印 `Wakeup and wait for 1sec...`，保持 active 1 秒，打印
  `Enter deep sleep again`，重新启动单次 WUT 并再次进入深度睡眠，如此循环。
- **WUT 周期：** 寄存器值 = 周期（ms）× 32768 / 1000。已提供 10／50／100／500 ms 与
  1／2／4 s 的宏；可在 `RF_App.c` 修改 `WUT_PERIOD_SEL`。
- **`INT_ST_TMRTHD`** 在本工程为可选。仅在开启其中断屏蔽时才会产生；开启后，定时器到期时
  会先产生 TMRTHD 事件，接着才是 WAKEUP 事件。
- **用法：** 观察周期性的电流波形（active 1 秒／深度睡眠 2 秒）。

#### WakeupTimer(WUT)
芯片保持 active 模式下运行唤醒定时器，以 UART 控制。
- **启动：** 停止正在运行的 WUT，在中断屏蔽中开启 `INT_ST_TMRTHD`，并打印命令菜单。
  收到第一个命令后才启动定时器。
- **UART 命令：**
  - `0` ~ `4` : 周期模式，10 ms／50 ms／100 ms／500 ms／1 s
  - `5` ~ `7` : 单次模式，1 s／2 s／4 s
  - `p` : 停止定时器并重新打印菜单

  每次输入周期／模式命令都会重新启动定时器、清除事件计数，并打印例如
  `WUT starting : period = 1s, mode = Periodic`。
- **事件：** 每次定时器到期（`INT_ST_TMRTHD`）打印
  `WUT event : N, key in 'p' to stop periodic`。单次模式则打印 `WUT event : 1`，
  接着打印 `One-shot done`。
- **用法：** 以带时间戳的 UART 日志检查 WUT 时序，或在自己的应用中沿用
  `XTAPP_WUT_Config()`／`XTAPP_WUT_Stop()`。

---

### 测试／工具（3 个工程）

#### SingleTone
- **启动：** 自动以设定的频率与功率发射单音（无调制载波）。
- **按键：** 切换载波关闭／开启（`Stop Single Tone!`／`Start Single Tone!`）。
- **用法：** 搭配频谱仪进行频偏（CapIO）校准与输出功率量测。

#### RSSI_Scan
- **启动：** 以非数据包模式开启 RX，收集环境信号；每 1 秒读取一次 RSSI 并打印 `rssi(dBm): -XX`。
- **用法：** 检查信道背景噪声／干扰，或量测附近的发射端（例如 SingleTone 板）。

#### GIO_TRBSY
示范如何将内部状态信号输出到 ER4100 GPIO。
- **基础：** 与 TransTxRx 相同（按键 = 发送 64 字节）。
- **UART 命令：**
  - `0` : GPIO0 → Debug 模式（关闭 TR_BSY 输出）
  - `1` : GPIO0 → GIO 模式，选择 TR_BSY（`TRx_GPIO0_Sel(1, TR_BSY)`）
  - `2` : 开启 RX
  - `3` : 关闭 RX
- **用法：** 对端可使用另一块烧录 TransTxRx 的板子。用示波器量测 ER4100 GPIO0。输入 `1` 后，TX／RX 进行期间 GPIO0 为高电平——
  按键可看到 TX 波形，输入 `2` 可看到 RX 窗口。

---

## 修订记录

- 2026-10-07
  - 新增 PowerSaving、PowerSaving_Shutdown、PowerSaving_WUT、WakeupTimer(WUT)
  - 使用说明／命令菜单仅在初始化成功时打印
- 2026-09-30
  - 初版发布（EzFirm 2.0）
