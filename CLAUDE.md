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
  - 飞行：空中再按跳进入，WASD 全向渐加速，速度随境界；筑基需飞剑+耗灵力（10/s），金丹+无条件；攻击/冲刺后自动恢复飞行（was_flying）
  - HitBox (layer 5) + HurtBox (monitors layer 6) for combat
  - 本命法宝：温养 120%→150% → 渡劫觉醒 200%，飞升后锁定
- **`Enemy`** (`src/nodes/enemy.h/cpp`) — 6 states: Idle, Patrol, Chase, Attack, Hurt, Death
  - 种类标志：近战/远程（Projectile）/飞行/Boss（多阶段）
- **`Portal`** (`src/nodes/portal.h/cpp`) — Self-contained room transition (composition over inheritance)
- **`CameraRoom2D`** (`src/nodes/camera_room_2d.h/cpp`) — 跟随增益随距离缩放（高速飞行不落后）+ room lock
- **`CultivationSystem`** (`src/cultivation/`) — 13 境界、int64 累计修为经验（9系门槛）、期数、四轴（门派/五仙/出身/果位）、混元一气、TitleComposer 称号、灵力法力池（与修为分离，自动回复）、生命/攻防速随境界、突破调试开关
- **`AbilityManager`** (`src/cultivation/`) — 境界门控能力（纳戒/飞行/云游等）
- **`Inventory` / `ItemDatabase` / `ItemPickup`** (`src/inventory/`, `src/nodes/`) — 24→999 纳戒扩容、装备三槽、掉落拾取
- **`DropSystem`** (`src/core/drop_system.h/cpp`) — 所有掉落物的唯一入口（掉落表+生成）
- **`SaveSystem` / `GameManager`** (`src/core/`) — ConfigFile 存档、检查点、重生、击杀统计
- **`GameHUD` / `TelemetryPanel` / `InventoryPanel`** (`src/nodes/`) — UI 三类分立：游戏 HUD（生命/灵力/修为%条，F4）/ 遥测（F3/F5）/ 背包（I）
- **调试键**: F3 遥测 / F4 HUD / F5 突破无经验门槛开关 / Q 突破 / R 读档

### Input Map

WASD 移动，J 攻击，K 跳跃（空中再按=飞行），L 冲刺，E 交互，F 传送门，I 背包，Q 修炼突破

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

- **战斗**: Player.attack() → HitBox 激活 → 碰撞 HurtBox → Enemy.take_damage()
- **房间切换**: Player 进入 Portal → 加载目标场景 → Camera 约束更新
- **能力门控**: 获得 Ability → AbilityManager 标记 → 之前不可达的区域变为可达
- **修仙**: 击杀/丹药累积修为 → Q 突破（机缘事件待做）→ 境界提升 → 属性/生命/灵力上限提升 → 新能力解锁
- **掉落**: Enemy 死亡 → SignalBus enemy_killed → DropSystem roll 掉落表 → ItemPickup

后续计划与 OOP 抽取候选见 `design/roadmap.md`。
