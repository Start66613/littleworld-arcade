# 恢复与备份方案（当前状态：**未备份、不具备可靠回滚**）

更新：2026-10-25。本文件只描述事实与计划；在用户明确授权前，没有对实体开发板执行任何读取或写入。

## 1. 一句话结论

- 这块 v3.2 板子**目前没有可用的原厂恢复镜像**：官方预编译 `FactoryOnly.bin` 的镜像头最大只支持 **v1.99**，实板是 **v3.2 (302)**，会被 bootloader 直接拒绝。
- 官方仓库至今**没有发布过适用于 7B 的 rev3.x 预编译镜像**（`firmware/` 目录只有一个 bin，且该文件自 2026-08-18 起从未变更；仓库没有 GitHub Releases）。
- 因此，唯一可靠的“随时刷回原系统”手段是：**先做整片 Flash 只读备份**；代码级恢复只能算“功能等价的原厂桌面”，不是原样回滚。
- 在完成并校验整片备份之前，**不应**刷入 Little World Arcade。

## 2. 已核实的兼容性事实

| 项目 | 值 | 来源 |
|---|---|---|
| 实板芯片 | ESP32-P4，silicon **v3.2**（major*100+minor = 302） | 正式工程阶段 02 ROM 只读查询 |
| 实板 Flash | 33,554,432 字节（32 MiB） | 同上 |
| 官方 FactoryOnly.bin | 33,488,896 字节；bootloader(0x2000)/app(0x110000) 头 `min_rev=1`、`max_rev=199` | 离线解析 + 官方 README |
| 该文件 SHA-256 | `3a60bb19b90f04914ac1173d9a63df19eeb3626662c72d7d631028aded00c6df` | 实算，与官方一致 |
| 该文件历史 | 唯一一次提交 `611d5bdb`（2026-08-18），此后未替换 | GitHub API 文件历史 |
| rev3 预编译包 | 只存在于 `waveshareteam/ESP32-P4-Platform`（16 MiB、另一套 BSP、7B 不在支持列表） | 官方仓库 |
| 不兼容原因 | ESP-IDF 在启动时比较 `max_chip_rev_full`；官方文档给出的报错原文即 `Image requires chip rev <= v2.99, but chip is v3.0` | Espressif Chip Revision 文档 |

为什么不能“改版本头”或“强制刷写”：镜像头里的芯片修订范围是给 bootloader 做安全校验用的；改头会得到一个未经验证的组合（rev3 的 PLL/DSI PHY 配置与 pre-v3 不同，官方明确说明两套 profile 的固件不可互换）。这类绕过不在本项目允许范围内。

## 3. 可用的恢复来源（按可靠性排序）

### A. 整片 Flash 备份（唯一字节级回滚）——**尚未执行**
覆盖 P4 外部 Flash `0x00000000–0x01FFFFFF`（32 MiB），包含 bootloader、分区表、应用、NVS、尾部数据。

不包含：ESP32-C6 自身的固件/Flash、P4 eFuse/密钥、PSRAM 内容、SD 卡。

### B. 官方源码重建的原厂桌面（功能等价，不保证逐字节一致）——**已在本机构建并校验**

已实际构建：`examples/esp-idf/11_esp_brookesia_phone`（官方示例，`sdkconfig.defaults` 本身已是 rev3 配置）。
构建目录 `D:\Espressif\LittleWorld\builds\arcade-recovery\brookesia-phone-rev3_x`，源码副本在
`D:\Espressif\LittleWorld\work\brookesia-phone-rev3`（不污染官方 checkout）。

| 产物 | 大小 | SHA-256 | 修订头 |
|---|---|---|---|
| `esp_brookesia_demo.bin` | 3,356,176 B | `2608038fbfcd9141ead7beb3c4c178148a8dd5eecca3c10cf9d63e1ec88330c3` | v3.0–v3.99 |
| `bootloader.bin` | 22,016 B | `765585dda07b5877ee2d941ca55f25bb50c4b18006b9ace9d2e7751ef78ce14d` | v3.0–v3.99 |
| `partition-table.bin` | 3,072 B | 见 `reports/brookesia-recovery-headers.json` | — |

关键性质：**它的分区偏移与本工程一致**（bootloader `0x2000`、分区表 `0x8000`、app `0x10000`，
factory 9 MB），因此刷完 Arcade 之后，用它就能把设备恢复成一个可运行的原厂 Brookesia 桌面，
**不需要先恢复原分区表**。这让“实验固件刷坏了怎么办”有了一个不依赖备份的兜底。

限制：
- 它是官方 Brookesia 手机示例界面，与 7B 出厂预装的桌面**不一定逐像素/逐功能相同**；
  不能声称“这就是原厂固件”。
- 官方 `firmware/brookesia/`（完整版）需要追加 `config/esp32p4_rev3_x.defaults`，且分区表在 `0x9000`，
  与上面的镜像布局不同，不能混刷；本轮没有构建它。
- Wi-Fi、摄像头、音频等完整功能在示例中被裁剪，实板功能需另测。

其它候选：

