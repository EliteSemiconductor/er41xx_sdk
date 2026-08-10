# ER4100_EzFirm_PowerSaving_WUT 專案結構

## 概述

此專案示範 ER4100 RF 收發器的 **WUT（喚醒定時器）單次觸發模式（Oneshot Mode）**。

初始化後，晶片立即進入深度睡眠（Deep Sleep）。WUT 定時器在設定時間後（預設 2 秒）觸發，對 MCU 發出 NIRQ 訊號。MCU 喚醒後透過 UART 輸出訊息，重新設定 WUT 定時器，再次進入深度睡眠，如此循環。

本專案不含任何 RF 封包收發（TX/RX），僅示範 WUT 定時器與深度睡眠的操作流程。

> **注意：** 必須在 `SPI_ER41xx_config.h` 中定義 `EXTERN_RF_APIS_WUT`，才能啟用 WUT 相關 API（`TRx_SetWUTMR_Timer`、`TRx_PowerDownMode` 等）。

---

## 目錄樹

```
ER4100_EzFirm_PowerSaving_WUT/
│
├── Project/                          # Keil 專案檔
│   ├── TransWOR.uvproj              # 主專案檔
│   ├── TransWOR.uvopt               # 專案選項
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
│   └── main.c                       # 主程式：GPIO 初始化、系統初始化、主迴圈
│
└── RF Drivers/                       # RF 驅動層
    ├── RF_Hal.h / RF_Hal.c          # RF HAL 層：腳位定義、SPI bit-bang 實作
    ├── RF_App.h / RF_App.c          # RF 應用層：WUT 初始化、深度睡眠進入、IRQ 處理
    │
    └── ER4100Api/                   # ER4100 底層 SPI API（ESMT 原廠提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、暫存器操作、模式切換
        └── SPI_ER41xx_config.h      # RF 暫存器初始值配置（由 EzGen 工具產生）
```

---

## 層次架構

```
User Application
  main.c
    │  GPIO 初始化、系統初始化、主迴圈
    ▼
RF Application Layer
  RF_App.c / RF_App.h
    │  WUT 定時器配置、深度睡眠進入、NIRQ 喚醒處理
    ▼
ER4100 SPI API
  SPI_ER41xx.c / SPI_ER41xx.h  ← SPI_ER41xx_config.h（暫存器配置）
    │  暫存器讀寫、模式切換、WUT 定時器控制
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
  NIRQ  P03  中斷輸入（WUT 喚醒訊號）
  MOSI  P00  SPI 資料輸出
  MISO  P01  SPI 資料輸入
  SCK   P10  SPI 時脈
  NSS   P15  SPI 片選 (CS)

### WUT（喚醒定時器）— 單次觸發模式

WUT 定時器設定為**單次觸發模式（Oneshot）**：定時器到期後觸發一次 NIRQ 並停止，每次喚醒後由韌體手動重新啟動。

[WUT 流程]
  XTAPP_Init()
    │  TRx_SetWUTMR_Timer(period, 0)  ← 單次模式 (repeat=0)
    │  TRx_PowerDownMode()            ← 進入深度睡眠
    ▼
  （晶片睡眠 WUT_PERIOD 時間）
    ▼
  NIRQ 觸發 → 呼叫 XTAPP_Scan()
    │  TRx_GetIntStatus() → IS_INT_ST_WAKEUP
    │  UART0_SendStr("Wakeup...")
    │  TRx_SetWUTMR_Timer(period, 0)  ← 重新啟動單次定時器
    │  TRx_PowerDownMode()            ← 再次進入深度睡眠
    ▼
  （重複循環）

[WUT 週期巨集]
  SET_WUT_PERIORD_ARRAY_10MS   0x00000147   ~10 ms
  SET_WUT_PERIORD_ARRAY_50MS   0x00000666   ~50 ms
  SET_WUT_PERIORD_ARRAY_100MS  0x00000CCC   ~100 ms
  SET_WUT_PERIORD_ARRAY_500MS  0x00004000   ~500 ms
  SET_WUT_PERIORD_ARRAY_1S     0x00008000   ~1000 ms
  SET_WUT_PERIORD_ARRAY_2S     0x00010000   ~2000 ms
  SET_WUT_PERIORD_ARRAY_4S     0x00020000   ~4000 ms

  WUT_PERIOD_SEL 選擇使用的週期，預設為 SET_WUT_PERIORD_ARRAY_2S（2 秒）。

週期計算公式：Register_Value = (Period_ms × 32768) / 1000

### 開機 UART 訊息

開機時 `main.c` 透過 UART 輸出專案名稱及初始化結果：

  "PowerSaving_WUT : init done\r\n"  — 初始化成功
  "PowerSaving_WUT : init fail\r\n"  — 初始化失敗

### XTAPP_Scan() 初始化保護

`XTAPP_Scan()` 開頭檢查 `rf_err_mode`，若初始化失敗（`rf_err_mode == 1`）則立即返回，防止對未初始化的 RF 芯片進行訪問。

### EXTERN_RF_APIS_WUT

WUT 相關 API（`TRx_SetWUTMR_Timer`、`TRx_PowerDownMode`）需在 `SPI_ER41xx_config.h` 中定義以下巨集才能啟用：

```c
#define EXTERN_RF_APIS_WUT
```

未定義此巨集時，WUT 函式不會被編譯，連結器將報告符號缺失。

### IS_INT_ST_TMRTHD 與 IS_INT_ST_WAKEUP

WUT 喚醒時可能觸發兩個中斷旗標：

[中斷旗標]
  IS_INT_ST_TMRTHD  : 定時器閾值到達（可選，可忽略）
  IS_INT_ST_WAKEUP  : 晶片喚醒事件 — 用於偵測實際喚醒

韌體收到 TMRTHD 時輸出 "Wakeup Timer\r\n"，收到 WAKEUP 時輸出 "Wakeup, and enter deep sleep again...\r\n"。

---

## Keil 包含路徑（Include Paths）

```
..\Include
..\Common
..\User
..\RF Drivers
..\RF Drivers\ER4100Api
```

優先順序：`..\Common` 在 `..\User` 之前，確保 `Common/Common.h` 優先於 `User/Common.h`（若存在）。
