# JHT_Ccode_LCB_machine_control_lib — Linux 編譯指南

本文件說明如何在 Linux 編譯主機的 Docker container 裡，用 RK3566 Buildroot SDK 編出板子用的 `liblcb_protocol.so`（aarch64）。

## 1. 環境總覽

| 項目 | 內容 |
|------|------|
| 編譯主機 | `192.168.70.36`（Ubuntu 24.04, x86_64, Docker 29） |
| 原始碼位置（主機） | `/home/frankhung/rk3566_dev/JHT_Ccode_LCB_machine_control_lib` |
| 原始碼位置（Windows 網芳） | `\\192.168.70.36\frankhung\rk3566_dev\JHT_Ccode_LCB_machine_control_lib` |
| Container | `rk3566_frankhung`（image `rk3566-env:frankhung`, Ubuntu 18.04, 使用者 `build`） |
| Container 內的原始碼路徑 | `/home/build/rk3566_dev/JHT_Ccode_LCB_machine_control_lib` |
| 工具鏈 | Buildroot SDK `/opt/rk3566-dev/sdk`：`aarch64-buildroot-linux-gnu-gcc` 12.3、glibc 2.37 |
| 產出 | `out/aarch64/lib/liblcb_protocol.so`（ELF 64-bit, ARM aarch64） |

主機的 `~/rk3566_dev` 以 bind mount 掛進 container，所以在 Windows 網芳或主機上改的檔案，container 內會立即看到，不需要複製。container 使用者 `build` 的 uid 是 1002，和主機的 `frankhung` 相同，產出的檔案在主機上的擁有者也是 `frankhung`。

## 2. 快速開始

```bash
# 1) 登入編譯主機
ssh frankhung@192.168.70.36

# 2) 進入 container
docker start -ai rk3566_frankhung
#    （container 已在執行中時改用：docker exec -it rk3566_frankhung bash）

# 3) 編譯
cd /home/build/rk3566_dev/JHT_Ccode_LCB_machine_control_lib
make
```

完成後的 `.so` 在：

```
out/aarch64/lib/liblcb_protocol.so
```

不進 container，也可以在主機上用一行指令編譯（container 必須在執行中，必要時先執行 `docker start rk3566_frankhung`）：

```bash
docker exec -w /home/build/rk3566_dev/JHT_Ccode_LCB_machine_control_lib rk3566_frankhung make
```

## 3. Make 指令

在專案根目錄（`JHT_Ccode_LCB_machine_control_lib/`）執行：

| 指令 | 說明 | 產出 |
|------|------|------|
| `make` | 編譯靜態與動態函式庫 | `out/aarch64/lib/liblcb_protocol.so`、`liblcb_protocol.a` |
| `make test` | 編譯測試程式並用 qemu 執行（`test_lcb`、`test_constants`） | `out/aarch64/bin/` |
| `make example` | 編譯範例 `simple_usage` | `out/aarch64/bin/simple_usage` |
| `make package` | 打包函式庫和對外 header（不含 `*_internal.h`） | `out/aarch64/package/lib/`、`out/aarch64/package/include/lcb_protocol/` |
| `make info` | 顯示 `.so` 的架構、相依函式庫與需要的 glibc 版本 | — |
| `make clean` | 刪除 `out/aarch64/` | — |
| `make help` | 列出指令與變數 | — |

可覆寫的變數：

| 變數 | 預設值 | 用途 |
|------|--------|------|
| `SDK` | `/opt/rk3566-dev/sdk` | Buildroot SDK 位置 |
| `CROSS_COMPILE` | `$(SDK)/bin/aarch64-buildroot-linux-gnu-` | 工具鏈前綴；設成空字串就用本機 `gcc` |
| `SYSROOT` | `$(SDK)/aarch64-buildroot-linux-gnu/sysroot` | 執行測試時 qemu 使用的 sysroot |
| `ARCH_NAME` | `aarch64` | 輸出子目錄名稱（`out/<ARCH_NAME>/`） |
| `QEMU` | 空 | 空：透過主機的 binfmt qemu 執行；也可指定，例如 `QEMU=qemu-aarch64` |

-j 平行編譯可加快速度，例如 `make -j8`。

## 4. 驗證產出

```bash
make info
```

預期輸出：

```
.../out/aarch64/lib/liblcb_protocol.so: ELF 64-bit LSB shared object, ARM aarch64, ...
 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [ld-linux-aarch64.so.1]
Required GLIBC symbol versions:
GLIBC_2.17
GLIBC_2.34
```

板子上的 glibc 必須 ≥ 2.34。用同一套 Buildroot SDK 做出的系統是 glibc 2.37，符合需求。

```bash
make test
```

預期最後看到：