1. `firmware/brookesia/`（官方仓库内的完整 Brookesia 工程）
   - 官方文档要求：`cd firmware/brookesia && idf.py set-target esp32p4 && idf.py build`
   - **注意**：它的 `sdkconfig.defaults` 里没有任何芯片修订符号，且把分区表放在 `0x9000`。直接编译会得到 pre-v3 或不带修订限制的镜像。
   - 必须追加 `config/esp32p4_rev3_x.defaults`（本项目已有同内容文件 `firmware/sdkconfig.defaults.rev3_x`）。
   - 该工程带有 legacy `esp_wifi_remote 0.14.*`（Wi-Fi 走 C6），是其中最不确定的部分，且官方 CI 不构建它。
2. `examples/esp-idf/11_esp_brookesia_phone/`（官方示例，**已经是 rev3 配置**）
   - `CONFIG_ESP32P4_REV_MIN_300=y`、分区表在 `0x8000`、依赖 BSP 3.0.1 + IDF ≥ 5.5，与本机环境一致。
   - 功能上是原厂桌面的同源示例（Brookesia 手机壳），但不是 7B 出厂固件的逐字节复制。

结论：B 方案能让设备恢复“类似出厂的可玩桌面”，但**不能替代 A**。

### C. 其它（不可用）
- 预编译 rev3 包：属于 `ESP32-P4-Platform`（16 MiB、另一 BSP、分区表 0x8000），与 7B 不是同一产品线，禁止交叉刷写。
- 修改旧 bin 的版本头 / 强制擦写：禁止。

## 4. 备份操作计划（需要用户明确授权后执行）

前置：USB 连接、确认当前串口（此前为 COM5/CH343，**每次都要重新核对**）、板子能进 ROM 下载模式。

### 4.1 只读安全检查（不写入）
```powershell
# 仅为计划，未执行；端口与波特率以实测为准
python -m esptool --chip esp32p4 -p COMx --before default_reset --after hard_reset flash_id
python -m espefuse --chip esp32p4 -p COMx summary        # 只读 eFuse 摘要（判断 Secure Boot / Flash 加密）
```
关注：`SECURE_BOOT_EN`、`SPI_BOOT_CRYPT_CNT`、`DIS_DOWNLOAD_MANUAL_ENCRYPT`、`DIS_DOWNLOAD_*`。
- 若 **Flash 加密已启用**：读回的是密文，只能在**同一颗芯片**上恢复；不能解密、不能换板。
- 若 ROM 下载被禁用：立即停止，不尝试绕过。

### 4.2 整片双读
```powershell
# 仅为计划，未执行
python -m esptool --chip esp32p4 -p COMx --baud 460800 --before default_reset --after hard_reset `
  read_flash --flash_size 32MB 0x0 0x2000000 D:\Espressif\LittleWorld\private\backups\LittleWorld\<时间戳>\p4-full-A.bin
# 同上，输出 p4-full-B.bin
```
- 两次必须是**同一次物理连接下**的独立读取，完成后比较 SHA-256，完全一致才算通过。
- 速度：纯线速下限 460800 ≈ 12 分钟、921600 ≈ 6 分钟；用 ROM 直读（`--no-stub`，每次 64 字节请求）会慢到数小时，因此备份建议允许使用 esptool 自带的 RAM stub（不写入 Flash、不掉电保存）。**这一点需要用户单独同意**，因为正式工程此前的规则是默认 `--no-stub`。
- 保存位置：`D:\Espressif\LittleWorld\private\backups\LittleWorld\<时间戳>\`，本地私有、不入 Git、不上传。空间需求 ≥ 256 MiB（D 盘充足）。

### 4.3 备份校验（离线，不需要板子）
- 文件大小必须正好 33,554,432 字节。
- 两份 SHA-256 相同。
- 离线解析：`0x2000` 处的 bootloader、`0x9000` 处的分区表（出厂布局）magic 是否为 `0xAA50`，并记录应用偏移。
- 记录：读取时间、端口、芯片修订、Flash 容量、eFuse 安全摘要、esptool 版本、两份哈希。
- 只有以上全部通过，状态才能从“未备份”改为“已校验备份”。

### 4.4 恢复演练（需要另外一次写入授权）
恢复 = 按备份原样写回整片（或至少 bootloader + 分区表 + 全部分区），然后断电重启确认桌面回来。
在真正写回验证之前，“可恢复”只能算**有依据**，不能算**已验证**。

## 5. 首次刷入 Little World Arcade 会改动的范围

本工程当前产物（`D:\LittleWorld-Arcade\build`）：

| 区域 | 偏移 | 说明 |
|---|---|---|
| bootloader | `0x2000` | 本工程编译产物，`min_rev=300` |
| 分区表 | `0x8000` | 本工程 `partitions.csv`（factory 6 MB @ 0x10000） |
| 应用 | `0x10000` | `little_world_arcade.bin` |
| NVS | `0x9000` | 本工程布局；**与出厂固件的分区表位置重叠** |

后果：刷入后出厂分区表会被覆盖，**不可能只回滚应用**；任何恢复都必须恢复整片（A 方案）或重刷 B 方案。
这是“必须先备份”的直接原因。

## 6. 需要用户决定的事项

1. 是否授权连接串口、进入 ROM 下载模式，执行**只读**安全检查与整片备份？
2. 备份是否允许使用 esptool stub（大幅缩短时间）？
3. 备份完成并校验后，是否授权刷入第一版 Little World Arcade？

在这些得到明确答复之前，本工程只做本机构建与主机测试。
