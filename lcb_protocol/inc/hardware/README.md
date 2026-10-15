# hardware（Layer_5）

實體 I/O 層：讓 LCB 改走 USB 轉 RS485 轉接器（CH340 / CH341，VID:PID `1a86:7523`、`1a86:5523`），不需要主機板 UART。

| Header | 用途 |
|--------|------|
| `usb_rs485.h` | **一般使用這個就好**：自動找到轉接器並開啟，回傳可放進 `serial_port_t.fd` 的 fd |
| `ch340.h` | CH340/CH341 晶片驅動：初始化、鮑率、資料格式、DTR/RTS、modem status、break |
| `usb_serial_bridge.h` | 用 socketpair 把 USB 晶片轉成一般的 stream fd，上層 `lcb_comm` 完全不需要修改 |
| `usb_host.h` | usbdevfs（`/dev/bus/usb/BBB/DDD`）傳輸層與 USB 描述元解析 |

運作方式：

```
lcb_comm  read()/write()  ⇄  socketpair  ⇄  bridge RX/TX 執行緒  ⇄  ch340  ⇄  usbdevfs bulk/control  ⇄  轉接器  ⇄  LCB
```

- 需要 `/dev/bus/usb/BBB/DDD` 的讀寫權限（root 或 udev 規則）。
- 若 kernel 的 `ch341` 驅動已綁定這個轉接器（`/dev/ttyUSBn`），開啟時會自動把它卸離。
- RS485 收發方向由轉接器硬體自動切換。

使用方式見套件 `README.md` 的「使用 USB RS485（CH340）」一節。

UART（例如 `/dev/ttyS4`）仍由應用程式自行開啟與設定，見套件 `README.md` 的「開啟序列埠」一節。
