# Little World Arcade v0.1 — 构建与验证报告

日期：2026-10-25（Asia/Shanghai）
结论：**本机编译通过、主机单元测试通过、镜像修订检查通过；未进行板上测试。**

## 1. 交付物

| 文件 | 大小 | SHA-256 |
|---|---|---|
| `build/little_world_arcade.bin` | 955,056 B | `0b3979cb542508407e9e7c414b5e3ecc2ddd70febf35a17b0ec2ead6878d0257` |
| `build/bootloader/bootloader.bin` | 22,016 B | `898718125626f8395c5425307f2fa0f5335befd47698f551feb03280fcf0efac` |
| `build/partition_table/partition-table.bin` | 3,072 B | `3b05e4d7a6ee48240f16eb33d0180ea752a111e9d33bbeb700e338537647e03b` |

源码版本：本仓库提交 `7bfdfc2`（Little World Arcade v0.1）。
依赖：ESP-IDF v5.5.5、BSP 3.0.1、LVGL 9.5.0、esp_lvgl_adapter 0.6.4、`esp32p4_rev3_x.defaults`。

## 2. 镜像修订检查（工具：`tools/image_check.py`）

```
PASS  little_world_arcade.bin
      chip            : esp32p4 (id 0x0012)
      chip revision   : v3.0 (300) .. v3.99 (399)
      flash           : 32MB
      size / sha256   : 955056 bytes / 0b3979cb...878d0257
PASS  bootloader.bin
      chip            : esp32p4 (id 0x0012)
      chip revision   : v3.0 (300) .. v3.99 (399)
      flash           : 32MB
      size / sha256   : 22016 bytes / 89871812...cf0efac
INFO  partition-table.bin
      nvs        offset=0x009000 size=0x006000
      phy_init   offset=0x00F000 size=0x001000
      factory    offset=0x010000 size=0x600000
      storage    offset=0x610000 size=0x200000
```

实板 silicon v3.2 对应 302，落在 `[300, 399]` 区间内；官方预编译 `FactoryOnly.bin` 的上限是 199，这正是它无法启动的原因。

## 3. 主机单元测试（`tools/run_host_tests.ps1`，MSVC x64 /W4 /WX）

```
1393 checks, 0 failure(s)
HOST TESTS PASSED
```

覆盖：
- 2048：四种方向的合并规则、单次合并限制、得分、胜利（1024+1024）、无路可走、初始两格、随机生成不覆盖已有格。
- 贪吃蛇：初始长度/朝向、禁止反向、两步输入队列、吃食物增长与计分、速度下限、撞墙、追尾合法/咬身死亡、食物不刷在身体上。
- 打砖块：初始 60 砖 3 命、发球、左右墙/顶棚反弹、砖块命中判定与得分、挡板反弹带角度、掉球扣命、三条命结束、清屏通关、2000 帧内不出界。
- 存档：编解码往返、CRC-32 已知答案、损坏字节/错误魔数/错误长度/错误版本全部拒绝、最高分与局数统计。

## 4. 未执行的验证（明确列出）

- 实板启动、屏幕显示、触摸坐标、帧率、内存稳定性、NVS 断电保持：**未执行**（需要刷机）。
- 固件写入、整片备份：见 `docs/RECOVERY.md` 的当前状态与 `docs/DEVICE_TEST.md` 的验收清单。
- 音频输出：v1 不含。

## 5. 结论边界

本次“通过”只代表：源码能编译、规则逻辑在主机上正确、产物的芯片/修订/Flash 配置与实板匹配。
它不代表屏幕能亮、触摸能用、游戏在设备上流畅。任何相反的说法都需要实板证据。
