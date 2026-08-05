# Session 009 — 西牛贺洲 v1（火焰山/方寸山/流沙河）

**日期**: 2026-08-06
**分支**: main
**起始提交**: 6bcf9a7（南赡部洲/商店/地府进阶）
**结束提交**: （本会话提交）

## 完成内容

### 环境火伤（新机制，`scripts/zones/fire_zone.gd`）

- Area2D + `_physics_process` 用 `get_overlapping_bodies()` 检测玩家，dot 累积（每 0.5s 扣 max 血 ×6%，三灾阴火模式）
- `extinguish()`：停伤害 + 隐藏视觉 + 关 monitoring（被芭蕉扇熄灭）；加入 `fire_zones` 组
- 火焰山岩浆池挡路（需绕行跳跃；芭蕉扇可灭开道）

### 弱水禁飞（新机制，`scripts/zones/no_fly_zone.gd` + Player 钩子）

- NoFlyZone Area2D：body_entered 置 `player.set("flight_blocked", true)` / body_exited 清
- **Player** 加 `_flight_blocked`：`can_fly()` 开头检查；FlyState `physics_update` 加禁飞即坠（流沙河弱水区飞行失效，须跳石墩过河）

### 斜月三星洞秘境（Portal-room，水帘洞模板）

- `scenes/rooms/xieyue_sanxing_dong.tscn` + `scripts/rooms/xieyue_sanxing_dong.gd`：暗洞 + 洞顶三星 + 守洞妖×2+精英 + 秘藏
- 秘藏：**菩提心法残卷**（新物品 `pu_ti_xin_fa_juan`，use_consumable learn）+ 千年灵芝 + 灵石

### 芭蕉扇（法宝 + 新物品管线）

- **Item 新字段 `learn_artifact`**（JSON + fallback + use_consumable 处理）：使用物品获得法宝
- 芭蕉扇物品（铁扇公主遗物，火焰山拾取）+ `ARTIFACT_DEFS` 加 `ba_jiao_shan`（FX_PROJ_FAN 风刃，25 威力，cd 4s）+ **activate_slot 加 FX_PROJ_FAN 分支 + 祭出时扇灭 `fire_zones` 组**（设计钩子「芭蕉扇开路」）

### 菩提心法（新被动技能）

- SkillSystem `PAS_ELEM_RESIST` 新枚举 + `get_passive_elem_resist()` + SKILL_DEFS/`skills.json` 加 `pu_ti_xin_fa`（全元素抗性 +10%）
- Player 元素抗性计算（`_take_damage_typed` DefenseProfile）加被动乘区

### 地图（scripts/continents/xiniuhe.gd 重写，~3500px）

- 火焰山 0~1500（赤岩台地 + FireZone×3 + 火鸦/火牛 + 赤焰花 + 芭蕉扇）+ 方寸山 1500~2500（高台 + 三星洞 Portal + 守山妖）+ 流沙河 2550~3500（弱水缺口 + 石墩 + NoFlyZone + 沙怪）+ 检查点 ×3

### 测试

`test_xiniuhe.gd`（15 步）：金丹解锁+travel → 环境火伤 → 芭蕉扇得法宝+灭火 → 弱水禁飞+恢复 → 三星洞进洞 → 菩提心法 learn+抗性 → 出洞。**全量回归通过**（test_double_jump 偶发时序抖动，复跑通过）。

### 关键坑

- skills.json 缺 `proj_color` 字段 → `ensure_defs_loaded` 读 None 崩溃（`Array c = d["proj_color"]` 后 `c[0]`）——JSON 必须补齐所有解析字段
- `can_fly` 未绑定 → GDScript 调用报错（测试补绑定）
- `get_tree()` 在 SceneTree 脚本中不存在（自身即树）——用 `get_nodes_in_group` 直接调
- get_nodes_in_group 返回 TypedArray，C++ 遍历需 `Object::cast_to<Node>(zones[i])`
