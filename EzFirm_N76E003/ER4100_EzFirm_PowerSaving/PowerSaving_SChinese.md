# ER4100_EzFirm_PowerSaving 專案結構
此專案提供休眠及喚醒的操作，透過P05觸發。

## 目錄樹

```
ER4100_EzFirm_PowerSaving/
│
├── Project/                          # Keil 專案檔
│   ├── PowerSaving.uvproj           # 主專案檔
│   ├── PowerSaving.uvopt            # 專案選項
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
    ├── RF_App.h / RF_App.c          # RF 應用層：初始化、掃描、IRQ 處理、電源管理
    │
    └── ER4100Api/                   # ER4100 底層 SPI API（ESMT 原廠提供）
        ├── SPI_ER41xx.h / .c        # SPI 指令集、暫存器操作、FIFO 管理
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
    │  封包收發、IRQ 處理、電源管理狀態機
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

### GPIO — 應用腳位

[GPIO — 應用腳位]
  P05  Push-pull  電源管理按鈕（低電位觸發）

P07 使用 `Enable_BIT7_FallEdge_Trig` + `Enable_INT_Port0` + `set_EPI` 設定硬體腳位中斷。

### 電源管理

`RF_App.c` 實作兩狀態電源管理狀態機：

[電源管理狀態]
  `active`    : RF 晶片正常運作
  `deepsleep` : RF 晶片透過 `TRx_PowerDownMode()` 進入省電模式

**狀態轉換：**
- `active → deepsleep`：active 狀態下按下 P05
- `deepsleep → active`：RF 晶片透過 NIRQ 觸發 `IS_INT_ST_WAKEUP` 事件

**按鈕去彈跳：** P05 動作後，`while(P05==0)` 等待放開，再接 `Delay_ms(20)` 消除放開彈跳。
### 開機 UART 訊息

開機時 `main.c` 透過 UART 輸出專案名稱及初始化結果：

  "PowerSaving : init done\r\n"  — 初始化成功
  "PowerSaving : init fail\r\n"  — 初始化失敗

### XTAPP_Scan() 初始化保護

`XTAPP_Scan()` 開頭會檢查 `rf_err_mode`，若初始化失敗（`rf_err_mode == 1`）則立即返回，避免對未初始化的 RF 晶片進行存取。

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
