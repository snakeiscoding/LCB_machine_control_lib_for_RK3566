# liblcb_protocol 使用說明（RK3566 / aarch64 Linux）

`liblcb_protocol.so` 是 JHT 健身器材 LCB（Lower Control Board，下控板）的通訊與機種控制函式庫。應用程式透過 RS-485 與 LCB 溝通，可以走主機板 UART（例如 `/dev/ttyS4`），也可以走 USB 轉 RS485 轉接器（CH340/CH341，見 7.6）。函式庫負責封包、CRC、送出佇列、回應解析、LCB 初始化握手與各機種的輪詢，應用程式只需要開啟連線並呼叫 C API。

## 1. 套件內容

| 路徑 | 說明 |
|------|------|
| `lib/liblcb_protocol.so` | 函式庫本體（aarch64、已 strip、SONAME `liblcb_protocol.so`） |
| `include/lcb_protocol/<階層>/*.h` | 對外 header（49 個），依 5 階層架構分目錄，見 1.1 |
| `example/lcb_demo.c`、`example/Makefile` | 可直接編譯的範例：開啟 UART 或 USB RS485 → 握手 → 交棒給機種 handler → 結束 |
| `BUILD_INFO.txt` | 這份 `.so` 的編譯時間、編譯器、相依函式庫、glibc 需求與 SHA256 |
| `README.md` | 本文件 |

### 1.1 標頭檔目錄（5 階層架構）

```
include/lcb_protocol/
├── capabilities/            Layer_1  機型能力與參數（13 個）
├── adapters/                Layer_2  機種安全／轉換層（3 個）
├── control/                 Layer_3  機台控制（1 個）
├── protocol/
│   ├── MachineHandler/      Layer_3  各機種 handler（11 個）
│   └── Comm/                Layer_4  LCB 通訊協定核心（16 個）
├── hardware/                Layer_5  USB 轉 RS485（CH340/CH341）（4 個，另有 README.md）
└── third_party/cJSON/       內含的 cJSON（1 個）
```

| 階層 | 目錄 | Header | 用途 |
|------|------|--------|------|
| Layer_1 | `capabilities/` | `frame_config_registry.h`、`frame_config_json_loader.h`、`frame_config_spec.h`、`frame_config_data.h`、`frame_config_data_treadmill.h`、`frame_config_data_bike.h`、`frame_config.h`、`frame_config_limits.h`、`machine_config.h`、`machine_category.h`、`value_range.h`、`calibration_table.h`、`treadmill_machine_convert.h` | 各機型的速度／升降範圍等參數（可由 `frame_configs.json` 載入）、單位換算、校正表 |
| Layer_2 | `adapters/` | `treadmill_interface.h`、`treadmill_machine_adapter.h`、`tcv_fx_spin_bike_interface.h` | 包在機種 handler 外層的安全邏輯與單位轉換（跑步機 safety key／E-Stop 等） |
| Layer_3 | `control/` | `treadmill_control.h` | 跑步機工程單位控制：km/h、坡度 %、開始／暫停／E-Stop、校正 |
| Layer_3 | `protocol/MachineHandler/` | `general_handler.h`、`treadmill_handler.h`、`hybrid_tm_handler.h`、`bike_handler.h`、`indoor_bike_handler.h`、`tcv_fx_spin_bike_handler.h`、`elliptical_handler.h`、`ascent_handler.h`、`stepper_handler.h`、`en_climb_handler.h`、`jht_ad_board_handler.h` | LCB 握手與各機種的指令、輪詢、狀態機 |
| Layer_4 | `protocol/Comm/` | `lcb_comm.h`、`lcb_comm_callback.h`、`lcb_commands.h`、`lcb_constants.h`、`lcb_type.h`、`lcb_proc_comm.h`、`lcb_logging.h`、`lcb_device_data.h`、`lcb_universal_data.h`、`lcb_packet.h`、`lcb_packet_queue.h`、`lcb_parser.h`、`lcb_crc8.h`、`lcb_receive_data_buffer.h`、`lcb_receive_data_queue.h`、`lcb_utils.h` | 封包、CRC、送出佇列、回應解析、指令 API、常數、log |
| Layer_5 | `hardware/` | `usb_rs485.h`、`ch340.h`、`usb_serial_bridge.h`、`usb_host.h` | USB 轉 RS485 轉接器：自動偵測與開啟、CH340/CH341 晶片驅動、把 USB 轉成一般 fd 給 `lcb_comm`（見 7.6） |
| — | `third_party/cJSON/` | `cJSON.h` | 解析 `frame_configs.json` 用 |

