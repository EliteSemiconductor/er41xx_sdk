# ER4100 EzFirm — NFC_StatusTrigger

## 概述

演示 NFC 磁场进入/离开检测与 USER_CFG4 状态触发的联动功能。
ER4100 作为 NFC 标签运行。检测到 NFC 磁场（通过 INT_N 中断）后，
IRQ 来源动态切换为 `NFC_USER_CFG4`，以便在 NFC 读写器写入特定指令时触发通知。
磁场离开通过 SPI 轮询 PWRGOOD 位检测，具有 50 次连续确认的去抖动机制。

**关键行为：**
- 启动时：IRQ 来源 = `NFC_PWRGOOD`，检测到磁场时 INT_N 触发
- 附着时：切换 IRQ 来源为 `NFC_USER_CFG4`，读写器可触发状态事件
- STATUS4 触发：NFC 读写器写入 `A2 FF 00 00 10 04`，将 DPE Status bit4 置 1 → INT_N 触发
- 离开时：SPI 轮询检测（连续 50 次 PWRGOOD=0），IRQ 来源重置为 `NFC_PWRGOOD`

**与 NFC_Field_Interrupt 的对比：**

| | NFC_StatusTrigger | NFC_Field_Interrupt |
|---|---|---|
| 附着检测 | INT_N（NFC_PWRGOOD） | INT_N（NFC_PWRGOOD） |
| 附着后行为 | 切换 IRQ 来源为 NFC_USER_CFG4 | 保持 NFC_PWRGOOD |
| STATUS4 触发 | 支持，USER_CFG4 触发 INT_N | 不支持 |
| 离开检测 | SPI PWRGOOD 轮询 + 50 次去抖动 | INT_N（PWRGOOD=0） |

---

## 程序流程

```
main()
 ├── InitialUART0_Timer1()       UART 115200 baud
 ├── XTAPP_Init()
 │    ├── TRx_IoConfig()         SPI / INT_N GPIO 设置
 │    ├── XTAPP_Strobe()         验证 ER4100 芯片 ID（0xF100 = 0x90_00_00）
 │    ├── TRx_SW_Reset()
 │    ├── TRx_Init()             加载 RF 寄存器默认值
 │    ├── 使能 NFC IRQ mask      写 0x00_02_00_00 → 0xF004
 │    └── XTAPP_NFC_Config()
 │         ├── NFC_Disable() / NFC_Enable()
 │         ├── NFC_SetLockBlock_0_15(0, 0, 15)     解锁全部
 │         ├── NFC_SetLockBlock_16_247(0, 16, 247)  解锁全部
 │         ├── NFC_Select_IRQ_Src(NFC_PWRGOOD)     等待磁场
 │         └── 写入默认头部块（Block 0~3）
 └── while(1)
      └── XTAPP_NFC_Scan()
           ├── （rf_err_mode = 1 时直接返回）
           ├── if NIRQ_Value == 0  （INT_N 低电平 = 中断触发）
           │    ├── TRx_GetIntStatus()     读取 0xF004
           │    ├── IS_INT_ST_NFC()?
           │    │    ├── if irq_src == NFC_PWRGOOD && PWRGOOD 位置 1
           │    │    │    ├── u8NfcPwrGood = 1，打印 "+--- NFC Attached ---+"
           │    │    │    ├── NFC_Select_IRQ_Src(NFC_USER_CFG4)
           │    │    │    └── XTAPP_NFC_PrintInstructions()
           │    │    └── else if irq_src == NFC_USER_CFG4
           │    │         └── 打印 "+--- STATUS4 Trigger---+"
           │    ├── NFC_Clear_NFC_Event()
           │    └── TRx_ClearIntFlag()
           └── if u8NfcPwrGood  （轮询离开）
                ├── if PWRGOOD 位 == 0
                │    ├── u8DettachedCnt++
                │    └── if u8DettachedCnt >= 50
                │         ├── u8NfcPwrGood = 0，打印 "+--- NFC Detached ---+"
                │         └── NFC_Select_IRQ_Src(NFC_PWRGOOD)
                └── else  （磁场仍存在）
                     └── u8DettachedCnt = 0
```

---

## STATUS4 触发方式

磁场附着后，IRQ 来源切换为 `NFC_USER_CFG4`（DPE Status bit4）。
要触发此事件，NFC 读写器/手机需发送以下 NFC 指令：

```
A2 FF 00 00 10 04
```

- `A2`    : WRITE 指令（ISO 15693 / NFC Forum T5T）
- `FF`    : 块地址 = 0xFF（Block 255 = DPE 寄存器 0x93FC）
- `00 00 10 04` : 数据字节——byte[2]（DPE Status）的 bit4 = USER_CFG4 = 1

DPE 检测到 USER_CFG4 上升沿时拉低 INT_N。MCU 随即打印：

```
+--- STATUS4 Trigger---+
```

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
- Block 4~15（0x9010~0x903C）：Data0~Data47（用户数据）
- Block 16~247（0x9040~0x93DC）：Data48~Data975（用户数据）
- Block 248~254（0x93E0~0x93F8）：动态锁定位
- Block 255（0x93FC）：DPE 控制 / 状态寄存器 ← 本 Demo 的核心寄存器

---

## DPE（数字协议引擎）

