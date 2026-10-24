# Little World Arcade - current status.
# 更新：2026-10-25（Asia/Shanghai）

## 一句话

第一版 Little World Arcade（菜单 + 2048 + 贪吃蛇 + 打砖块）已在本机编译通过、主机测试通过、镜像头检查通过；
**尚未在实板上运行**，出厂固件**尚未备份**，因此还没有刷机。

## 分项状态

| 项目 | 状态 | 证据 |
|---|---|---|
| 独立工程 `D:\LittleWorld-Arcade` | 通过 | 本地 Git，无远程 |
| 游戏规则（arcade_core） | 通过 | `tools/run_host_tests.ps1`：1393 项检查，0 失败 |
| LVGL 9 界面（菜单/HUD/暂停/结算/最高分/音效开关） | 通过（仅编译） | `firmware/components/arcade_ui/` |
| 中文字体（Noto Sans SC 子集，114 字） | 通过 | `tools/gen_fonts.ps1`，OFL 文本随附 |
| 固件编译（esp32p4 / rev3_x / 32 MB） | 通过 | `reports/image-headers.json`，`docs/BUILD.md` |
| 芯片修订范围 | 通过 | app + bootloader 均为 `300..399`，覆盖实板 302 |
| 分区表 | 通过（离线） | nvs 0x9000 / phy 0xF000 / factory 0x10000 6 MB / storage 0x610000 2 MB |
| 实板显示、触摸、帧率、内存 | **未执行** | 需要刷机 |
| 出厂固件整片备份 | **未执行** | 需要用户授权（`docs/RECOVERY.md`） |
| 已验证的回滚路径 | **不具备** | 官方预编译镜像不兼容 v3.2 |

## 本轮已解决 / 已确认

1. **v3.2 不兼容的根因**：官方 `FactoryOnly.bin` 的 bootloader 与 app 镜像头 `max_rev_full=199`，实板 302 超出范围，被 bootloader 拒绝；
   这与“板子坏了”无关，也不是分区或 Flash 问题。
2. **官方没有为 7B 发布 rev3 预编译镜像**：`firmware/` 只有一个 bin，自 2026-08-18 未变更；仓库没有 Releases。
   因此恢复必须来自“本板备份”或“源码重建”。
3. **可重复的 v3.2 构建链已建立**：固定 IDF 5.5.5 + BSP 3.0.1 + LVGL 9.5.0 + `esp32p4_rev3_x.defaults`，
   并新增离线镜像头校验工具 `tools/image_check.py`（不再依赖“编译成功”这种弱证据）。
4. **兜底恢复镜像已构建**：官方 `11_esp_brookesia_phone`（rev3 配置）本机编译通过，
   镜像头 `v3.0–v3.99`，分区偏移与本工程一致（0x2000 / 0x8000 / 0x10000），
   app `2608038f…`、bootloader `765585dd…`。它能把设备恢复成可运行的原厂 Brookesia 桌面，
   但不是逐字节的原厂固件。详见 `docs/RECOVERY.md`。

## 备份进度：**已完成并通过校验**

- 目录：`D:\Espressif\LittleWorld\private\backups\LittleWorld\20261025-005021\`
- `p4-full-A.bin` 与 `p4-full-B.bin` 均为 33,554,432 字节，两次独立读取 SHA-256 完全一致：
  `de8ce1e4b7e212f3588c4ecd25df00e65341c3b25facbfa304178764df2e292f`
- 耗时 22 分钟（460800、esptool stub、4 MiB 分块、每段 ≤2 MiB、自动重试与断点续读）。
- 校验：`tools/check_backup.py` 解析出完整引导链，`manifest.json` 标记 `verified`。

### 实测出厂布局（来自备份，与官方文档不同）

| 分区 | 偏移 | 大小 |
|---|---|---|
| bootloader | 0x2000 | 镜像头 v3.1–v3.99（**是 rev3 固件**） |
| 分区表 | **0x8000** | — |
| nvsfactory | 0x9000 | 0x32000 |
| nvs | 0x3B000 | 0xD2000 |
| otadata | 0x10D000 | 0x2000 |
| phy_init | 0x10F000 | 0x1000 |
| model (spiffs) | 0x110000 | 0xF0000 |
| factory (app) | 0x200000 | 0x800000 |
| storage (spiffs) | 0xA00000 | 0x600000 |

即：**实板出厂固件本身就是 rev3 编译的**，出厂分区表在 0x8000（不是文档里 `firmware/brookesia` 说的 0x9000）。
这与“官方预编译 FactoryOnly.bin 是 v1.99、刷不进去”并不矛盾——那个文件不是这块板子上的固件。

因为本工程的分区表（nvs 0x9000 / factory 0x10000）与出厂布局完全不同，
**任何回滚都必须整片恢复**，不能只重刷 app。

### 备份过程中的问题与解决（已记录）

- 921600 单次整片读取：`Corrupt data, expected 0x1000 received 0xfc0`。
- 460800 单次整片读取：每次传到约 3.5 MiB 就停滞（与波特率无关，是**单次 ROM 会话的传输上限**）。
- 最终方案：4 MiB 分块、每块拆成 ≤2 MiB 的独立 ROM 会话、每段带硬超时（无进度 90 秒即杀）、
  最多 4 次重试、连续失败降到 230400、已完成块断点复用。

## 下一步（按顺序）

1. **等待用户授权**：连接 USB → 只读安全状态检查 → 整片 32 MiB 双读备份 → 校验（详见 `docs/RECOVERY.md`）。
2. 备份通过后，申请**本次**刷入许可；刷入范围：bootloader 0x2000、分区表 0x8000、应用 0x10000。
3. 实板验收（用户配合点触/拍照）：
   - 启动、屏幕亮起、1024×600 方向正确
   - 触摸命中（滑动 2048、滑动/方向键贪吃蛇、拖动打砖块）
   - 三款游戏可玩、返回菜单、暂停/继续、重新开始
   - 连续进出 20 次后，`运行状态`页显示的空闲堆与 tick rate 无持续下降
   - 重启后最高分仍在（NVS）
4. 若备份被安全策略阻止，转为：构建官方 Brookesia 恢复镜像（rev3_x）+ 只做离线开发，不刷机。

## 已知问题 / 未验证项

- 音频：`音效` 开关只持久化偏好，**没有实际声音输出**（v1 明确不含音频，避免未验证的 codec 初始化影响首次启动）。
- 暂停时打砖块/贪吃蛇定时器被暂停，但触摸事件仍会移动挡板（无副作用）；优雅度待实板确认。
- 2048 的合成动画为“瞬时刷新”，未做位移动画（视觉可接受，后续可加）。
- 屏幕方向沿用官方示例的 `ROTATE_180` + `mirror_x/mirror_y`，**必须在实板确认方向与触摸坐标一致**。
- `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y` 但镜像头记录的 mode 字段为 DIO（IDF 在 P4 上的规范化行为），bootloader 会自行处理；未在实板验证 Flash 访问速度。