機種 handler 概念上屬於 Layer_3，但沿用原始碼的位置放在 `protocol/MachineHandler/`。

header 之間只用**檔名**互相引用（例如 `#include "lcb_comm.h"`），所以編譯時每個階層目錄都要加進 include 路徑（見第 4 節）。應用程式自己也一樣只寫檔名：`#include "general_handler.h"`。

## 2. 系統需求

| 項目 | 需求 |
|------|------|
| 目標系統 | aarch64 Linux（RK3566 Buildroot），glibc ≥ 2.34 |
| 相依函式庫 | `libc.so.6`、`libm.so.6`、`ld-linux-aarch64.so.1`（皆為系統內建） |
| 編譯器 | 與板子同一套 Buildroot SDK 的 `aarch64-buildroot-linux-gnu-gcc`（建議），或其他 aarch64 glibc 工具鏈 |
| 語言 | C（`-std=gnu11`）或 C++；所有 header 都有 `extern "C"` |
| 硬體 | LCB 以 UART RS-485 連接（參考板的裝置為 `/dev/ttyS4`），或使用 USB 轉 RS485 轉接器（CH340/CH341，`1a86:7523`、`1a86:5523`，需支援自動收發方向切換） |

## 3. 快速開始

```bash
# 1) 編譯範例（工具鏈需在 PATH 中，或用 CROSS_COMPILE=<前綴> 指定）
cd 3rd_used/example
make

# 2) 複製到板子（同一個目錄即可，範例以 rpath $ORIGIN 連結）
scp lcb_demo ../lib/liblcb_protocol.so root@<board-ip>:/opt/lcb/

# 3) 在板子上執行
/opt/lcb/lcb_demo -d /dev/ttyS4 -m tm      # 走 UART；-m general | tm | bike，-t 秒數，-v 開 debug log
/opt/lcb/lcb_demo -d usb -m tm             # 走 USB RS485 轉接器（自動偵測），需 root 或 udev 規則（見 7.6）
```

LCB 正常回應時的輸出（以跑步機為例，數值依實際機台而定）：

```
[demo] /dev/ttyS4 opened, waiting for the LCB handshake (10 s)...
[gh] LCB type 0x01
[gh] LCB version: type 0x01, firmware 5
[gh] LCB handshake finished, type 0x01
[demo] treadmill_interface attached
[tm] motor RPM 0
[tm] incline ADC 1234
[demo] done
```

範例只讀取狀態，不會送出馬達轉速或升降指令。若出現 `no handshake`，請參考第 12 節。

## 4. 編譯與連結

- **Include 路徑**：7 個目錄都要加（只用 UART 時雖然用不到 `hardware/`，也建議一併加上）：
  ```
  -I<套件>/include/lcb_protocol/capabilities
  -I<套件>/include/lcb_protocol/adapters
  -I<套件>/include/lcb_protocol/control
  -I<套件>/include/lcb_protocol/protocol/Comm
  -I<套件>/include/lcb_protocol/protocol/MachineHandler
  -I<套件>/include/lcb_protocol/hardware
  -I<套件>/include/lcb_protocol/third_party/cJSON
  ```
- **連結**：`-L<套件>/lib -llcb_protocol -pthread -lm`

Makefile（與 `example/Makefile` 相同的寫法）：

```make
LCB_KIT    ?= /path/to/3rd_used
LCB_INC     = $(LCB_KIT)/include/lcb_protocol
LCB_LAYERS  = capabilities adapters control protocol/Comm protocol/MachineHandler \
              hardware third_party/cJSON

CFLAGS  += -std=gnu11 $(addprefix -I$(LCB_INC)/,$(LCB_LAYERS))
LDFLAGS += -L$(LCB_KIT)/lib -Wl,-rpath,'$$ORIGIN'     # rpath 可省略，見第 5 節
LDLIBS  += -llcb_protocol -pthread -lm
```

CMake：

```cmake
set(LCB_KIT /path/to/3rd_used)
set(LCB_INC ${LCB_KIT}/include/lcb_protocol)
add_library(lcb_protocol SHARED IMPORTED)
set_target_properties(lcb_protocol PROPERTIES
    IMPORTED_LOCATION             ${LCB_KIT}/lib/liblcb_protocol.so
    INTERFACE_INCLUDE_DIRECTORIES "${LCB_INC}/capabilities;${LCB_INC}/adapters;${LCB_INC}/control;${LCB_INC}/protocol/Comm;${LCB_INC}/protocol/MachineHandler;${LCB_INC}/hardware;${LCB_INC}/third_party/cJSON")
find_package(Threads REQUIRED)
target_link_libraries(myapp PRIVATE lcb_protocol Threads::Threads m)
```

