# ER4100 EzFirm — NFC_Menu

## 概述

演示使用 ER4100 射频收发器在 NFC 模式下进行 NFC 标签读写操作。
ER4100 作为 NFC 标签运行，NFC 读写器（手机/读卡器）可访问其内存块。
MCU 通过 SPI 接口对标签进行配置和监控。

NFC 磁场附着时，MCU 通过 UART 打印交互式操作菜单。开发者可发送指令
对数据块进行读写、读取 DPE 寄存器，或配置 IRQ 来源以测试
读写器触发的状态事件（USER_CFG4~7）。

---

## NFC 内存映射

NFC 内存范围从 `0x9000` 到 `0x93FC`，共 256 个块，每块 4 字节。

- MCU Addr（Hex）：ER4100 SPI 总线上的绝对寄存器地址
- Host Block（Dec）：NFC 读写器可见的块编号
- 字节以小端序存储（Byte0 = LSB，位于最低地址）

**块布局：**

- Block 0（0x9000）：UID（Byte0~3）
- Block 1（0x9004）：UID（Byte0~3）
- Block 2（0x9008）：BCC1 / Internal / Lock0 / Lock1
  - Lock0（Byte2，bit0~7）：Block 0~7 的锁定位
  - Lock1（Byte3，bit0~7）：Block 8~15 的锁定位
- Block 3（0x900C）：CC0 / CC1 / CC2 / CC3（Capability Container）
- Block 4~15（0x9010~0x903C）：Data0~Data47（用户数据，由 Lock1 控制锁定）
- Block 16~247（0x9040~0x93DC）：Data48~Data975（用户数据，由动态锁定位控制）
- Block 248~254（0x93E0~0x93F8）：动态锁定位
  - 每个 32 位字控制 32 个块
  - 0x93E0 bit0~31：控制 Block 16~47
  - 0x93E4 bit0~31：控制 Block 48~79
  - …（每个寄存器控制 32 个块）
  - 0x93F8 bit0~31：控制 Block 208~239
- Block 255（0x93FC）：特殊寄存器
  - Byte0（RFU）：保留
  - Byte1（动态锁定，bit0~7）：控制 Block 240~247
  - Byte2（DPE Status）：NFC 中断状态（由 NFC 读写器写入，MCU 读取）
  - Byte3（DPE Control）：NFC 中断控制（由 MCU 写入）

---

## DPE（数字协议引擎）

DPE 在 `0x93FC` 提供两个寄存器供 MCU 访问：

- Byte3 — DPE Control（由 MCU 写入）
- Byte2 — DPE Status（由 NFC 读写器写入，MCU 读取）

### DPE Control（0x93FC Byte3）

- Bit 7~5 : 保留
- Bit 4    : `DPE_rst`        — 置 1 时，DPE 保持复位状态
- Bit 3    : `clear/mask IRQ` — 置 1 时，清除并屏蔽当前所有中断，直到该位重新置 0
- Bit 2~0  : `irq_src[2:0]`  — 选择哪个 DPE Status 位触发 INT_N 到 MCU（直接索引值 0~7）
  - `irq_src = 1` → PWRGOOD（检测到 NFC 磁场/磁场消失）
  - `irq_src = 4` → USER_CFG4

### DPE Status（0x93FC Byte2）

由 DPE 在对应事件发生时置位；当选定位的索引与 `irq_src` 匹配时触发 INT_N 到 MCU。
MCU 读取该字节以判断触发事件，然后通过 `NFC_Clear_NFC_Event()` 进行清除：
1. 通过 SPI 将 Byte2 写入 0x00，清除状态标志
2. 置位 `clear/mask` 位（Byte3 bit3），释放 INT_N 信号线
3. 等待 1ms 后清除 `clear/mask` 位，重新使能中断

- Bit 7 : USER_CFG7  — DPE 置位，触发 MCU 中断
- Bit 6 : USER_CFG6  — DPE 置位，触发 MCU 中断
- Bit 5 : USER_CFG5  — DPE 置位，触发 MCU 中断
- Bit 4 : USER_CFG4  — DPE 置位，触发 MCU 中断
- Bit 3 : TX_REPLY   — DPE 正在发送回复
- Bit 2 : RX_CMD     — DPE 正在接收命令
- Bit 1 : PWRGOOD    — 检测到 NFC 磁场
- Bit 0 : IGNORED    — 无功能

---

## NFC IRQ 来源宏

定义于 `RF_NFC_APP.h`，作为 `NFC_Select_IRQ_Src()` 的输入参数。
注意：这些是**直接索引值**（0~7），不是位掩码：