DPE 在 `0x93FC` 提供两个寄存器供 MCU 访问：

- Byte3 — DPE Control（由 MCU 写入）
- Byte2 — DPE Status（由 NFC 读写器写入，MCU 读取）

### DPE Control（0x93FC Byte3）

- Bit 7~5 : 保留
- Bit 4    : `DPE_rst`        — 置 1 时，DPE 保持复位状态
- Bit 3    : `clear/mask IRQ` — 置 1 时，清除并屏蔽当前中断；写 0 重新使能
- Bit 2~0  : `irq_src[2:0]`  — 选择哪个 DPE Status 位触发 INT_N 到 MCU（直接索引值 0~7）

### DPE Status（0x93FC Byte2）

由 NFC 读写器写入；当选定位的索引与 `irq_src` 匹配时触发 INT_N 到 MCU。

- Bit 7 : USER_CFG7
- Bit 6 : USER_CFG6
- Bit 5 : USER_CFG5
- Bit 4 : USER_CFG4  ← 磁场附着后本 Demo 等待此位
- Bit 3 : TX_REPLY   — DPE 正在发送回复
- Bit 2 : RX_CMD     — DPE 正在接收命令
- Bit 1 : PWRGOOD    — 检测到 NFC 磁场
- Bit 0 : IGNORED    — 无功能

---

## NFC IRQ 来源宏

定义于 `RF_NFC_APP.c`，作为 `NFC_Select_IRQ_Src()` 的输入参数。
注意：这些是**直接索引值**（0~7），不是位掩码：

- `NFC_IGNORED`   = 0 — 不触发中断
- `NFC_PWRGOOD`   = 1 — 检测到 NFC 磁场时触发  ← 启动及离开后使用
- `NFC_RX_CMD`    = 2 — 接收到命令时触发
- `NFC_TX_REPLY`  = 3 — 发送回复完成时触发
- `NFC_USER_CFG4` = 4 — 由读写器自定义配置  ← 磁场附着后切换至此

---

## 底层 API 函数

- `NFC_Enable()`
  使能 NFC 模式（向寄存器 0x0030 写入 0x00000000）。

- `NFC_Disable()`
  关闭 NFC 模式（向寄存器 0x0030 写入 0x00000001）。

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  设置 DPE Control（0x93FC Byte3）的 `irq_src[2:0]`，选择触发 INT_N 的事件源。
  本 Demo 在每次附着/离开周期中调用两次：附着时切换为 `NFC_USER_CFG4`，离开后切换回 `NFC_PWRGOOD`。

- `NFC_Get_IRQ_Src_Select()`
  返回当前 DPE Control 字节中的 `irq_src[2:0]` 值。
  在 `XTAPP_NFC_Scan()` 中用于区分附着中断与 STATUS4 中断。

- `NFC_Clear_NFC_Event()`
  清除当前 NFC 中断：
  1. 将 DPE Status 字节（Byte2）写 0x00，确认事件
  2. 置位 `clear/mask` 位（Byte3 bit3），触发中断清除
  3. 等待 1 ms
  4. 清除 `clear/mask` 位，重新使能中断

- `NFC_SetLockBlock_0_15(uint8_t value, uint16_t start_block, uint8_t end_block)`
  通过 Block 2（0x9008）的 Lock0/Lock1 字节设置或清除 Block 0~15 的锁定位。

- `NFC_SetLockBlock_16_247(uint8_t value, uint16_t start_block, uint8_t end_block)`
  通过寄存器 0x93E0~0x93FC 设置或清除 Block 16~247 的动态锁定位。

---

## 应用层函数（RF_NFC_APP.c）

- `XTAPP_NFC_Config()`
  初始化 NFC 标签：
  - 关闭再重新使能 NFC 引擎
  - 解锁所有块（Block 0~247）
  - 将 IRQ 来源设置为 `NFC_PWRGOOD`（启动时等待磁场）
  - 写入默认头部块（Block 0~3）

- `XTAPP_Init()`
  初始化射频收发器与 NFC。成功返回 0，失败返回 1。
  向 0xF004 写入 `0x00020000` 以使能 NFC 中断掩码。

- `XTAPP_Get_NFC_Statu_Reg()`
  读取 DPE Status 字节（0x93FC Byte2）并返回 PWRGOOD 位。
  磁场存在时返回非零值。

- `XTAPP_NFC_PrintInstructions()`
  向 UART 打印 STATUS4 触发指令说明，提示用户如何从读写器/手机发送触发命令。

- `XTAPP_NFC_Scan()`
  主循环处理函数，同时处理 INT_N 中断事件与 SPI PWRGOOD 轮询：
  - **附着**（irq_src == NFC_PWRGOOD 且 PWRGOOD 位置 1）：打印 `"+--- NFC Attached ---+"`，切换 IRQ 来源为 `NFC_USER_CFG4`，打印操作说明
  - **STATUS4**（irq_src == NFC_USER_CFG4）：打印 `"+--- STATUS4 Trigger---+"`
  - **离开**（轮询）：每次读取到 PWRGOOD 低时递增 `u8DettachedCnt`，连续 50 次后确认离开，打印 `"+--- NFC Detached ---+"`，IRQ 来源重置为 `NFC_PWRGOOD`