## 5. 部署到板子

把 `liblcb_protocol.so` 放到動態連結器找得到的位置，三選一：

1. 與執行檔放同一個目錄，連結時加 `-Wl,-rpath,'$ORIGIN'`（範例的做法）。
2. 放進 `/usr/lib`。
3. 放在任意目錄，執行前設定 `LD_LIBRARY_PATH=<目錄>`。

執行程式的使用者必須能讀寫序列埠裝置（例如 `/dev/ttyS4`）；走 USB RS485 時則需要 `/dev/bus/usb/BBB/DDD` 的讀寫權限（見 7.6）。

## 6. 架構

呼叫關係（方括號內為所屬階層與 header 目錄）：

```
 應用程式
   │
   ├─ treadmill_control    [Layer_3 control/]                  （選用）跑步機工程單位控制：km/h、%、開始/暫停/E-Stop
   ├─ treadmill_interface  [Layer_2 adapters/]                 跑步機安全層（safety key、E-Stop、錯誤碼）
   ├─ bike_handler / ...   [Layer_3 protocol/MachineHandler/]  各機種 handler：機種專屬指令與 400 ms 輪詢
   ├─ general_handler      [Layer_3 protocol/MachineHandler/]  LCB 初始化握手、1 秒狀態心跳、錯誤碼處理
   └─ lcb_comm             [Layer_4 protocol/Comm/]            封包、CRC8、優先權送出佇列、回應解析（內含 TX/RX/ERP 三條執行緒）
          │  fd（serial_port_t.fd）
          ├─ UART：應用程式自行開啟的 /dev/ttyS4（7.1）
          └─ USB：usb_rs485  [Layer_5 hardware/]  bridge ⇄ ch340 ⇄ usbdevfs ⇄ USB 轉 RS485 轉接器（7.6）
                 │
                LCB

 frame_config_registry 等  [Layer_1 capabilities/]  機型參數，供 adapters／control 使用（選用，見 10.2）
```

一般流程：

1. 用 `general_handler` 完成 LCB 握手，同時得知 LCB 類型。
2. 握手完成後，把同一條 `lcb_comm` 交給對應的機種 handler，例如跑步機用 `treadmill_interface`、腳踏車用 `bike_handler`。
3. 之後透過機種 handler 下指令、收資料。

## 7. 使用流程

以下程式碼節錄自 `example/lcb_demo.c`，完整可編譯版本請看該檔。

### 7.1 開啟序列埠

走 USB RS485 轉接器時，不需要這一節，請改看 7.6。

函式庫不會開啟或設定 UART，由應用程式負責：

```c
int fd = open("/dev/ttyS4", O_RDWR | O_NOCTTY);

struct termios tio;
tcgetattr(fd, &tio);
cfmakeraw(&tio);
tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
tio.c_cflag |= CS8 | CREAD | CLOCAL;
tio.c_cc[VMIN]  = 0;      /* 重要，見下方說明 */
tio.c_cc[VTIME] = 1;      /* read() 最多等 100 ms */
cfsetispeed(&tio, B9600);
cfsetospeed(&tio, B9600);
tcsetattr(fd, TCSANOW, &tio);

struct serial_rs485 rs485 = { .flags = SER_RS485_ENABLED | SER_RS485_RTS_AFTER_SEND };
ioctl(fd, TIOCSRS485, &rs485);      /* 板子 UART 的 RS-485 收發方向控制 */
tcflush(fd, TCIOFLUSH);
```

| 設定 | 值 |
|------|-----|
| 鮑率 | 9600 |
| 資料格式 | 8N1，raw mode，無硬體／軟體流量控制 |
| `VMIN` / `VTIME` | `0` / `1`（每次 `read()` 最多等 100 ms） |
| RS-485 | `SER_RS485_ENABLED`，送出時 RTS 低、送完 RTS 高（`SER_RS485_RTS_AFTER_SEND`） |

> **務必使用 `VMIN=0`、`VTIME>0`。** 函式庫的 RX 執行緒用 `read()` 收資料，每次 `read()` 返回時才會檢查是否該停止。若設成 `VMIN=1`，LCB 不再送資料時 `read()` 會永遠卡住，`lcb_comm_stop()` 也會跟著卡住不返回。

