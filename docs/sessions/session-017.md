# Session 017 — 分辨率方案重构：引擎托管整数缩放（Celeste 式 keep+integer）

**日期**: 2026-08-13
**分支**: worktree（agent-aebda5af1160743a6）

## 背景

session 016 的「动态 gcd 视口」方案（`aspect="expand"` + `scale_mode="integer"` + `_apply_content_scale`
把 content_scale_size 设为 window/gcd 因子 + `_pending_geometry` 每帧纠偏）试图让任意窗口尺寸
「精确吃满无黑边」，代价是：

- 脆弱——历经 7 轮黑边 bug 修补仍有边角（WM 异步、全屏退出内部 size 不跟随等）。
- 视口尺寸随窗口变化 → HUD 绝对坐标布局错位、相机房界 480×270 假设被破坏。

调研结论（业界像素风标准做法）：空洞骑士固定 16:9 构图加黑边、Celeste 320×180 整数倍放大加黑边
——**像素游戏黑边是标准做法，玩家接受**。改回固定 480×270 视口，缩放完全交给引擎。

## 目标方案

project.godot `aspect="keep"` + `scale_mode="integer"`：引擎自动整数倍放大 + 多余区域居中黑边，
视口恒定 480×270，无需任何动态 content_scale_size 代码。

## 改动

- **project.godot**：`window/stretch/aspect` `expand` → `keep`（mode=canvas_items / scale_mode=integer 不动）。
- **src/nodes/game_menu.cpp**：
  - 删除 `_apply_content_scale()` 及 `_igcd()` helper、全部调用点（`_apply_window_geometry` 尾部、
    `_process` 全屏档纠偏分支）。
  - `_apply_display` 全屏档不再置 `_pending_geometry`（缩放引擎托管，无需纠偏）。
  - `_apply_window_geometry` 保留 `Window::set_size`（全屏→窗口内部尺寸同步的关键修复，不回退）；
    `_pending_geometry` 窗口档对齐窗口尺寸的纠偏保留（防 WM 异步）。
  - 分辨率 6 预设档 + X 自定义整数倍（×2~×8）保留——这些尺寸下整数倍零黑边，预设值依然有意义。
  - RES_PRESETS 等注释更新为新方案语义。
- **src/nodes/nodes.cppm**：删 `_last_geom` 成员与 `_apply_content_scale` 声明；`_pending_geometry`
  注释更新；`_geom_target_w/h` 保留（窗口档纠偏仍用）。
- **scripts/test_settings_display.gd**：stretch 断言 EXPAND → KEEP（scale_mode 仍 INTEGER）；
  「无黑边吃满」措辞改「整数倍尺寸零黑边」。
- **data/locale_en.json**：检查后无需改动——分辨率行说明 key 本就是整数倍缩放语义，无「吃满无黑边」措辞。
- **CLAUDE.md**：GameMenu 设置页条目「视口跟随窗口比例 gcd 整除」段改写为引擎托管方案；
  Display 行补 aspect=keep + scale_mode=integer。

## 验证

- `scripts/build_mcpp.sh` 编译通过。
- headless 测试全绿：`test_settings_display.gd`（21 PASS）/ `test_menu.gd` / `test_multi_bossbar.gd`。
- 注意：headless 下 DisplayServer 窗口操作为 no-op，测试只断言配置/属性写入；
  真机像素验证（整数倍、居中黑边、全屏切换）待主线合并后进行。
- worktree 复跑提醒：新 worktree 缺 `.godot/` 缓存时扩展不加载（session 014 已记），
  先 `godot --headless --import` 一次再跑测试。
