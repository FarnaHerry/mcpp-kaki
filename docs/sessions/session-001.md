# Session 001 — 基础设施搭建

**日期**: 2026-07-23  
**分支**: HEAD (main)  
**起始提交**: 34dca80 Initial commit

## 完成内容

### 新增系统

#### 1. SignalBus (`src/utils/signal_bus.h/cpp`)
全局信号总线，替代原有的 ad-hoc 直接引用通信模式。17 个信号覆盖四大领域：
- **玩家**: `player_health_changed`, `player_damaged`, `player_died`, `player_respawned`
- **战斗**: `enemy_killed`, `combo_changed`, `combo_ended`
- **修炼**: `spiritual_energy_changed`, `realm_changed`
- **游戏**: `game_paused`, `game_resumed`, `checkpoint_set`, `scene_transition_*`
- **交互**: `interaction_prompt`, `item_picked_up`

#### 2. GameManager (`src/core/game_manager.h/cpp`)
游戏主控节点，作为 autoload 使用：
- 暂停/恢复系统（`get_tree()->set_pause()`）
- 玩家死亡 → 1.5s 延迟 → 检查点复活（满血 + 速度归零）
- 检查点系统（位置 + 场景路径）
- 场景切换请求
- 击杀计数

#### 3. ComboChain (`src/combat/combo_chain.h/cpp`)
三段连招系统：
- **Hit 0**: 快速刺击 (1.0× damage, 0.04s 启动 / 0.08s 活跃 / 0.12s 恢复)
- **Hit 1**: 强力追击 (1.4× damage, 0.06 / 0.10 / 0.16)
- **Hit 2**: 终结重击 (2.0× damage, 0.08 / 0.12 / 0.22)
- 0.5s 衔接窗口，1.2s 超时自动重置
- 通过 `HitBox.hit_landed` 信号追踪命中

#### 4. GameHUD (`src/nodes/game_hud.h/cpp`)
纯 C++ CanvasLayer 实现的游戏内 HUD：
- 左上：血条（红→黄渐变）+ HP 数字
- 血条下：灵气条（蓝色）+ 数值（气 X/Y）
- 境界名显示（金色）
- 右上：连击计数（3+ 时显示，带颜色变化和脉冲缩放）
- 底部中央：交互提示
- 死亡红色遮罩

### 重构内容

| 文件 | 变更说明 |
|------|---------|
| `player.h/cpp` | 多阶段攻击状态（STARTUP→ACTIVE→RECOVERY）；集成 ComboChain；新增 `on_attack_landed` 回调；`attack_held()` 判断；SignalBus 广播健康变更 |
| `enemy.cpp` | 攻击状态翻转到朝向方向；死亡时通过 SignalBus 广播 `enemy_killed` |
| `hitbox.cpp` | 新增 `hit_landed(victim, damage)` 信号用于连击追踪 |
| `cultivation_system.cpp` | `accumulate_energy` 和 `attempt_breakthrough` 成功时广播到 SignalBus |
| `portal.cpp` | `_on_body_entered/exited` 广播 `interaction_prompt` 到 SignalBus |
| `register_types.cpp` | 注册 SignalBus, GameManager, GameHUD |
| `bootstrap.gd` | 启动链: SignalBus → GameManager → HUD → Camera → Player；把 player_died 连接到 GameManager |

### 启动顺序（关键）
```
bootstrap.gd:
  1. SignalBus     ← 必须最先创建
  2. GameManager   ← 依赖 SignalBus
  3. GameHUD       ← 依赖 SignalBus（连接信号）
  4. CameraRoom2D  ← GameManager 持有引用
  5. Player        ← GameManager 持有引用
  6. Portals
  7. Enemies
```

## 架构模式

- **信号解耦**: 各系统通过 `SignalBus::get_singleton()->emit_signal()` 广播事件，`GameHUD` 等监听方通过 `connect()` 订阅
- **GameManager 协调**: 仅持有 Player/Camera 弱引用，不拥有场景生命周期
- **ComboChain 组合**: 作为 Player 的值成员（非 Node），纯数据 + 逻辑

## 已知限制

- GameManager/Potal 房间内的 Enemies 无法找到 Player（跨 scene 引用断开）
- 攻击只有一段动画（无 sprite 切换），但判定帧已通过 Phase 系统控制
- 死亡后 Enemy 不重置
- 无音效/粒子

## 下一步建议

1. 道具/灵石拾取系统 (Inventory + 掉落物)
2. 存档/读档 (SaveSystem)
3. 音效和粒子效果
4. Boss 战系统
5. 更多敌人类型（飞行、远程）