用 USB 或 RS-232 轉接器等不支援 `TIOCSRS485` 的裝置時，`ioctl` 會失敗。可以忽略這個錯誤，或乾脆不呼叫它（範例的 `-n` 選項）。

### 7.2 建立物件並開始握手

```c
static serial_port_t port;            /* lcb_comm 會保存這個指標，生命週期必須涵蓋整段通訊 */
memset(&port, 0, sizeof(port));
port.fd = fd;                         /* 函式庫只使用 fd；其他欄位僅供紀錄 */
strncpy(port.device_path, "/dev/ttyS4", sizeof(port.device_path) - 1);

lcb_comm_t        *comm = lcb_comm_create();
general_handler_t *gh   = general_handler_create();

general_handler_callbacks_t cb;
general_handler_callbacks_init(&cb);          /* 先清零，不用的 callback 保持 NULL */
cb.on_lcb_version        = on_gh_version;
cb.on_lcb_type           = on_gh_lcb_type;
cb.on_error_code         = on_gh_error_code;
cb.on_lcb_initial_finish = on_gh_initial_finish;
general_handler_set_callbacks(gh, &cb);

/* 會啟動 lcb_comm（不需要另外呼叫 lcb_comm_start）並開始握手 */
if (!general_handler_auto_start(gh, comm, &port)) {
    /* 失敗：general_handler_destroy(gh); lcb_comm_destroy(comm); close(fd); */
}
```

`general_handler_auto_start()` 返回只代表通訊已啟動。握手是非同步進行的，完成時才會呼叫 `on_lcb_initial_finish`。

`on_lcb_version(user_data, a, b)` 的兩個參數分別是 0x73 版本字組的**高位元組 = LCB 類型**、**低位元組 = 韌體版本**，不是「主版本.次版本」。

### 7.3 握手完成後交棒給機種 handler

`on_lcb_initial_finish` 是在 `general_handler` 自己的心跳執行緒上被呼叫的，所以在這個 callback 裡：

- 用 `general_handler_get_lcb_type()` 取得 LCB 類型，必須在 `request_stop` 之前讀取。
- 用 `general_handler_request_stop()` 讓 `general_handler` 讓出 `lcb_comm`。
- **不可以**呼叫 `general_handler_auto_stop()` 或 `general_handler_destroy()`：在自己的執行緒上等待自己結束會造成死結，或在執行緒還在使用時釋放記憶體。
- 建立機種 handler，用**同一個** `comm` 呼叫 `*_start()`。`*_start()` 只會註冊自己的 callback，不會重新啟動序列通訊。

跑步機：

```c
static void on_gh_initial_finish(void *ud)
{
    LcbType_t type = general_handler_get_lcb_type(gh);
    general_handler_request_stop(gh);

    ti = treadmill_interface_create();
    treadmill_interface_set_lcb_type(ti, type);

    treadmill_interface_callbacks_t tcb;
    treadmill_interface_callbacks_init(&tcb);
    tcb.on_rpm                   = on_tm_rpm;
    tcb.on_incline_adc           = on_tm_incline_adc;
    tcb.on_safetykey_lock_state  = on_tm_safetykey;
    tcb.on_safetykey_locked_code = on_tm_estop;      /* 0x02B2 / 0x02B1 */
    tcb.on_error_code            = on_tm_error_code;
    treadmill_interface_set_callbacks(ti, &tcb);

    treadmill_interface_start(ti, comm);
    treadmill_interface_set_auto_reply_rpm(ti, true);
    treadmill_interface_set_auto_reply_incline(ti, true);
}
```

腳踏車：先送出控制類型，**再**讓出 `lcb_comm`。控制類型是 LCB 指令 0x6C，必須趁 `general_handler` 還擁有通訊時送出。

```c
    LcbType_t type = general_handler_get_lcb_type(gh);
    general_handler_set_control_type(gh, GH_CONTROL_TYPE_BIKE);
    general_handler_request_stop(gh);

    bh = bike_handler_create();
    bike_handler_set_lcb_type(bh, type);
    /* bike_handler_callbacks_init / set_callbacks（on_bike_rpm 等） */
    bike_handler_start(bh, comm);
    bike_handler_set_auto_reply_rpm(bh, true);
```

`general_handler` 在 `request_stop()` 之後，它的心跳執行緒會自行結束，但無法被 join，所以**不要再 destroy 這個 gh**（只會留下一小塊記憶體）。只需要讀狀態、不需要機種功能時，也可以不交棒，讓 `general_handler` 持續以 1 秒心跳查詢狀態（範例的 `-m general`）。

