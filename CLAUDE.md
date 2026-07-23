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
- **`Player`** (`src/nodes/player.h/cpp`) — 7 states: Idle, Run, Jump, Fall, WallCling, Dash, Attack
  - Variable jump height, coyote time, jump buffering, wall slide/jump, air dash
  - HitBox (layer 5) + HurtBox (monitors layer 6) for combat
- **`Enemy`** (`src/nodes/enemy.h/cpp`) — 6 states: Idle, Patrol, Chase, Attack, Hurt, Death
  - Distance-based detection, chase, attack cooldown, knockback on hurt
  - HitBox (layer 6) + HurtBox (monitors layer 5) for combat
- **`Portal`** (`src/nodes/portal.h/cpp`) — Self-contained room transition (composition over inheritance)
  - Place anywhere, press F to enter/exit. Each Portal owns its scene lifecycle.
  - Entrance → loads .tscn, moves player in, locks camera, creates exit Portal
  - Exit → delegates back to entrance, unloads scene, restores player position
  - Config: `target_scene`, `spawn_marker`, `prompt_text`, `room_bounds`
  - Signals: `portal_prompt(text, show)` for UI hints
- **`CameraRoom2D`** (`src/nodes/camera_room_2d.h/cpp`) — Open world smooth follow + room lock
  - WORLD_FOLLOW: lerp follow with dead zone + look-ahead
  - ROOM_LOCKED: clamped to room bounds
  - `enter_room(bounds)` / `exit_room()` with smooth transitions
- **`HitBox`** (`src/combat/hitbox.h/cpp`) — Area2D attack hitbox; damage, knockback, active-frame control
- **`HurtBox`** (`src/combat/hurtbox.h/cpp`) — Area2D damage receiver; emits hurtbox_hit signal

### Collision Layers

| Layer | Name | Used By |
|---|---|---|
| 1 | Ground | World geometry |
| 2 | One Way Platform | Pass-through platforms |
| 3 | Player | Player body |
| 4 | Enemy | Enemy body |
| 5 | Player HitBox | Player attacks → detected by Enemy HurtBox |
| 6 | Enemy HitBox | Enemy attacks → detected by Player HurtBox |

### Core C++ Classes (Planned)
| `HitBox` | `Area2D` | 攻击判定 |
| `HurtBox` | `Area2D` | 受击判定 |
| `RoomManager` | `Node2D` | 房间加载/卸载/相机约束 |
| `RoomTransition` | `Area2D` | 房间切换触发器 |
| `CameraRoom2D` | `Camera2D` | 房间约束相机 |
| `CultivationSystem` | `Object` | 境界、灵气、突破 |
| `AbilityManager` | `Object` | 可解锁能力管理 |
| `InventorySystem` | `Object` | 背包系统 |
| `SaveSystem` | `Object` | 存档/读档 |
| `SignalBus` | `Node` | 全局信号总线 (autoload) |
| `GameManager` | `Node` | 游戏主控 (autoload) |

### Key System Interactions

- **战斗**: Player.attack() → HitBox 激活 → 碰撞 HurtBox → Enemy.take_damage()
- **房间切换**: Player 进入 RoomTransition → RoomManager.transition_to(target) → 卸载/加载房间 → Camera 约束更新
- **能力门控**: 获得 Ability → AbilityManager 标记 → 之前不可达的区域变为可达
- **修仙**: 灵气累积 → 满足条件 → 境界突破 → 解锁新能力/属性提升 → 新区域可探索

### Input Map

WASD 移动，J 攻击，K 跳跃，L 冲刺，E 交互，Q 修炼
