# 构建说明与已验证产物

更新：2026-10-25（Asia/Shanghai）。以下全部为本机真实执行结果。

## 1. 固定环境（复用正式工程，未重新安装）

| 项目 | 版本 / 路径 |
|---|---|
| ESP-IDF | v5.5.5，`D:\Espressif\LittleWorld\esp-idf-v5.5.5`（提交 `b774170ff46c393eeb5e495ea37936038d3f4f4f`） |
| 工具链根目录 | `D:\Espressif\LittleWorld`（Python 3.12.14、CMake 3.30.2、Ninja 1.12.1、riscv32-esp-elf-gcc 14.2.0） |
| 官方 BSP | `waveshare/esp32_p4_wifi6_touch_lcd_7b` **3.0.1** |
| LVGL | **9.5.0** |
| 中间层 | `espressif/esp_lvgl_adapter` **0.6.4** |
| 官方源码基线 | `waveshareteam/ESP32-P4-WIFI6-Touch-LCD-7B` @ `9fcd49a8c927a00f5583c22d568b3448addbe51c` |
| 芯片修订配置 | `config/esp32p4_rev3_x.defaults`（`CONFIG_ESP32P4_REV_MIN_300=y`） |
| 字体 | Noto Sans SC（`notofonts/noto-cjk`，SIL OFL 1.1），`lv_font_conv` 1.5.2 生成子集 |
| 主机测试编译器 | MSVC 14.51（Visual Studio 18 BuildTools / 2022），x64 |

环境激活：`tools/activate_env.ps1`（只设置当前进程变量，不改系统 PATH，不安装任何东西）。

## 2. 命令

```powershell
# 主机单元测试（arcade_core，不依赖 IDF）
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run_host_tests.ps1

# 生成中文子集字体（114 个非 ASCII 字符，来自扫描源码）
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/gen_fonts.ps1

# 固件编译（产物在 D:\LittleWorld-Arcade\build）
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 -Clean     # 全新 configure
```

`build.ps1` 会把 `SDKCONFIG_DEFAULTS` 设为 `sdkconfig.defaults;sdkconfig.defaults.rev3_x`（在 `activate_env.ps1` 中），
因此**不要**在没有激活环境的情况下直接 `idf.py build`，否则可能生成不带修订限制的镜像。

## 3. 结果

产物大小、SHA-256、镜像头与分区表**由脚本从磁盘上的文件生成**，见
[`reports/BUILD_REPORT.md`](../reports/BUILD_REPORT.md)
（每次构建后运行 `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/make_build_report.ps1` 刷新）。

当前关键结论（2026-10-25）：

- 应用镜像与 bootloader 的芯片修订窗口都是 **v3.0 – v3.99**，覆盖实板 v3.2（302）——这正是官方
  `FactoryOnly.bin`（上限 v1.99）失败的那一项；
- 目标 `esp32p4`，Flash 配置 32 MB；
- 分区表在 `0x8000`；`factory` 位于 `0x10000`、大小 6 MiB，应用约占 0.91 MiB；
- bootloader 22,016 B，从 `0x2000` 起，结束于 `0x7600`，不与分区表重叠。

完整机器可读结果：`reports/image-headers.json`。构建日志：`reports/logs/build-*.log`（本地，不入库）。

## 4. 复现检查清单

1. `firmware/sdkconfig` 中必须同时出现：
   - `CONFIG_IDF_TARGET="esp32p4"`
   - `CONFIG_ESP32P4_REV_MIN_300=y`、`CONFIG_ESP32P4_REV_MIN_FULL=300`、`CONFIG_ESP32P4_REV_MAX_FULL=399`
   - 不得出现 `CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y`
   - `CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y`
2. `python tools/image_check.py --expect-rev 302 <app.bin> <bootloader.bin>` 全部 `PASS`。
3. `tools/run_host_tests.ps1` 退出码 0（1393 项检查）。

## 5. 已知限制

- 这些结论只覆盖**编译与离线镜像检查**，不代表屏幕、触摸、帧率、内存稳定性已在实板通过。
- 构建目录 `D:\LittleWorld-Arcade\build` 与官方示例的构建目录互不影响。
- 首次进入某目录时组件管理器会从 `D:\Espressif\LittleWorld\cache\components` 复用已下载组件；离线也能构建，但需要该缓存存在。
