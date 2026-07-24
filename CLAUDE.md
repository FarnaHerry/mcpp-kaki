# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

修仙题材的 2D 动作游戏，类银河恶魔城（Metroidvania）风格。Godot 4.6 项目，所有游戏逻辑用 C++ GDExtension 编写，不使用 GDScript。

## Game Design

- **类型**: 修仙 × ACT，银河恶魔城式关卡设计
- **核心特征**: 2D 横版动作战斗，互联地图，能力门控探索
- **世界观**: 修仙体系 —— 境界突破、功法修炼、法宝、丹药、灵气融入玩法
- **分辨率**: 480×270 内部分辨率，canvas_items stretch 到 1920×1080

## Key Project Settings

- **Engine**: Godot 4.6 (config_version=5)
- **Renderer**: gl_compatibility (2D)
- **Display**: 480×270 → 1920×1080, canvas_items stretch
- **Texture filter**: Nearest (pixel art)
- **Project name**: `cpp-kaki`

## Build / Development

```bash
# First time setup
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build build -j $(nproc)

# Output: bin/libcpp-kaki.linux.template_debug.x86_64.so

# Run game (requires display — headless mode does NOT load GDExtensions)
godot
```

CMake variables set before `add_subdirectory(godot-cpp)`:
- `GODOTCPP_TARGET`: `template_debug` (dev) / `template_release` (ship) / `editor`
- `GODOTCPP_API_VERSION`: `"4.6"` — must match the installed Godot version

GDExtension is registered via `cpp_kaki.gdextension` at project root (entry symbol: `cpp_kaki_library_init`) and explicitly referenced in `project.godot` via `[native_extensions] paths=["res://cpp_kaki.gdextension"]`.

**Important**: C++ classes cannot be placed directly in `.tscn` files due to GDExtension registration timing. Use `scripts/bootstrap.gd` (`call_deferred` + `ClassDB.instantiate`) to create C++ nodes at runtime. All game logic remains in C++ — the bootstrap only handles node creation.

## Architecture

### Design Principles

- **组合优于继承**: Godot Node 组合 + Component 模式
- **数据驱动**: 角色属性、敌人、物品用 Godot `.tres` (Resource) 定义，C++ 只读
- **信号解耦**: 系统间通过全局 `SignalBus` autoload 通信

### Directory Structure

```
src/              # C++ 源码 (GDExtension)
  register_types.cpp/h  # 入口，注册所有 class
  core/            # 纯 C++ 系统 (CultivationSystem, AbilityManager, etc.)
  nodes/           # Godot Node 派生类 (Player, Enemy, CameraRoom2D, etc.)
  combat/          # 战斗逻辑 (HitBox, HurtBox, CombatSystem)
  cultivation/     # 修仙数据定义
  inventory/       # 物品/背包数据
  utils/           # 工具类 (StateMachine, InputBuffer, SignalBus)
  resources/       # Godot Resource 绑定
scenes/           # Godot .tscn 场景文件
resources/        # .tres 数据定义 (items, enemies, abilities)
assets/           # 美术/音频/字体
godot-cpp/        # Git submodule — Godot C++ bindings
bin/              # 编译产物 (.so)，gitignored
```

### Currently Implemented

- **`StateMachine<Owner>`** (`src/utils/state_machine.h`) — Generic FSM template, used by Player and Enemy
- **`InputBuffer`** (`src/utils/input_buffer.h`) — Input buffering for responsive platforming
- **`TXT()`** (`src/utils/text.h`) — 统一文本编码包装（所有字符串字面量必须用它，禁直接 `String("...")`，否则中文乱码）
- **`Player`** (`src/nodes/player.h/cpp`) — 8 states: Idle, Run, Jump, Fall, WallCling, Dash, Attack, Fly
  - Variable jump height, coyote time, jump buffering, wall slide/jump, air dash
  - 飞行：空中再按跳进入，方向键全向渐加速，速度随境界；筑基需飞剑+耗灵力（10/s），金丹+无条件；攻击/冲刺后自动恢复飞行（was_flying）
  - HitBox (layer 5) + HurtBox (monitors layer 6) for combat
  - 本命法宝：温养 120%→150% → 渡劫觉醒 200%，飞升后锁定
- **`Enemy`** (`src/nodes/enemy.h/cpp`) — 6 states: Idle, Patrol, Chase, Attack, Hurt, Death
  - 种类标志：近战/远程（Projectile）/飞行/Boss（多阶段）
