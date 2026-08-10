# ER4100 SDK_EZ

ER4100（ESMT）射频收发器 SDK 的发布根目录：嵌入式固件示例 + PC 端寄存器配置工具。

---

## 内容说明

- **[`EzFirm/`](EzFirm/README_SC.md)** — 基于 Nuvoton N76E003（8051）MCU 的嵌入式固件 SDK，
  共 18 个示例工程，涵盖透明传输、IEEE 802.15.4、NFC 及工具/测试模式（WOR、PER、CCA、
  RSSI 扫描、省电模式等）。完整工程列表、硬件平台细节、目录结构与构建方式请见
  `EzFirm/README_SC.md`。
- **`EzCodeGen2_20260805_V2.2.exe`** — PC 端配置工具（前身为 EzGen）。读取工程的 `.ini`
  文件，生成对应的 `SPI_ER41xx_config.h`（RF 寄存器表）。EzGen V2.1 以来的变化请见
  `revise_sc.txt`。
- **`SPI_ER41xx_config_EzGen2.2.h`** — EzCodeGen V2.2 生成的样本头文件，保留在此作为新版
  `_D_CODE` 格式的参考。
- **[`revise_sc.txt`](revise_sc.txt)** — 版本说明：SDK_EZ 相对于前一版利尔达定制交付版的
  差异，以及 EzGen → EzCodeGen 工具更新。

---

## 快速开始

1. 在 `EzFirm/ER4100_EzFirm_<Variant>/` 下选择一个工程。
2. 用 Keil uVision（8051 工具链）打开 `Project/*.uvproj`，全部编译（F7）。
3. 通过 Nu-Link 烧录 `Project/Output/*.hex`。
4. 如需重新生成工程的 `SPI_ER41xx_config.h`，用 `EzCodeGen2_20260805_V2.2.exe` 打开对应
   `.ini` 文件并导出头文件。
