# Session 018 — 渲染分辨率与窗口解耦（普通游戏语义）+ 多比例支持

**日期**: 2026-08-13
**分支**: main
**前置**: session-017（分辨率改引擎托管 keep+integer）+ 凌霄宝殿飞升结局

## 需求

用户：「分辨率和普通游戏一样——全屏和无边框全屏都可以调节；窗口大小和分辨率解耦；
支持 16:9、21:9、16:10 等常见分辨率」+ 追加「以 16:9 为核心，支持 16:10、21:9、4:3」
+「最后支持我这种特殊的 3:2」（用户屏幕 3120×2080=3:2）。

## 方案

**渲染分辨率（content_scale_size）与窗口大小彻底解耦**——普通游戏语义：

- **渲染比例**（5 档，全窗口模式可调）：16:9 480×270（基准核心）/ 16:10 480×300 /
  3:2 480×320 / 4:3 480×360 / 21:9 630×270。从基准**单边延伸**——更高的比例看多一截纵向世界，
  21:9 看多一截横向世界。aspect=keep：渲染比例与窗口比例不符时引擎居中黑边。
- **缩放模式**（2 档）：整数倍（像素锐利，比例不符黑边）/ 铺满（fractional 填满窗口，
  像素大小略不均）。用户 3:2 屏选 3:2+铺满 = 精确填满零黑边。
- **窗口大小**：原「分辨率」行改名（语义只是窗口尺寸），预设/X 自定义整数倍不变，仅窗口档生效。

## 坑

- **godot-cpp 绑定命名**：project.godot `stretch/scale_mode`(integer/fractional) 对应的
  Window 属性是 `content_scale_stretch`（`CONTENT_SCALE_STRETCH_INTEGER/FRACTIONAL`），
  不是 `content_scale_mode`（那是 disabled/canvas_items/viewport 的 ContentScaleMode）。
- HUD 布局函数里 `_skill_bar_nodes` 是 `CanvasItem*`，无 set_position——需 cast 到 Control。

## 改动

- **game_menu.cpp / nodes.cppm**：设置页 6→8 行（主音量/语言/窗口模式/窗口大小/渲染比例/缩放模式/保存/退出，
  保存/退出顺延 6/7）；`ASPECT_PRESETS[5]` + `SCALE_MODE_NAMES[2]`；`_apply_render_scale()`
  （set_content_scale_size + set_content_scale_stretch）由 `_apply_display` 全模式调用；
  settings.cfg [display] 增 aspect_idx/scale_mode（旧档默认 16:9/整数倍，无迁移成本）。
- **game_hud.cpp / nodes.cppm**：HUD 渲染比例自适应——`_sync_viewport()`（_process 每帧比对
  content_scale_size）→ `_relayout_hud()`：右锚（法则条/威压灵压）/底中（12 技能槽）/左下（消耗品栏）/
  全宽居中（洲名横幅/交互提示/死亡遮罩/查簿 overlay/连击）/Boss 血条居中，全部随 vw/vh 重排。
  创建函数只建节点，位置一律布局函数接管。
- **test_settings_display.gd**：重写 33 断言（新两行循环 + content_scale_size/stretch 属性断言 +
  快照还原 aspect_idx/scale_mode；has_section_key 防缺 key ERROR）。

## 测试

test_settings_display 33 PASS；回归 menu/multi_bossbar/bossbar/lingxiao/hotbar/skill_qwerty 全绿；全量批量回归见尾注。

## 遗留

- 菜单/面板类 overlay（GameMenu 页、背包/商店/仓库面板）仍是 480×270 绝对布局——宽/高比例下
  偏左上，功能正常。后续可做统一锚定（同 HUD 模式）。
- 真机待验证：3:2 屏选 3:2+铺满 的精确填满效果；21:9 下小房间（480 宽）相机 limits 居中表现。
