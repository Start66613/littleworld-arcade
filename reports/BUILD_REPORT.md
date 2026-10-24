# Little World Arcade — 构建与验证报告

> 本文件由 `tools/make_build_report.ps1` 从当前构建产物生成，请勿手改。生成时间：2026-10-25 01:48:15 +08:00

源码版本：`c7817ed` (working tree has 33 uncommitted change(s))

## 1. 产物

| 文件 | 大小 | SHA-256 |
|---|---|---|
| `little_world_arcade.bin` | 956256 B | `2044e51811906535c6044e5268cbffcf49046c2c309d2451399823f8b9847516` |
| `bootloader.bin` | 22016 B | `898718125626f8395c5425307f2fa0f5335befd47698f551feb03280fcf0efac` |
| `partition-table.bin` | 3072 B | `3b05e4d7a6ee48240f16eb33d0180ea752a111e9d33bbeb700e338537647e03b` |

## 2. 镜像头（由 tools/image_check.py 离线解析）

- `little_world_arcade.bin`: chip **esp32p4**, revision **v3.0 .. v3.99**, flash 32MB
- `bootloader.bin`: chip **esp32p4**, revision **v3.0 .. v3.99**, flash 32MB

Partition table:
  - partition `nvs` offset 0x009000 size 0x006000
  - partition `phy_init` offset 0x00F000 size 0x001000
  - partition `factory` offset 0x010000 size 0x600000
  - partition `storage` offset 0x610000 size 0x200000

实板 ESP32-P4 silicon **v3.2 = 302**，落在 `[300, 399]` 内，说明本镜像允许在这块板上启动。
官方预编译 `FactoryOnly.bin` 的上限是 199，这正是它无法启动的原因。

## 3. 本机验证

| 检查 | 命令 | 结果 |
|---|---|---|
| 主机单元测试（游戏规则） | `tools/run_host_tests.ps1` | 1393 项检查通过 |
| 中文字体覆盖 | `tools/check_fonts.py` | 116 个非 ASCII 字符，22/28 px 全部覆盖 |
| 镜像头 / 芯片修订 | `tools/image_check.py` | 见第 2 节 |
| 界面渲染与交互回归 | `tools/run_sim.ps1` | 18 张截图，21 次开合无对象增长，无崩溃 |

一条命令复跑前三项：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/verify.ps1
```

## 4. 未执行的验证

- 实板启动、屏幕显示、触摸坐标、帧率、内存稳定性、NVS 断电保持：**未执行**（尚未刷机）。
- 音频输出：v1 不含（音效开关只持久化偏好）。
- 固件写入本身：等待用户对这一次写入的明确许可。

## 5. 结论边界

“本机编译通过”只代表源码可编译、规则逻辑在主机上正确、镜像头与实板芯片匹配；
不代表屏幕能亮、触摸能用、游戏在设备上流畅。任何相反说法都需要实板证据。
