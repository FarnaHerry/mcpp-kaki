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

## 追加：凌霄宝殿主线叙事 + 飞升结局（主线 agent 亲做，与分辨率重构并行）

roadmap 最后一项大内容。设计依据 design/cultivation-realms.md（混元一气=金仙大圆满+特殊事件解锁，非经验堆出）+ world-map.md（凌霄殿为飞升终点）。

- **持久化 flags（GameManager）**：`set_flag/get_flag/has_flag`，`_flags` Dictionary 存档 `data["flags"]` 段（旅行桥 collect/apply 自动携带）；`boss_fight_ended(name)` 自动落 `boss_dead:<名字>` flag——Boss 守关门控从此有通用持久化依据（此前没有任何 Boss 击杀持久化）。
- **NarrativeNode**（通用叙事交互节点，C++）：Area2D + X 轮询模板；overlay 暂停世界逐行推进；precheck_method（条件不足单行拒绝）/gm_method（首轮走完回调 GameManager）/once_flag/after_lines（完成后改播后日谈）。PROCESS_MODE_ALWAYS + 树暂停守卫（菜单暂停期不响应 X）。
- **CondPortal**（条件房间门 GDScript）：flag 满足 ↑ 手动 trigger 内部 Portal（monitoring 关闭不自触发），否则拒绝提示 2.5s。
- **凌霄宝殿**：rooms/lingxiao_dian.tscn + scripts/rooms/lingxiao_dian.gd（金柱/玉阶/御座视觉）；入口在天界巨灵神身后（x=3480）。太白金星引见（once）→ 玉帝混元仪式：precheck 金仙大圆满+巨灵神已伏 → 7 行仪式 → attain_hunyuan + ending_seen + 全恢复 = **飞升结局**；结局后可自由云游（after_lines 后日谈）。
- **测试**：test_lingxiao.gd 34 步端到端全绿（真杀巨灵神走完整死亡链验证 flag）；回归 test_tianjie/bossbar/multi_bossbar/difu/settings_display/menu 全绿。
- 顺带：`collect_save_data` 补 bind_method（测试/脚本可调）。

### 并行说明
分辨率重构在 worktree 由后台 agent 完成（见上文），主线同做飞升结局，两边文件零重叠
（agent：game_menu.cpp/nodes.cppm/project.godot/test_settings_display；主线：game_manager.*/narrative_node.*/gates/rooms/tianjie.gd），合并无冲突。