### 7.4 下指令與讀取資料

指令會放進送出佇列，由函式庫依優先權依序送出；`*_high_priority` 版本會插隊。所有數值都是 LCB 原始單位。

跑步機（`treadmill_interface.h`）：

| 函式 | LCB 指令 | 說明 |
|------|---------|------|
| `treadmill_interface_set_rpm(ti, rpm)` | 0xF1 | 馬達轉速，bit 15 = 反轉。safety key 拔起時不會送出 |
| `treadmill_interface_set_incline(ti, adc)` | 0xF6 | 升降位置 ADC 0~32767，bit 15 = 反向 |
| `treadmill_interface_set_incline_action(ti, a)` | 0xF5 | 0x00 停止、0x01 上升、0xFF 下降 |
| `treadmill_interface_set_motor_type(ti, watts)` | 0x90 | 馬達功率（例如 1100 / 1500 / 1800 W） |
| `treadmill_interface_get_one_shot_rpm(ti)` | 0xF9 | 查一次轉速，結果由 `on_one_shot_rpm` 回傳 |
| `treadmill_interface_get_one_shot_incline_adc(ti)` | 0xFA | 查一次升降 ADC，結果由 `on_incline_adc` 回傳 |
| `treadmill_interface_set_auto_reply_rpm/incline/...(ti, true)` | — | 開啟定期輪詢，結果持續由 callback 回傳 |
| `treadmill_interface_set_auto_ramp_rpm(ti, en, up, down)` | — | 轉速漸進加減速 |
| `treadmill_interface_is_safetykey_down(ti)` 等 | — | 查詢 safety key／E-Stop 狀態 |

腳踏車（`bike_handler.h`）：

| 函式 | 說明 |
|------|------|
| `bike_handler_set_watt_or_resistance(bh, v)` | 依 LCB 類型自動選擇指令（0x65 PWM/電流、0x67 瓦數、0x62 ECB 位置），所以必須先呼叫 `bike_handler_set_lcb_type()` |
| `bike_handler_get_one_shot_rpm(bh)` | 查一次 RPM，結果由 `on_bike_one_shot_rpm` 回傳 |
| `bike_handler_set_auto_reply_rpm(bh, true)` | 定期輪詢 RPM，結果由 `on_bike_rpm` 回傳 |
| `bike_handler_set_gear_ratio` / `set_resistance_type` / `set_generator_pole` | 機構參數（0x68 / 0x86 / 0x69） |

`general_handler`（握手後仍持有通訊時）：序號讀寫（`general_handler_get_serial_number()` 等）、E-Stop 處理、PFC／馬達類型／MCB 電壓查詢、ERP 待機模式。完整清單請見 `general_handler.h`。

### 7.5 結束與釋放

順序：先停機種 handler，再停 `lcb_comm`，最後由應用程式關閉 fd。

```c
/* 1. 等待 gh 的心跳執行緒結束，確保沒有 gh callback 正在執行。
 *    未交棒時會一併停止 lcb_comm；已交棒時不會動到 lcb_comm。 */
general_handler_auto_stop(gh);

if (handed_off) {
    treadmill_interface_stop(ti);          /* 或 bike_handler_stop(bh) */
    treadmill_interface_destroy(ti);
    lcb_comm_stop(comm);
    /* 交棒後的 gh 不 destroy（見 7.3） */
} else {
    general_handler_destroy(gh);
}

lcb_comm_destroy(comm);
close(fd);                                 /* 函式庫不會關閉 fd；走 USB 時改呼叫 usb_rs485_close()，見 7.6 */
```

### 7.6 使用 USB RS485（CH340）

不走主機板 UART 時，可以把 USB 轉 RS485 轉接器（CH340/CH341）插到板子的 USB 埠。`usb_rs485.h` 會找到轉接器、初始化晶片並回傳一個 fd。把這個 fd 放進 `serial_port_t.fd` 後，7.2 之後的流程（握手、交棒、指令）完全不變。

