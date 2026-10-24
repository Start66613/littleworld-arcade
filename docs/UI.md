# 界面与模拟器验证

下面的截图**不是效果图**：它们由 `host_sim/` 把真实的 `arcade_ui` + `arcade_core` + 生成字体
编译进桌面版 LVGL 9.5，创建 1024×600 的显存显示，用脚本模拟触摸（点击、滑动、拖拽）后直接读回显存生成的。
同一份代码编译进固件，所以布局、字号、颜色、交互逻辑与真机一致（渲染后端不同，见“差异”一节）。

重新生成：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run_sim.ps1
```

## 1. 主页

![主页](screenshots/01-menu.png)

- 三张卡片：2048 / 贪吃蛇 / 打砖块，各自带最高分
- 右上角：运行状态（齿轮）与音效开关
- 中文全部来自 Noto Sans SC 子集字体，无缺字方框

## 2. 2048

![2048](screenshots/02-2048.png)

![2048 滑动后](screenshots/03-2048-after-swipes.png)

- 顶部：得分 / 最高 / 目标，右侧暂停与音效
- 滑动两次后方块上移、生成新块（模拟器确实执行了手势，不是静态截图）

## 3. 运行状态（诊断浮层）

![诊断](screenshots/05-diagnostics.png)

实机会显示真实堆内存与真实 LVGL tick 速率；模拟器里堆数字是桩值。
tick 速率这一项用来在实板上判断“UI 线程有没有被游戏循环堵住”。

## 4. 贪吃蛇

![贪吃蛇](screenshots/06-snake.png)

![贪吃蛇暂停](screenshots/08-snake-paused.png)

- 顶部方向键：左上角的返回箭头与本机的“左方向键”是不同控件，模拟器按坐标点击
- 暂停浮层三个按钮：继续游戏 / 重新开始 / 返回主页

## 5. 打砖块

![打砖块](screenshots/12-breakout.png)

![打砖块进行中](screenshots/13-breakout-playing.png)

- 六行彩色砖块、挡板、小球、生命
- 上图是发球后击中 1 块砖（得分 10、生命 2）的瞬间

## 6. 生命周期检查

![开合 20 次后](screenshots/17-after-20-open-close-cycles.png)

`tools/run_sim.ps1` 会连续“进入 2048 → 返回主页”21 次并统计活动对象数：

```
live objects: after 1 cycle = 67, after 21 cycles = 67
PASS: no object growth across 21 open/close cycles
```

这是“反复切换页面不泄漏”的自动化证据；实板上还会用诊断页的空闲堆再确认一次。

## 模拟器覆盖了什么

- 三个游戏的创建/退出/暂停/继续/重新开始/返回主页
- 2048 的滑动、打砖块的拖动、贪吃蛇的方向键
- 诊断浮层开关
- 21 次页面开合的对象数不增长
- 整个过程没有崩溃（否则进程非 0 退出）

## 模拟器不覆盖什么

- 真实 LCD 时序、撕裂、帧率、背光、触摸坐标与旋转（必须实板验证）
- 真实堆内存、PSRAM、NVS 掉电保持
- 音效（v1 无音频输出）

## 差异说明

模拟器用 16 位色、全屏单缓冲渲染；实机用 BSP 的三缓冲 + 撕裂避免模式，颜色格式与绘制结果一致。
模拟器的 ESP-IDF API（NVS/RNG/堆）是桩实现，所以诊断页数字与实机不同，属预期。
