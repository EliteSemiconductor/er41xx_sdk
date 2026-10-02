# EzFirm 2.0 工程列表

每个示例均提供两个平台版本，应用逻辑相同：

- N76E003（8051）
  - 工程 : `EzFirm_v2_8051/ER4100_EzFirm_B_N76_<Variant>`
  - IDE : Keil C51，`Project/*.uvproj`
  - UART : TXD P06，RXD P07，115200 bps
  - TX／触发按键 : P05（低电平有效）
- Nano100（Cortex-M0，DVB 板）
  - 工程 : `EzFirm_v2_M0/ER4100_EzFirm_B_DVB_<Variant>`
  - IDE : Keil MDK，`Keil/*.uvprojx`
  - UART : UART1 TX PB5，RX PB4，115200 bps
  - TX／触发按键 : DVB_SW5 = PB15（低电平有效）

---

## 工程总览

格式：Variant : 模式 — 触发方式 — 对端

- [`TransTxRx`](#transtxrx) : 透明模式 — 按键 — `TransTxRx`
- [`TransTxRx_CCIT`](#transtxrx_ccit) : 透明模式 + 序号 + CRC-CCITT — 按键 — `TransTxRx_CCIT`
- [`TransTxRx_CCIT_PER`](#transtxrx_ccit_per) : 透明模式 + 序号 + CRC-CCITT — 按键 — `TransTxRx_CCIT_PER`
- [`TransTxRx_LongPkt`](#transtxrx_longpkt) : 透明模式，512 字节长包 — 按键 — `TransTxRx_LongPkt`
- [`TransWOR`](#transwor) : 无线唤醒（WOR）接收 — 无（仅接收）— 以 `TransTxRx` 作为发射端
- [`TransWOR_Ack_CCIT`](#transwor_ack_ccit) : WOR 接收 + CRC-CCITT + ACK — 无（仅接收）— 以 `TransTxRx_CCIT` 作为发射端
- [`802154TxRx`](#802154txrx) : IEEE 802.15.4 — 按键 — `802154TxRx`
- [`802154TxRx_Addressing`](#802154txrx_addressing) : 802.15.4 + PAN ID／地址过滤 — UART `1`~`4` — `802154TxRx_Addressing`
- [`SingleTone`](#singletone) : 持续载波 — 按键（切换）— 频谱仪
- [`RSSI_Scan`](#rssi_scan) : RSSI（CCA）扫描 — 自动，每 1 秒 — 任意发射端／`SingleTone`
- [`GIO_TRBSY`](#gio_trbsy) : 透明模式 + GPIO0 = TR_BSY — 按键 + UART `0`~`3` — `TransTxRx`；示波器量 GPIO0

建议使用的 EzToolkit 设定文件（位于 `Tools/`）：

- `EzToolkit_settings_TransTxRx.ini` : TransTxRx、TransTxRx_CCIT、TransTxRx_CCIT_PER、SingleTone、RSSI_Scan
- `EzToolkit_settings_longPkt.ini` : TransTxRx_LongPkt（开启 TX FIFO 将空／RX FIFO 将满中断）
- `EzToolkit_settings_WOR.ini` : TransWOR、TransWOR_Ack_CCIT
- `EzToolkit_settings_802154.ini` : 802154TxRx、802154TxRx_Addressing
- `EzToolkit_settings_GPIO0_TRBSY.ini` : GIO_TRBSY（GPIO0 为 GIO 模式）

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
 │    ├─ 各工程的启动动作             例如开启 RX／WOR／单音
 │    └─ dump_rf_config()             打印晶振、CapIO、速率、频偏、频率、功率、
 │                                    FIFO／中断／TX 模式、同步字
 ├─ 打印 "<Project> : init done"（或 "init fail"）
 └─ while(1) XTAPP_Scan()
      ├─ NIRQ 为低 → XTAPP_IrqHdlr()  读取并清除中断状态、处理 RX、重启 RX
      └─ 按键／UART → 组包、发射、等待 TX 完成、重启 RX
```

初始化失败（`rf_err_mode = 1`）时，`XTAPP_Scan()` 会直接返回，不执行任何动作——
看到 `init fail` 请检查 SPI 接线与供电。

修改 RF 参数请用 EzToolkit 重新生成 `RF Drivers/ER4100Api/SPI_ER41xx_config.h`；
应用层参数则为 `RF_App.c`／`RF_App.h` 中的 `#define`。

---

## 透明模式

### TransTxRx
基本双向透明传输。
- **启动：** 开启 RX，`RX_TRANSPARENT`，固定接收长度 `RF_BUF_SIZE` = 64 字节。
- **TX：** 按下按键 → 发送 64 字节 `00 01 02 … 3F`，等待 TX 完成，打印 `TX(64):` + 十六进制
  内容及 `Tx done.`，松开按键后回到 RX。
- **RX：** RX 中断时读取 FIFO 与 RSSI，打印 `RX(64): rssi: -XX` + 十六进制内容。
- **用法：** 两块板烧录本工程，任一块按键，观察另一块的 UART 日志。

### TransTxRx_CCIT
在 TransTxRx 基础上加入软件帧格式与校验。
- **帧格式：** `[len][seq_hi][seq_lo][payload…][crc_lo][crc_hi]`
  - `len` = 序号（2）+ payload + CRC（2），不含自身
  - CRC-CCITT（多项式 0x8408，初值 0x0000），计算范围为序号 + payload
- **TX：** 按键 → payload 59 字节（`RF_BUF_SIZE − 5`），每按一次序号加 1。
- **RX：** 打印 `RX seq=N`，接着为 `CRC OK` + 十六进制内容，或 `CRC FAIL`／`RX: bad length`／
  `RX: len mismatch`。
- **用法：** 两块板均烧录 TransTxRx_CCIT。

### TransTxRx_CCIT_PER
使用 TransTxRx_CCIT 帧格式的误包率（PER）测试。
- **TX：** 按键 → 连续发送 `PER_TX_COUNT` = 100 包，间隔 `PER_TX_INTERVAL_MS` = 30 ms，
  每包打印一个 `.`。
- **RX：** 每收到一包打印 pass／error 计数、RSSI，以及目前的 PER
  `(100 − pass) / 100`（百分比，两位小数）。
- **用法：** 先复位 RX 板，再在 TX 板按一次按键，于 RX 板读取最终 PER。包数／间隔可在
  `RF_App.h` 修改。

### TransTxRx_LongPkt
利用 FIFO 阈值收发长包（512 字节，超过 128 字节 FIFO）。
- **需要**使用 `longPkt` 的 `.ini`（开启 TX FIFO 将空与 RX FIFO 将满中断，阈值 64 字节 =
  `RF_RX_ALMOST_FULL_THR`／`RF_TX_ALMOST_EMPTY_THR`）。
- **帧格式：** `[len_hi][len_lo][pkt_num][data…][crc_lo][crc_hi]`，总长 `RF_TX_SIZE` = 512。
- **TX：** 先填满 FIFO，以总长度启动 TX，每次将空中断补 64 字节，直到 TX 完成
  （约 250 ms 超时保护，超时打印 `TX Timeout!`）。
- **RX：** 每次将满中断读出 64 字节，RX 完成时读出剩余数据并校验 CRC，成功则打印
  `PktNum:` 与 512 字节内容。
- **用法：** 两块板均烧录 TransTxRx_LongPkt。

### TransWOR
无线唤醒（WOR）接收端（低功耗周期侦听）。
- **启动：** 等待 3 秒让 ER4100 稳定，然后启动 WOR。
- **WOR 时序：** 唤醒周期 `WUT_PERIOD_SEL` = 1 s（可选 10／50／100／500 ms、1／2／4 s）；
  RX 窗口 = `WOR_CNT` × `WOR_CNT_UNIT` = 4 × 20 ms = 80 ms。
- **RX：** 收到数据包时停止 WOR，打印 `RX(…)` + RSSI + 内容，然后重启 WOR。
  其他中断仅重置 RX FIFO 并重启 WOR。
- **用法：** 发射端为烧录 **TransTxRx** 且设定相同的板子。数据包需落在 RX 窗口内才会被
  接收；若无显示，请再按一次 TX 按键。

### TransWOR_Ack_CCIT
会校验 CCIT 帧并回复的 WOR 接收端。
- WOR 时序与 TransWOR 相同。
- **RX：** 进行 CRC-CCITT 校验（帧格式同 TransTxRx_CCIT）。`CRC OK` 时：等待 200 ms，
  发送 3 字节 `"ACK"`，打印 `Tx done.`，重启 WOR。错误帧直接丢弃。
- **用法：** 发射端为烧录 **TransTxRx_CCIT** 的板子，其 UART 日志会显示收到的 `ACK`。

---

## IEEE 802.15.4 模式

### 802154TxRx
- **启动：** 开启 RX，`RX_802154`。接收长度由 PHR 决定，因此长度参数为 0。
- **TX：** 按键 → 64 字节帧 = MHR 样本（`M802154_MHR_PATTERN`）+ payload，payload 第一个
  字节为序号。前导码、同步字、PHR 与 CRC 由硬件自动加入。
- **RX：** `INT_ST_RX` → 打印帧内容 + RSSI。`INT_ST_RXERR` → 打印 `RX Error` 及
  `Valid802154／CrcErr／FormatErr／FormatRej` 标志（寄存器 0xA004），并重置 FIFO。
- **用法：** 两块板均烧录 802154TxRx。

### 802154TxRx_Addressing
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

---

## 测试／工具

### SingleTone
- **启动：** 自动以设定的频率与功率发射单音（无调制载波）。
- **按键：** 切换载波关闭／开启（`Stop Single Tone!`／`Start Single Tone!`）。
- **用法：** 搭配频谱仪进行频偏（CapIO）校准与输出功率量测。

### RSSI_Scan
- **启动：** 不处理接收数据包；每 1 秒以 CCA 模式读取 RSSI 并打印 `rssi(dBm): -XX`。
- **用法：** 检查信道背景噪声／干扰，或量测附近的发射端（例如 SingleTone 板）。

### GIO_TRBSY
示范如何将内部状态信号输出到 ER4100 GPIO。
- **基础：** 与 TransTxRx 相同（按键 = 发送 64 字节）。
- **UART 命令：**
  - `0` : GPIO0 → Debug 模式（关闭 TR_BSY 输出）
  - `1` : GPIO0 → GIO 模式，选择 TR_BSY（`TRx_GPIO0_Sel(1, TR_BSY)`）
  - `2` : 开启 RX
  - `3` : 关闭 RX

- **用法：** 用示波器量测 ER4100 GPIO0。输入 `1` 后，TX／RX 进行期间 GPIO0 为高电平——
  按键可看到 TX 波形，输入 `2` 可看到 RX 窗口。
