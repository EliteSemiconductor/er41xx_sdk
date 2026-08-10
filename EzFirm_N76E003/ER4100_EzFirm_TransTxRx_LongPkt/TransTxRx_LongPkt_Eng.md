# ER4100_EzFirm_TransTxRx_LongPkt

## Overview

This project demonstrates **Long Packet Transparent Mode** transmission and reception using the ER4100 RF transceiver on an N76E003 (8051) MCU (up to 2047 bytes per packet).

Standard transparent mode is limited by FIFO size (128 bytes). Long packet mode overcomes this by moving data out of the RX FIFO when it reaches the almost-full threshold, and refilling the TX FIFO in segments until the full packet is sent. Each packet embeds a CRC-16/CCITT-Kermit checksum for integrity verification on the receiver side.

---

## 目錄樹

```
ER4100_EzFirm_TransTxRx/
│
├── Project/                          # Keil 專案檔
│   ├── TransTxRx.uvproj             # 主專案檔
│   ├── TransTxRx.uvopt              # 專案選項
│   ├── STARTUP.A51                  # 8051 啟動組語
│   ├── Nu_Link_8051_Driver.ini      # 燒錄器設定
│   ├── LST/                         # 編譯器輸出列表檔 (*.lst, *.map)
│   └── Output/                      # 專案輸出 (*.obj, *.hex, *.lnp)
│
├── Include/                          # MCU 系統標頭檔（唯讀，勿修改）
│   ├── N76E003.h                    # MCU 暫存器定義
│   ├── SFR_Macro.h                  # SFR 操作巨集
│   └── Function_Define.h            # MCU 函式定義
│
├── Common/                           # 共用函式庫
│   ├── Common.h                     # 專案總 include 入口：typedef、MCU header、所有模組 header
│   ├── Delay.h / Delay.c            # 延遲函式
│   └── Uart.h / Uart.c              # UART 收發函式
│
├── User/                             # 使用者應用層
│   └── main.c                       # 主程式：初始化、主迴圈
│
└── RF Drivers/                       # RF 驅動層
    ├── RF_Hal.h / RF_Hal.c          # RF HAL 層：腳位定義、SPI bit-bang 實作
    ├── RF_App.h / RF_App.c          # RF 應用層：初始化、收發、IRQ 處理
    │
    └── XtLibSrc/                    # ER4100 底層 SPI API（ESMT 原廠提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、暫存器操作、FIFO 管理
        └── SPI_ER41xx_config.h      # RF 暫存器初始值配置（由 EzGen 工具產生，當前使用）
```

---

## 層次架構

```
User Application
  main.c
    │  初始化、主迴圈
    ▼
RF Application Layer
  RF_App.c / RF_App.h
    │  封包收發、IRQ 處理、RX buffer 管理
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（暫存器配置）
    │  暫存器讀寫、FIFO 操作、模式切換
    │  呼叫 TRx_Write / TRx_Read 進行實際 SPI 傳輸
    ▼
RF HAL Layer
  RF_Hal.c / RF_Hal.h
    │  SPI 時序 (Software bit-bang)、腳位定義
    ▼
Hardware
  N76E003 GPIO → ER4100 RF IC (433 MHz)
```

---

## 延伸說明

### Common.h 作為總入口
所有 `.c` 只需 `#include "Common.h"` 即可取得全部 typedef、MCU header 及模組宣告。

### SPI 實作
使用 **Software SPI (GPIO bit-bang)**，腳位定義集中於 `RF_Hal.h`：

[SPI Pin Assignment]
  NIRQ  P03  中斷輸入
  MOSI  P00  SPI 資料輸出
  MISO  P01  SPI 資料輸入
  SCK   P10  SPI 時脈
  NSS   P15  SPI 片選 (CS)

### RF_RX_SIZE
`RF_App.c` 中的 `RF_RX_SIZE` 為傳入 `TRx_RX_Trigger()` 的期望接收長度：
- **802.15.4 模式**：此值不影響接收，MAC 層從 PHR 自動取得封包長度
- **Transparent 模式**：須填寫實際期望長度（standard: 1~128、long packet: 1~2047）

### 開機 UART 訊息

開機時 `main.c` 透過 UART 輸出專案名稱及初始化結果：

  "TransTxRx_LongPkt : init done\r\n"  — 初始化成功
  "TransTxRx_LongPkt : init fail\r\n"  — 初始化失敗

### XTAPP_Scan() 初始化保護

`XTAPP_Scan()` 開頭會檢查 `rf_err_mode`，若初始化失敗（`rf_err_mode == 1`）則立即返回，避免對未初始化的 RF 晶片進行存取。

---

## Keil 包含路徑（Include Paths）

```
..\Include
..\Common
..\User
..\RF Drivers
..\RF Drivers\XtLibSrc
```

優先順序：`..\Common` 在 `..\User` 之前，確保 `Common/Common.h` 優先於 `User/Common.h`（若存在）。