```c
#include "usb_rs485.h"

usb_rs485_t *usb = usb_rs485_open(NULL, 9600);    /* NULL = 自動偵測；也可指定 "/dev/bus/usb/001/004" */
if (usb == NULL) {
    fprintf(stderr, "USB RS485: %s\n", strerror(errno));    /* ENODEV / EACCES / ENOTSUP / EIO */
    return -1;
}

static serial_port_t port;
memset(&port, 0, sizeof(port));
port.fd = usb_rs485_get_fd(usb);                  /* 由 usb 物件擁有，不要自行 close */
strncpy(port.device_path, usb_rs485_get_node(usb), sizeof(port.device_path) - 1);

general_handler_auto_start(gh, comm, &port);      /* 之後同 7.2～7.4 */

/* 執行中可檢查轉接器是否被拔除 */
if (!usb_rs485_is_healthy(usb)) {
    /* usb_rs485_get_error(usb) 為 -errno，例如 -ENODEV */
}

/* 結束：照 7.5 停掉 handler 與 lcb_comm，最後用 usb_rs485_close() 取代 close(fd) */
lcb_comm_destroy(comm);
usb_rs485_close(usb);
```

| 項目 | 說明 |
|------|------|
| 序列參數 | 鮑率由 `usb_rs485_open()` 的第 2 個參數指定（0 = 9600），固定 8N1，DTR／RTS 開啟 |
| 不需要的設定 | 不用 termios、`VMIN`／`VTIME`、`TIOCSRS485`。fd 是 non-blocking socket，`lcb_comm_stop()` 會正常返回；RS485 收發方向由轉接器自動切換 |
| 結束順序 | 一定要先讓 `lcb_comm` 停止（`general_handler_auto_stop()` 或 `lcb_comm_stop()`），再呼叫 `usb_rs485_close()` |
| kernel 驅動 | 若 kernel 的 `ch341` 驅動已綁定這個轉接器（出現 `/dev/ttyUSBn`），開啟時會自動卸離，使用期間 `/dev/ttyUSBn` 會消失 |
| 多個轉接器 | 自動偵測會使用第一個找到的。要指定時，先用 `usb_rs485_find()` 或 `lsusb` 確認 bus／device 編號，再傳入 `/dev/bus/usb/BBB/DDD` |
| 進階 | `usb_rs485_get_ch340()` 取得晶片物件，可用 `ch340.h` 調整 DTR／RTS、讀取 modem status |

**權限**：程式需要 `/dev/bus/usb/BBB/DDD` 的讀寫權限。請以 root 執行，或加一條 udev 規則，例如 `/etc/udev/rules.d/99-ch340.rules`：

```
SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="5523", MODE="0666"
```

系統若使用 mdev 而不是 udev，請依該系統的方式設定裝置節點權限。

**替代方式**：若板子的 kernel 已內建 `ch341` 驅動，也可以不使用 `usb_rs485`，直接開啟 `/dev/ttyUSB0`，照 7.1 設定 termios（不要呼叫 `TIOCSRS485`），之後當成一般 UART 使用。

範例程式：`lcb_demo -d usb`（自動偵測）或 `lcb_demo -d usb:/dev/bus/usb/001/004`。

## 8. 執行緒與 callback 規則

- `lcb_comm` 啟動後有 3 條執行緒（TX、RX、ERP）。`general_handler` 另有 1 秒心跳執行緒，機種 handler 各有 400 ms 的輪詢執行緒。走 USB RS485 時，`usb_rs485` 另有 2 條收送執行緒。
- **所有 callback 都在函式庫的執行緒上執行**，不在你的主執行緒上：
  - callback 內只做簡短處理，例如更新狀態、放進佇列、通知 UI 執行緒。不要做長時間阻塞的工作。
  - 和主執行緒共用的資料要自行加鎖，或使用 atomic。
- 在 `on_lcb_initial_finish` 裡只能用 `general_handler_request_stop()`，不可以呼叫 `auto_stop()` 或 `destroy()`。
- 同一條 `lcb_comm` 同一時間只能有一個擁有者：`general_handler` 或某一個機種 handler。`*_start()` 會覆蓋前一個擁有者的 callback 註冊。
- 一個序列埠只建立一個 `lcb_comm_t`。

## 9. 機種 handler 一覽

每個 handler 都使用同一套模式：`<name>_create()` → `<name>_callbacks_init()` / `<name>_set_callbacks()` → `<name>_start(h, comm)` → `<name>_stop()` → `<name>_destroy()`。

`treadmill_interface.h` 在 `adapters/`，其餘 `*_handler.h` 都在 `protocol/MachineHandler/`。