```
║  Tests Passed: 33                                          ║
║  Tests Failed: 0                                           ║
...
✓ All constants tests passed!
```

## 5. 在應用程式中使用

用 `make package` 產出的目錄編譯應用程式（同樣在 container 內）：

```bash
PKG=/home/build/rk3566_dev/JHT_Ccode_LCB_machine_control_lib/out/aarch64/package
aarch64-buildroot-linux-gnu-gcc -std=gnu11 myapp.c -o myapp \
    -I$PKG/include/lcb_protocol -L$PKG/lib -llcb_protocol -pthread -lm
```

部署到板子時，把 `liblcb_protocol.so` 和應用程式一起複製過去，並讓動態連結器找得到它：放進 `/usr/lib`，或執行前設定 `LD_LIBRARY_PATH=<so 所在目錄>`。

函式庫的 log 會輸出到 stderr，格式為 `[I|D|E] <tag>: <message>`。

## 6. 其他編譯方式

- **x86_64 原生編譯**（在 container 或主機上快速驗證程式邏輯）：
  ```bash
  make CROSS_COMPILE= ARCH_NAME=x86_64 test     # 產出在 out/x86_64/
  ```
- **直接使用 `lcb_protocol/` 自己的 Makefile 或 CMake**：Makefile targets、變數、CMake 選項和整合方式，請參考 [lcb_protocol/BUILD_GUIDE.md](lcb_protocol/BUILD_GUIDE.md)。

## 7. 目錄結構

```
JHT_Ccode_LCB_machine_control_lib/
├── Makefile              # 本文件使用的頂層 Makefile（RK3566 交叉編譯）
├── BUILD_LINUX.md        # 本文件
├── lcb_protocol/         # 函式庫原始碼（inc/、src/、test/、examples/、third_party/cJSON）
│   ├── Makefile          # 函式庫本身的 Makefile（頂層 Makefile 會呼叫它）
│   ├── CMakeLists.txt
│   ├── README.md         # 架構與 API 說明
│   └── BUILD_GUIDE.md    # 函式庫建置細節
├── test/                 # 舊的 gtest 測試（*.cpp），目前沒有建置檔，不在此流程中
└── out/                  # 編譯產出（make 後產生，make clean 刪除）
    └── aarch64/
        ├── obj/          # 物件檔
        ├── lib/          # liblcb_protocol.so / .a
        ├── bin/          # 測試程式、範例
        └── package/      # make package 的產出
```

## 8. 常見問題

| 狀況 | 原因／處理 |
|------|-----------|
| `aarch64-buildroot-linux-gnu-gcc: No such file or directory` | 不在 container 內，或 SDK 沒有掛載。用 `docker inspect rk3566_frankhung` 確認 `/opt/rk3566-dev/sdk` 有掛載；SDK 在別的位置時用 `make SDK=<路徑>` |
| `make test` 出現 `qemu: Unsupported syscall: 293` 或 `Illegal instruction` | 用了 container 內的舊版 qemu 2.11（例如設定了 `QEMU=qemu-aarch64-static`）。不要設定 `QEMU`，讓測試透過主機的 qemu 8.2（binfmt）執行 |
| `make test` 出現 `Exec format error` | 主機沒有註冊 aarch64 的 binfmt。在主機上安裝：`sudo apt-get install qemu-user-static binfmt-support`，並確認 `/proc/sys/fs/binfmt_misc/qemu-aarch64` 的 flags 包含 `F` |
| 板子上執行時出現 `version 'GLIBC_2.34' not found` | 板子的 glibc 比 SDK 舊。請用和板子系統相同版本的 Buildroot SDK 重新編譯 |
| 板子上找不到 `liblcb_protocol.so` | 把 `.so` 放進 `/usr/lib`，或設定 `LD_LIBRARY_PATH` |
| 在 Windows 修改後的檔案是 CRLF 換行 | 不影響編譯：GNU make 和 gcc 都能處理 CRLF |
| 想從頭重新編譯 | `make clean && make` |

## 9. 驗證紀錄

2026-10-13 在 `rk3566_frankhung` container 內驗證：

| 項目 | 結果 |
|------|------|
| `make`（aarch64, SDK gcc 12.3） | 54 個原始檔編譯成功，0 warning |
| `make test`（aarch64，透過主機 qemu 8.2 執行） | `test_lcb` 33/33 通過、`test_constants` 全部通過 |
| `make example` 並執行 `simple_usage` | 正常結束 |
| `make package` | 產出 `.so`、`.a` 與 45 個對外 header |
| `make CROSS_COMPILE= ARCH_NAME=x86_64 test`（container gcc 7.5） | 0 warning，測試全部通過 |
| 主機原生 gcc 13（Ubuntu 24.04） | 0 warning，測試全部通過 |