- **`Portal`** (`src/nodes/portal.h/cpp`) — Self-contained room transition (composition over inheritance)
- **`CameraRoom2D`** (`src/nodes/camera_room_2d.h/cpp`) — 跟随增益随距离缩放（高速飞行不落后）+ room lock
- **`CultivationSystem`** (`src/cultivation/`) — 13 境界、int64 累计修为经验（9系门槛，**到顶封顶卡境界**）、期数、四轴（门派/五仙/出身/果位）、混元一气、TitleComposer 称号、灵力法力池（与修为分离，自动回复）、生命/攻防速随境界、突破调试开关
- **`AbilityManager`** (`src/cultivation/`) — 境界门控能力（纳戒/飞行/云游等）
- **`BreakthroughManager`** (`src/cultivation/`) — 机缘突破唯一入口：Q → SignalBus `breakthrough_requested` → 叙事事件×6 / 心魔劫·三尸劫（战斗秘境，属性随玩家缩放，Enemy `no_drops`）/ 三灾连考；秘境复用 Portal 模式（场景加载+玩家重挂载+相机锁定）；失败境界不变、经验保持封顶可重试
- **`TribulationController`** (`src/cultivation/`) — 三灾：雷灾（预警落雷走位）→ 阴火（DoT 生存）→ 赑风（控制反转 `input_inverted`+罡风推移+风蚀）；渡劫过渡态 DU_JIE（失败退回大乘）
- **`Inventory` / `ItemDatabase` / `ItemPickup`** (`src/inventory/`, `src/nodes/`) — 24→999 纳戒扩容、装备三槽、掉落拾取
- **`DropSystem`** (`src/core/drop_system.h/cpp`) — 所有掉落物的唯一入口（掉落表+生成）
- **`SaveSystem` / `GameManager`** (`src/core/`) — ConfigFile 存档、检查点、重生、击杀统计
- **`GameHUD` / `TelemetryPanel` / `InventoryPanel`** (`src/nodes/`) — UI 三类分立：游戏 HUD（生命/灵力/修为%条，F4）/ 遥测（F3/F5）/ 背包（I）
- **DamageNumbers** (`src/nodes/`) — 伤害数字唯一入口：SignalBus `damage_dealt(pos,amount,is_player_victim)` → 世界坐标上浮淡出（敌=金/玩家=红）
- **DamageCalculator** (`src/combat/damage_calculator.h`, header-only) — 伤害统一结算：物理(防御减免,min1)/法术(抗性比例,cap0.9)/元素(五行抗性,克制×1.25只增伤)；`DamageInfo`+`DefenseProfile`；HitBox/Projectile 携带 `damage_category`+`element`，投射物经 `take_damage_typed` 入口
- **GongfaSystem** (`src/cultivation/gongfa_system.*`) — 功法：炼体/练气双槽(1+1)，黄/玄/地品(3/5/7层)，行为喂养主系100%/副系20%(受击/近战击杀养炼体，耗灵养练气)，层数乘区(1+每层×层数)，切换保留熟练(_known)，存档 pd["gongfa"]
- **SkillSystem** (`src/combat/skill_system.*`) — 武技/法术/神通/仙法统一 Skill 管线：8槽(A/S武技 D/F法术 G/H战斗页闲置 T神通 Y仙法)，武技=物理+冷却驱动(凡人起步破空斩/突进斩)，法术=元素伤害+耗灵+冷却(炼气授予火弹/冰锥)，神通=耗法则之力(化神授予缩地成寸,FX_BLINK碰撞安全瞬移)；存档 pd["skills"]
- **ArtifactSystem** (`src/cultivation/artifact_system.*`) — 法宝：槽0=本命(镜像Player本命,飞升后锁定)+次要×2(飞升后+3)，威力系数本命1.2~2.0/次要1.0→1.2→1.5(两段温养)，攻击型祭出复用Skill效果管线(耗灵+冷却)，辅助型常驻被动入防御乘区；示例：飞剑(筑基)/照妖葫(金丹)/玄铁塔(元婴)；存档 pd["artifacts"]
- **B键切页**: `Player._skill_page` + `skill_page_changed` 信号；法宝页 A~H=法宝槽0..5(T/Y两页通用)；页机制通用可扩技能多页
- **法则之力** (CultivationSystem) — 化神解锁独立能量条(max100,回复3/s+击杀+10)，神通唯一消耗源；HUD右上角紫条；存档 cd["law_power"]
- **GameMenu** (`src/nodes/game_menu.*`) — ESC 多页管理菜单（背包/能力/功法/技能/法宝/设置），托管 InventoryPanel（外部驱动 ext_navigate/ext_use）；能力页=主动/被动分区技能树总览（只读 v1）；设置页含音量(持久化 user://settings.cfg)/保存/退出；嵌套暂停安全（还原原暂停状态）；页签条独立 CanvasLayer 130
- **HUD 底部技能栏**: 武技[A/S] 法术[D/F] 法宝[G/H] 空槽占位（技能系统落地后填充）
- **调试键**: F3 遥测 / F4 HUD / F5 突破无经验门槛开关 / Q 突破 / R 读档

### Input Map

方向键移动（WASD 已腾出给技能槽，DNF 式），J 攻击，K 跳跃（空中再按=飞行），L 冲刺，E 使用/装备，Space 交互/确认，I 背包，Q 修炼突破，ESC 多页菜单；技能槽：A/S 武技、D/F 法术、T 神通、Y 仙法（预留），B 切法宝页（A~H=法宝槽）

### Collision Layers

| Layer | Name | Used By |
|---|---|---|
| 1 | Ground | World geometry |
| 2 | One Way Platform | Pass-through platforms |
| 3 | Player | Player body |
| 4 | Enemy | Enemy body |
| 5 | Player HitBox | Player attacks → detected by Enemy HurtBox |
| 6 | Enemy HitBox | Enemy attacks → detected by Player HurtBox |

### Key System Interactions

- **战斗**: Player.attack() → HitBox 激活（monitoring 关→开重扫重叠，每次激活都重新结算）→ **HitBox 侧驱动伤害**：检测到 HurtBox 即在 hurtbox 上 emit `hurtbox_hit` → owner take_damage（单边结算，HitBox 另发 hit_landed 给连击）。**不可反过来由 HurtBox 监控 HitBox**——持续重叠不发 exit/enter，只有第一下命中
- **房间切换**: Player 进入 Portal → 加载目标场景 → Camera 约束更新
- **能力门控**: 获得 Ability → AbilityManager 标记 → 之前不可达的区域变为可达
- **修仙**: 击杀/丹药累积修为 → Q 突破（机缘事件待做）→ 境界提升 → 属性/生命/灵力上限提升 → 新能力解锁
- **掉落**: Enemy 死亡 → SignalBus enemy_killed → DropSystem roll 掉落表 → ItemPickup

后续计划与 OOP 抽取候选见 `design/roadmap.md`。