| Handler（header） | 機種 |
|------------------|------|
| `treadmill_interface`（`treadmill_interface.h`） | 跑步機，建議使用；內含 `treadmill_handler` 並加上安全邏輯 |
| `treadmill_handler` | 跑步機，不含安全層的底層版本 |
| `hybrid_tm_handler` | Hybrid 跑步機 |
| `bike_handler` | 腳踏車 |
| `indoor_bike_handler` | 室內腳踏車 |
| `tcv_fx_spin_bike_handler` | TCV FX 飛輪車 |
| `elliptical_handler` | 橢圓機 |
| `ascent_handler` | Ascent Trainer |
| `stepper_handler` | 踏步機 |
| `en_climb_handler` | 登山機（EnClimbmill） |
| `jht_ad_board_handler` | JHT A/D 板 |

交棒前是否需要 `general_handler_set_control_type()`，依機種而定：跑步機不需要，腳踏車用 `GH_CONTROL_TYPE_BIKE`。其他機種請向 JHT 確認應使用 `general_handler.h` 中的哪個 `GH_CONTROL_TYPE_*`。

## 10. 進階用法

### 10.1 只用 `lcb_comm`（自行處理協定）

不經過 handler，直接送單一指令、接收回應：

```c
lcb_comm_callbacks_t ccb;
lcb_comm_callbacks_init(&ccb);
ccb.on_lcb_status  = my_status;        /* (void *ud, uint8_t status) */
ccb.on_lcb_version = my_version;       /* (void *ud, uint16_t version) */
lcb_comm_register_callbacks(comm, &ccb);
lcb_comm_start(comm, &port);

lcb_comm_set_lcb_initial(comm);                              /* 0x70 */
lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);    /* 0x71 */
lcb_comm_get_lcb_version(comm);                              /* 0x73 */
```

指令與回應 callback 的完整清單請見 `lcb_comm.h` 與 `lcb_comm_callback.h`。LCB 狀態位元的定義（例如 `LCB_STATUS_BIT_LCB_ERROR`）在 `lcb_constants.h`。

### 10.2 機型參數（frame config）

`frame_config_registry` 可以從 `frame_configs.json` 載入各機型的速度、升降範圍等參數，再套用到機種 handler：

```c
frame_config_registry_init();                          /* 整個程式只呼叫一次，會清空 registry */
frame_config_registry_load_from_file("/etc/jht/frame_configs.json");

machine_config_t mc;
frame_config_data_t fc;
if (frame_config_registry_get_pair_for_lcb_type("0x01", &mc, &fc))
    treadmill_interface_apply_frame_config(ti, &fc);
```

> `treadmill_interface_apply_frame_config()` 會送出 RPM 歸零、升降停止，以及該機型的**初始升降位置**，升降馬達會實際動作。

### 10.3 跑步機工程單位控制（`treadmill_control.h`）

`treadmill_control` 包在 `treadmill_interface` 之上，提供 km/h、坡度 %、開始／暫停／停止／E-Stop、校正等功能。它不會自己建立執行緒，應用程式需約每 200 ms 呼叫一次 `treadmill_control_tick(tc, now_ms)`。用法請見 header 說明。

## 11. 錯誤碼與 E-Stop

- LCB 錯誤碼透過 `on_error_code(uint16_t)` 回傳。`general_handler` 遇到 `0x04B0`（通訊逾時）只會記錄在 log，不會回傳。
- `0x02B2`（`LCB_ERROR_E_STOP`）與 `0x02B1`（`LCB_ERROR_E_STOP_UNLOCK`），定義在 `lcb_proc_comm.h`，代表 E-Stop 觸發與解除，請當成事件處理，不要當成一般故障：
  - `general_handler` 的 `on_error_code` 會收到這兩個碼。
  - `treadmill_interface` 改由 `on_safetykey_locked_code(true/false)` 回報，不會出現在它的 `on_error_code`。
- E-Stop 解除後重新初始化 LCB：交棒前用 `general_handler_estop_revert_lcb_initial()`，交棒後用 `treadmill_interface_estop_revert_lcb_initial()` 或 `bike_handler_estop_revert_lcb_initial()`。

## 12. Log 與疑難排解

函式庫的 log 輸出到 **stderr**，格式為 `[I|D|E] <tag>: <message>`。三個等級預設都開啟，其中 debug 的訊息量很大。產品環境建議：

```c
lcb_log_set_debug(false);      /* 另有 lcb_log_set_info()、lcb_log_set_error()，宣告於 lcb_logging.h */
```

hardware 層（標籤 `UsbRs485`、`Ch340`、`UsbSerialBridge`）的訊息同樣輸出到 stderr，但不受 `lcb_log_set_*()` 控制。這類訊息只在開啟、關閉與發生錯誤時出現。