- `NFC_IGNORED`   = 0 — 不触发中断
- `NFC_PWRGOOD`   = 1 — 检测到 NFC 磁场/磁场消失时触发
- `NFC_RX_CMD`    = 2 — 接收到命令时触发
- `NFC_TX_REPLY`  = 3 — 发送回复完成时触发
- `NFC_USER_CFG4` = 4 — 用户自定义
- `NFC_USER_CFG5` = 5 — 用户自定义
- `NFC_USER_CFG6` = 6 — 用户自定义
- `NFC_USER_CFG7` = 7 — 用户自定义

DPE Status 位掩码（`NFC_PWRGOOD_MASK`、`NFC_USER_CFG4_MASK` 等）同样定义于 `RF_NFC_APP.h`，
用于从 Status 字节中读取各事件标志位。

---

## 底层 API 函数（RF_NFC_APP.c）

### 底层 NFC 控制

- `NFC_Enable()`
  使能 NFC 模式（向寄存器 0x0030 写入 0x00000000）。

- `NFC_Disable()`
  关闭 NFC 模式（向寄存器 0x0030 写入 0x00000001）。

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  设置 DPE Control（0x93FC Byte3）的 `irq_src[2:0]`，选择触发 INT_N 的事件源。
  输入：直接索引值 0~7（使用 `NFC_IGNORED` ~ `NFC_USER_CFG7`）。

- `NFC_Get_IRQ_Src_Select()`
  返回当前 DPE Control 字节中的 `irq_src[2:0]` 值。

- `NFC_Clear_NFC_Event()`
  通过置位再清除 `clear/mask` 位（Byte3 bit3）来清除当前 NFC 中断。
  置位与清除之间有 1ms 延迟，确保 DPE 完成中断清除。

- `NFC_Set_Impedance(uint8_t u8IMP_0_7)`
  通过寄存器 0x1700 bit[12:10] 设置 NFC 天线阻抗（0~7）。

- `NFC_Get_Impedance()`
  返回当前 NFC 天线阻抗设置值（0~7）。

- `NFC_SetLockBlock_0_15(uint8_t value, uint16_t start_block, uint8_t end_block)`
  通过 Block 2（0x9008）的 Lock0/Lock1 字节设置或清除 Block 0~15 的锁定位。

- `NFC_SetLockBlock_16_247(uint8_t value, uint16_t start_block, uint8_t end_block)`
  通过寄存器 0x93E0~0x93FC 设置或清除 Block 16~247 的动态锁定位。
  保护 0x93FC 中的 DPE 寄存器（仅允许修改 Byte1 的 bit0~7）。

### 应用层函数

- `XTAPP_NFC_Config()`
  初始化 NFC 标签：
  - 复位 DPE，关闭再重新使能 NFC 引擎
  - 解锁所有块（Block 0~247），允许读写器访问
  - 设置 DPE 等待 NFC 磁场（`NFC_PWRGOOD`）
  - 写入默认头部块（Block 0~3）

- `XTAPP_NFC_IRQ_Parser()`
  在 `XTAPP_Scan` 中检测到 NFC 中断时调用。读取 DPE 块，根据 `irq_src` 分发处理：
  - `NFC_PWRGOOD`：置位 `u8NfcPwrGood = 1`，将 irq_src 切换为 `NFC_IGNORED` 防止
    磁场存在期间持续重复触发，然后打印 UART 操作菜单
  - `NFC_USER_CFG4~7`：打印 DPE Status 字节值，将 irq_src 切回 `NFC_IGNORED`
  - 最后统一调用 `NFC_Clear_NFC_Event()` 清除中断；返回本次触发的 `irq_src` 索引值

- `XTAPP_NFC_Task()`
  处理一个 UART 命令字符（非阻塞，直接检查 `RI` 标志）：
  - `'1'`：MCU 向数据块写入测试数据（0x01~0x40）（Block 4~19）
  - `'2'`：MCU 从数据块读取数据（Block 4~19）并打印
  - `'3'`：打印 DPE Control 和 Status 寄存器字节
  - `'4'`：设置 irq_src = USER_CFG4，等待读写器触发 status4
  - `'5'`：设置 irq_src = USER_CFG5，等待读写器触发 status5
  - `'6'`：设置 irq_src = USER_CFG6，等待读写器触发 status6
  - `'7'`：设置 irq_src = USER_CFG7，等待读写器触发 status7
  - `'a'`：将所有块（Block 0~255）通过 UART 打印输出

- `XTAPP_Scan()`
  主循环 RF 处理函数：
  1. 若 `rf_err_mode == 1` 则立即返回
  2. 检测到 NIRQ 有效时：读取并清除中断状态；若为 NFC 中断，调用 `XTAPP_NFC_IRQ_Parser()`
  3. 进入 `while(u8NfcPwrGood)` 循环：轮询 PWRGOOD 位检测磁场消失
     （通过 `DETTACHED_CNT_THR` 防抖），每次迭代调用 `XTAPP_NFC_Task()`
  4. 检测到磁场消失：清除 `u8NfcPwrGood`，重新设置 `irq_src = NFC_PWRGOOD`，打印 `NFC Detached`
