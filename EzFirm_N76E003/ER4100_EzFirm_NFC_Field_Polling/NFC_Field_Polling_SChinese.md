# ER4100 EzFirm — NFC_Field_Polling

## 概述

演示使用 ER4100 射频收发器在 NFC 模式下检测 NFC 磁场进入与离开。
MCU 通过 SPI 直接读取 DPE Status 寄存器中的 PWRGOOD 位来感知磁场状态，不使用中断引脚。

**关键行为：**
- IRQ 来源设置为 `NFC_IGNORED`，不使用 INT_N 引脚
- 主循环每次调用 `XTAPP_NFC_Scan()` 轮询 PWRGOOD 位
- 离开检测具有去抖动机制，需连续 50 次读取到磁场消失才确认（`DETACHED_CNT_THR = 50`）

**与 NFC_Field_Interrupt 的对比：**

| | NFC_Field_Polling | NFC_Field_Interrupt |
|---|---|---|
| 检测方式 | SPI 寄存器轮询 | INT_N 引脚中断 |
| IRQ 来源 | `NFC_IGNORED` | `NFC_PWRGOOD` |
| 0xF004 中断掩码 | `0x00000000`（禁用） | `0x00020000`（启用） |
| 离开去抖动 | 连续 50 次确认 | 无 |

---

## 程序流程

```
main()
 ├── InitialUART0_Timer1()       UART 115200 baud
 ├── XTAPP_Init()
 │    ├── TRx_IoConfig()         SPI GPIO 设置
 │    ├── XTAPP_Strobe()         验证 ER4100 芯片 ID（0xF100 = 0x90_00_00）
 │    ├── TRx_SW_Reset()
 │    ├── TRx_Init()             加载 RF 寄存器默认值
 │    ├── 禁用 NFC IRQ mask      写 0x00_00_00_00 → 0xF004
 │    └── XTAPP_NFC_Config()
 │         ├── NFC_Disable() / NFC_Enable()
 │         ├── NFC_SetLockBlock_0_15(0, 0, 15)     解锁全部
 │         ├── NFC_SetLockBlock_16_247(0, 16, 247)  解锁全部
 │         ├── NFC_Select_IRQ_Src(NFC_IGNORED)     不触发中断
 │         └── 写入默认头部块（Block 0~3）
 └── while(1)
      └── XTAPP_NFC_Scan()
           ├── （rf_err_mode = 1 时直接返回）
           ├── if !u8NfcPwrGood && XTAPP_Get_NFC_Statu_Reg()
           │    └── u8NfcPwrGood = 1，打印 "+--- NFC Attached ---+"
           └── else if u8NfcPwrGood
                ├── if !XTAPP_Get_NFC_Statu_Reg()
                │    ├── u8DettachedCnt++
                │    └── if u8DettachedCnt >= DETACHED_CNT_THR(50)
                │         └── u8NfcPwrGood = 0，打印 "+--- NFC Detached ---+"
                └── else  （磁场仍存在）
                     └── u8DettachedCnt = 0   （重置去抖动计数）
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

由 NFC 读写器写入。轮询模式下，MCU 通过 SPI 直接读取此字节。

- Bit 7 : USER_CFG7
- Bit 6 : USER_CFG6
- Bit 5 : USER_CFG5
- Bit 4 : USER_CFG4
- Bit 3 : TX_REPLY   — DPE 正在发送回复
- Bit 2 : RX_CMD     — DPE 正在接收命令
- Bit 1 : PWRGOOD    — 检测到 NFC 磁场  ← 本 Demo 直接轮询此位
- Bit 0 : IGNORED    — 无功能

---

## 底层 API 函数

- `NFC_Enable()`
  使能 NFC 模式（向寄存器 0x0030 写入 0x00000000）。

- `NFC_Disable()`
  关闭 NFC 模式（向寄存器 0x0030 写入 0x00000001）。

- `NFC_Select_IRQ_Src(uint8_t u8IrqSrcVal)`
  设置 DPE Control（0x93FC Byte3）的 `irq_src[2:0]`。
  本 Demo 设置为 `NFC_IGNORED`，不使用 INT_N 引脚。

- `NFC_Get_IRQ_Src_Select()`
  返回当前 DPE Control 字节中的 `irq_src[2:0]` 值。

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
  - 将 IRQ 来源设置为 `NFC_IGNORED`（轮询模式，不使用 INT_N）
  - 写入默认头部块（Block 0~3）

- `XTAPP_Init()`
  初始化射频收发器与 NFC。成功返回 0，失败返回 1。
  向 0xF004 写入 `0x00000000` 以禁用 NFC 中断掩码。

- `XTAPP_Get_NFC_Statu_Reg()`
  读取 DPE Status 字节（0x93FC Byte2）并返回 PWRGOOD 位。
  磁场存在时返回非零值。

- `XTAPP_NFC_Scan()`
  主循环处理函数。通过 SPI 轮询 PWRGOOD 位检测磁场变化：
  - **附着**：`u8NfcPwrGood` 从 0→1，打印 `"+--- NFC Attached ---+"`
  - **离开**：每次读取到 PWRGOOD 低时递增 `u8DettachedCnt`，
    连续 50 次后才确认离开，打印 `"+--- NFC Detached ---+"`
  - 磁场重新出现时将 `u8DettachedCnt` 重置为 0（去抖动重置）