| 狀況 | 可能原因／處理 |
|------|---------------|
| 編譯時 `xxx.h: No such file or directory` | 缺少某個階層目錄的 `-I`，請把第 4 節列出的 7 個目錄全部加上 |
| 一直等不到 `on_lcb_initial_finish` | 確認裝置路徑、9600 鮑率、RS-485 接線與 LCB 電源；用 `lcb_log_set_debug(true)` 看收送封包 |
| `lcb_comm_stop()` 不返回 | 序列埠設成了 `VMIN=1`，請改用 `VMIN=0`、`VTIME=1`（見 7.1） |
| 執行時 `version 'GLIBC_2.34' not found` | 板子的 glibc 太舊，請向 JHT 索取以對應 SDK 編譯的版本 |
| 執行時找不到 `liblcb_protocol.so` | 見第 5 節 |
| `open /dev/ttyS4: Permission denied` | 執行使用者沒有序列埠的讀寫權限 |
| `usb_rs485_open()` 失敗，errno = `ENODEV` | 沒有偵測到 CH340/CH341：確認轉接器已插上，且 `/sys/bus/usb/devices/*/idVendor` 中有 `1a86` |
| `usb_rs485_open()` 失敗，errno = `EACCES` / `EPERM` | 沒有 `/dev/bus/usb/BBB/DDD` 的權限：以 root 執行或加 udev 規則（7.6） |
| `usb_rs485_open()` 失敗，errno = `ENOTSUP` | 指定的 USB 節點不是 CH340/CH341 |
| `usb_rs485_open()` 失敗，errno = `EIO` | 晶片初始化失敗：重新插拔轉接器，並查看 stderr 中 `Ch340` 的訊息 |
| 執行中 `usb_rs485_is_healthy()` 變成 false | 轉接器被拔除或發生 USB 錯誤：停止通訊並 `usb_rs485_close()` 後，再重新 `usb_rs485_open()` |
| 與自己程式中的 cJSON 符號衝突 | 函式庫內含 cJSON，且會匯出 `cJSON_*` 符號。若應用程式也連結 cJSON，請使用相同版本，或與 JHT 聯絡 |

## 13. Header 索引

常用 header 與所在目錄（完整清單見 1.1）：

| Header | 目錄 | 內容 |
|--------|------|------|
| `lcb_comm.h` | `protocol/Comm/` | `lcb_comm_*`：建立／啟動／停止、所有 LCB 指令、`serial_port_t`、`lcb_queue_mode_t` |
| `lcb_comm_callback.h` | `protocol/Comm/` | `lcb_comm_callbacks_t`：底層回應 callback |
| `lcb_constants.h`、`lcb_commands.h` | `protocol/Comm/` | 指令 ID、狀態位元、常數 |
| `lcb_type.h` | `protocol/Comm/` | LCB 類型代碼（`LCB_TYPE_*`）、`LcbType_toString()` |
| `lcb_proc_comm.h` | `protocol/Comm/` | E-Stop 錯誤碼常數 |
| `lcb_logging.h` | `protocol/Comm/` | `lcb_log_set_info/debug/error()` |
| `lcb_device_data.h`、`lcb_universal_data.h` | `protocol/Comm/` | 0x50 / 0xAA 子指令 |
| `general_handler.h` | `protocol/MachineHandler/` | 握手、狀態心跳、錯誤碼、序號、E-Stop、ERP、`GH_CONTROL_TYPE_*` |
| `treadmill_handler.h`、`bike_handler.h` 及其他 `*_handler.h` | `protocol/MachineHandler/` | 各機種 handler |
| `treadmill_interface.h` | `adapters/` | 跑步機安全層 |
| `treadmill_control.h` | `control/` | 跑步機工程單位控制 |
| `frame_config_registry.h`、`frame_config_data.h` 等 | `capabilities/` | 機型參數 |
| `usb_rs485.h` | `hardware/` | USB RS485 轉接器：`usb_rs485_open()` / `get_fd()` / `close()`、自動偵測（7.6） |
| `ch340.h` | `hardware/` | CH340/CH341 晶片驅動：鮑率、資料格式、DTR／RTS、modem status |
| `usb_serial_bridge.h`、`usb_host.h` | `hardware/` | USB 轉一般 fd 的橋接、usbdevfs 傳輸層（一般不需要直接使用） |
| `cJSON.h` | `third_party/cJSON/` | 內含的 cJSON（frame config 解析用） |

`*_internal.h` 為函式庫內部使用，不在套件中。
