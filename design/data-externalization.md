
## 实施状态 (2026-07-26)

| 优先级 | 系统 | JSON文件 | 运行时 | 状态 |
|---|---|---|---|---|
| P0 | 物品 | data/items.json (22) | ItemDatabase::_ready → DataLoader | ✅ |
| P0 | 技能 | data/skills.json (24) | SkillSystem::ensure_defs_loaded → DataLoader | ✅ |
| P1 | Buff | data/buffs.json (6) | BuffSystem::ensure_defs_loaded → DataLoader | ✅ |
| P1 | 功法 | data/gongfas.json (6) | GongfaSystem::ensure_defs_loaded → DataLoader | ✅ |
| P1 | 宗门 | data/sects.json (4) | SectSystem::ensure_defs_loaded → DataLoader | ✅ |
| P2 | 掉落 | data/drops.json | DropSystem::_roll_drops → DataLoader | ✅ |
| P2 | 配方 | data/recipes.json (7) | DataLoader 已加载, JSON 就绪 | ⏸️ |
| P2 | 洲 | data/continents.json (4) | DataLoader 已加载, JSON 就绪 | ⏸️ |
| P2 | 境界 | — | 待做 | ❌ |
| P3 | 事件 | — | 待做 | ❌ |
| P3 | 能力 | — | 待做 | ❌ |

> 核心模式：每个系统增加 `static std::vector<Def> s_defs` + `static bool s_loaded` +
> `static void ensure_loaded()` 惰性填充。DataLoader 可用时走 JSON，否则退回硬编码静态数组。
> `const char*` 字段通过 `static std::vector<std::string>` 持久化存储。


# 硬编码数据外抽清单

> 2026-07-25 摸底。系统稳定后，所有静态定义表应迁移为外部数据（Godot `.tres` Resource 或 JSON/CSV），
> C++ 侧只保留运行时结构体与查询逻辑。本文件按系统逐一列出当前硬编码的每张表、字段、记录数。

---

## 一、修仙体系（CultivationSystem）

### 1.1 境界定义表（REALM_STATS）— 13 条
**位置**：`src/cultivation/cultivation_system.cpp:33`

| 字段 | 类型 | 说明 |
|---|---|---|
| name | string | 境界名（凡人/炼气/筑基/…/天尊） |
| dmg | float | 攻击乘数 |
| def | float | 防御乘数 |
| spd | float | 速度乘数 |

### 1.2 境界经验门槛（REALM_CAPS）— 13 条
**位置**：`src/cultivation/cultivation_system.cpp:15`

| 字段 | 类型 | 说明 |
|---|---|---|
| realm_idx | int (implicit) | 境界序号 0-12 |
| cap | int64 | 累计经验上限（0=无经验条，如渡劫/天尊） |

### 1.3 期数加成（STAGE_FACTOR）— 4 条
**位置**：`src/cultivation/cultivation_system.cpp:47`

| 字段 | 类型 | 说明 |
|---|---|---|
| early/mid/late/dayuanman | float | 前期 1.0 / 中期 1.05 / 后期 1.10 / 大圆满 1.20 |

### 1.4 混元一气加成 — 3 值
**位置**：`src/cultivation/cultivation_system.cpp:43`

| 字段 | 值 |
|---|---|
| HUNYUAN_DMG | 70.0f |
| HUNYUAN_DEF | 65.0f |
| HUNYUAN_SPD | 4.5f |

### 1.5 灵力基底 — 按境界 switch（13 条）
**位置**：`src/cultivation/cultivation_system.cpp:281` `get_max_mana()`

| 字段 | 类型 | 说明 |
|---|---|---|
| realm | enum | 境界 |
| base_mana | float | 凡人 0 / 每境 +50 / 真仙 1000 / 金仙 2000 / 天尊 9999 |

### 1.6 灵力回复率 — 硬编码常量
**位置**：`src/cultivation/cultivation_system.cpp:370`

当前：`max_mana * 0.02 / s`（2%/s），一条固定值。
建议外抽为每境界可配回复率。

### 1.7 法则之力参数 — 2 值
**位置**：`src/cultivation/cultivation_system.h:39`

```cpp
static constexpr double LAW_POWER_MAX = 100.0;
static constexpr double LAW_REGEN_PER_SEC = 3.0;
```

### 1.8 资源名切换 — 按境界
**位置**：`src/cultivation/cultivation_system.h:112`

`get_mana_name()`: 凡人→"灵力" / 真仙+→"仙元"（硬编码 is_immortal() 判定）

---

## 二、技能系统（SkillSystem）

### 2.1 技能定义表（SKILL_DEFS）— 21 条
**位置**：`src/combat/skill_system.cpp:11`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 技能 ID |
| name | string | 技能名（中文） |
| type | enum | MARTIAL / SPELL / SHENTONG / XIANFA / PASSIVE |
| category | enum | DMG_PHYSICAL / DMG_ELEMENTAL |
| element | enum | ELEM_NONE / ELEM_JIN/MU/SHUI/HUO/TU/LEI |
| mana_cost | float | 灵力消耗（武技=0） |
| law_cost | float | 法则之力消耗（神通专用） |
| cooldown | float | 冷却秒数 |
| power | float | 伤害倍率（×攻击面板） |
| effect | enum | FX_MELEE_SWING/LUNGE/PROJECTILE/BLINK/AOE_SWING/RISING/SELF_BUFF/PROJ_FAN/INVULN |
| min_realm | int | 解锁最低境界 |
| proj_speed | float | 投射物速度 |
| proj_color | Color | 投射物颜色 |
| effect_param | float | 效果参数（瞬移距离/无敌秒数） |
| buff_id | string | 自buff 引用 BuffSystem def |
| passive_stat | enum | 被动属性（PAS_ATK/SPD/DEF/MANA_REGEN/FLY_SPEED/LAW_REGEN/NONE） |
| passive_value | float | 被动数值（如 0.10=+10%） |

明细：

| # | 类别 | 技能 |
|---|---|---|
| 4 | 武技 | 破空斩 / 突进斩 / 旋风斩 / 升龙击 |
| 5 | 法术 | 火弹术 / 冰锥术 / 雷咒术 / 土盾术 / 御剑术 |
| 4 | 神通 | 缩地成寸 / 金刚不坏 / 三昧真火 / 身外化身 |
| 1 | 仙法 | 天雷引 |
| 4 | 宗门专属 | 万剑归宗 / 太清神光 / 玄龟护体 / 血影斩 |
| 6 | 被动 | 神行百变 / 剑心通明 / 铁布衫 / 灵台清明 / 风雷双翼 / 道法自然 |
| **21** | **合计** | |

### 2.2 境界授予映射 — 硬编码在 bootstrap + skill grant 逻辑
**位置**：`src/nodes/player.cpp` `_on_cultivation_realm_changed` + `on_ability_unlocked`

技能授予散布在多个位置（玩家 realm changed、物品 use_consumable 的 learn_skill 字段）。
建议统一为「境界授予表」（realm → [skill_id, …]）。

---

## 三、物品数据库（ItemDatabase）

### 3.1 物品注册表（_register_items）— 20+ 条
**位置**：`src/inventory/item_database.cpp:56`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 物品 ID |
| name | string | 中文名 |
| description | string | 描述文本 |
| type | enum | CONSUMABLE / EQUIPMENT / MATERIAL / KEY_ITEM |
| max_stack | int | 最大堆叠数 |
| grade | int | 品级 0凡/1灵/2地 |
| heal_amount | float | 固定回血量 |
| heal_pct | float | 比例回血 |
| mana_amount | float | 回灵量 |
| energy_amount | float | 返回修为 |
| buff_id | string | 使用后加 buff（引用 BuffSystem） |
| learn_skill | string | 使用后习得技能 |
| breakthrough_bonus | float | 突破成功率加成 |
| equip_slot | enum | 装备槽位 WEAPON/ARMOR/ACCESSORY |
| attack_bonus | float | 装备攻击加成 |
| defense_bonus | float | 装备防御加成 |

明细：消耗品 10 + 装备 3 + 材料 7 + 关键物品 1 ≈ 21 条

---

## 四、宗门系统（SectSystem）

### 4.1 宗门定义表（SECT_DEFS）— 4 条
**位置**：`src/cultivation/sect_system.cpp:8`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 宗门 ID |
| name | string | 中文名 |
| description | string | 描述 |
| skill_id | string | 入门专属技 ID |
| atk[3] | float[3] | 外门/内门/真传 攻击加成 |
| mana[3] | float[3] | 灵力上限加成 |
| regen[3] | float[3] | 回灵加成 |
| hp[3] | float[3] | 生命加成 |
| def[3] | float[3] | 防御加成 |
| kill_xp[3] | float[3] | 击杀修为加成 |

### 4.2 职位门槛 — 3 条
**位置**：`src/cultivation/sect_system.cpp:20`

```cpp
RANK_COST = { 0, 100, 300 } // 外门/内门/真传
```
字段：rank_name（外门弟子/内门弟子/真传弟子/散修）

---

## 五、功法系统（GongfaSystem）

### 5.1 功法定义表（GONGFA_DEFS）— 6 条
**位置**：`src/cultivation/gongfa_system.cpp:10`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 功法 ID |
| name | string | 中文名 |
| school | enum | SCHOOL_BODY（炼体）/ SCHOOL_QI（练气） |
| grade | enum | GRADE_HUANG（黄）/ XUAN（玄）/ DI（地） |
| max_layer | int | 最大层数（3/5/7） |
| hp_mult[layer] | float | 每层生命倍率 |
| atk_mult[layer] | float | 每层攻击倍率 |
| def_mult[layer] | float | 每层防御倍率 |
| mana_mult[layer] | float | 每层灵力上限倍率 |
| spell_mult[layer] | float | 每层法强倍率 |
| mana_regen[layer] | float | 每层回灵倍率 |
| … | … | 其他字段 |

明细：炼体系 3（莽牛劲/铁布衫/金刚诀）+ 练气系 3（吐纳诀/紫霞功/太玄经）

---

## 六、Buff 系统（BuffSystem）

### 6.1 Buff 定义表（BUFF_DEFS）— 6 条
**位置**：`src/cultivation/buff_system.cpp:9`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | Buff ID |
| name | string | 中文名（HUD 显示） |
| duration | float | 持续时间（秒） |
| atk_mult | float | 攻击乘区加值 |
| def_mult | float | 防御乘区加值 |
| elem | enum | 元素抗性类型 |
| elem_resist | float | 元素抗性加值 |

明细：冰心 / 赤焰 / 金刚 / 土盾 / 身外化身 / 玄龟护体

---

## 七、炼丹系统（AlchemySystem）

### 7.1 配方表（RECIPES）— 7 条
**位置**：`src/cultivation/alchemy_system.cpp:17`

| 字段 | 类型 | 说明 |
|---|---|---|
| result_id | string | 产物物品 ID |
| result_name | string | 产物中文名 |
| mats[3] | const char*[3] | 材料 ID（最多 3 种） |
| counts[3] | int[3] | 对应数量 |
| mat_count | int | 实际材料种类数 |
| grade | int | 产物品级 |
| min_realm | int | 境界门控（金丹=3 方可炼制地品） |
| success_rate | float | 成功率（v1 全 1.0） |
| desc | string | 效果简述（UI 用） |

---

## 八、法宝系统（ArtifactSystem）

### 8.1 法宝定义表（ARTIFACT_DEFS）— 3 条
**位置**：`src/cultivation/artifact_system.cpp:11`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 法宝 ID |
| name | string | 中文名 |
| kind | enum | KIND_ATTACK（攻击型）/ KIND_SUPPORT（辅助型） |
| category | enum | 伤害类别 |
| element | enum | 元素 |
| mana_cost | float | 祭出灵力消耗 |
| cooldown | float | 冷却 |
| power | float | 威力系数 |
| effect | enum | SkillSystem 效果类型 |
| proj_speed | float | 投射物速度 |
| proj_color | Color | 投射物颜色 |
| passive_def_bonus | float | 辅助型常驻防御加成 |

### 8.2 法宝槽数 — 硬编码
**位置**：`src/nodes/player.h` `get_artifact_slot_limit()`

飞升前 1 本命+2 次要，飞升后+3。建议外配。

### 8.3 温养参数 — 硬编码
**位置**：`src/nodes/player.h` + `src/cultivation/artifact_system.cpp`

本命 120%→150%→渡劫觉醒 200%，温养值、阈值均硬编码。

---

## 九、洲框架（ContinentManager）

### 9.1 洲定义表（CONTINENT_DEFS）— 4 条
**位置**：`src/core/continent_manager.cpp:17`

| 字段 | 类型 | 说明 |
|---|---|---|
| id | string | 洲 ID |
| name | string | 中文名 |
| scene | string | 场景路径 |
| spawn_x/y | float | 到达落点 |
| min_realm | int | 旅行境界门槛 |
| description | string | 描述 |
| gate_text | string | 锁定条件话术 |

---

## 十、突破事件（BreakthroughManager）

### 10.1 机缘事件表 — 11 条（每个境界一条）
**位置**：`src/cultivation/breakthrough_manager.cpp` `_event_for_realm()`

| 字段 | 类型 | 说明 |
|---|---|---|
| realm | enum | 触发境界 |
| kind | enum | NARRATIVE（叙事）/ COMBAT（战斗秘境）/ TRIBULATION（三灾） |
| name | string | 事件名 |
| waves | int | 战斗波数（COMBAT类型） |
| intro_lines | string[] | 开场叙事 |
| outro_lines | string[] | 成功叙事 |

事件明细：引气入体→百日闭关→三花聚顶→心魔劫→出窍游历→三尸劫→形神合一→了断尘缘→三灾利害→仙元圆满→…

---

## 十一、掉落系统（DropSystem）

### 11.1 掉落表 — 2 组（Boss / 普通）
**位置**：`src/core/drop_system.cpp:76`

| 字段 | 类型 | 说明 |
|---|---|---|
| item_id | string | 掉落物 ID |
| qty_min | int | 最小数量 |
| qty_max | int | 最大数量 |
| chance | float | 掉落概率 |

字段极少但结构简陋——按 `is_boss / is_ranged / is_flying` 三个 bool 分表。
应改为可配置的掉落组（按敌人 ID 或标签引用）。

---

## 十二、能力解锁（AbilityManager）

### 12.1 境界→能力映射 — 15 主动 + 7 被动
**位置**：`src/cultivation/ability_manager.cpp` + `game_menu.cpp` 的能力页

| 能力 ID | 解锁境界 |
|---|---|
| 冲刺(boundless) | 初始 |
| 二段跳 | 炼气 |
| 空中冲刺 | 筑基 |
| 短暂飞行 | 筑基 |
| 滑翔 | 金丹 |
| 自主飞行 | 金丹 |
| 元婴出窍 | 元婴 |
| 领域展开 | 元婴 |
| … | … |

能力页渲染在 `GameMenu::_build_ability_page()` 中硬编码了名称和解锁条件文本。

---

## 十三、角色属性（Player/Enemy）

### 13.1 Player 默认属性 — 散落各处
**位置**：`src/nodes/player.h:37-54`

```cpp
move_speed = 180.0f;      jump_velocity = -350.0f;
dash_speed = 500.0f;      dash_duration = 0.15f;   dash_cooldown = 0.4f;
wall_slide_speed = 80.0f; coyote_time = 0.08f;     jump_buffer_time = 0.1f;
max_health = 100.0f;      attack_damage = 10.0f;
```

### 13.2 飞行参数 — 硬编码
```cpp
fly_speed = 260.0f;       fly_acceleration = 250.0f;
flight_mana_cost_per_sec() = 10.0f (筑基), 0.0f (金丹+);
```

---

## 十四、敌人 archetype — Bootstrap 侧

**位置**：`scripts/bootstrap.gd` / 各洲 stub `.gd` / `world_common.gd`

每个 `spawn_enemy()` 调用的位置、属性（速度/探测范围/血量/伤害/realm）全在 GDScript 中逐行手写。
应定义 enemy archetype 模板（.tres 或 JSON），bootstrap 仅引用模板+位置。

---

## 十五、输入映射（Input Map）

**位置**：`project.godot` `[input]` section

当前 20+ 个 action：left/right/up/down/jump/dash/interact/cultivate/menu/inventory/
artifact_page/skill_a~h/consume_1~6/pressure_wei/pressure_lin。
每个 action 的键位绑定硬编码在 project.godot 中。应提供键位配置界面（设置页内）。

---

## 十六、UI 文本与布局

### 16.1 UI 固定文本
散落在各文件中的 `TXT("…")` 字符串——能力页、功法页、菜单提示等，均为硬编码中文。
建议统一为一个本地化表或至少集中到单个文件。

### 16.2 HUD 布局常量
**位置**：`src/nodes/game_hud.cpp` 顶部

`BAR_WIDTH/BAR_HEIGHT/BAR_X/HEALTH_BAR_Y/…` 均为硬编码像素值。
建议为可配 UI 皮肤布局。

---

## 优先级建议

| 优先级 | 系统 | 理由 |
|---|---|---|
| **P0** | 物品数据库 | 最频繁新增（新道具/装备/消耗品），tres 最成熟 |
| **P0** | 技能定义表 | 21 条，新增技能是主要内容迭代路径 |
| **P1** | 掉落表 | 数值调优刚需，当前结构过于简陋 |
| **P1** | 宗门/功法/Buff | 规则稳定后可抽，当前硬编码尚可维护 |
| **P2** | 配方表 | 7 条，新增配方是内容迭代 |
| **P2** | 境界参数 | 数值平衡调优时需外部化 |
| **P3** | 洲定义 | 4 条，新增洲才需要改 |
| **P3** | 突破事件 | 叙事文本天然适合外部资源文件 |
| **P3** | 能力解锁 | 15+7 条，境界调整时需同步 |
| **P4** | Player 属性 | 需引入角色模板/装备系统后才可抽 |
| **P4** | 敌人 archetype | 需定义 enemy 模板系统 |
| **P4** | 输入映射 | 需键位配置 UI |
| **P5** | UI 文本/布局 | 需本地化框架或 UI 皮肤系统 |

---

## 推荐外部格式

- **Godot `.tres` (Resource)**: 物品、技能、Buff、配方——可在编辑器中可视化编辑，C++ 侧
  已有 Resource 绑定基础（`src/resources/`）
- **JSON**: 境界参数、洲定义、掉落表——简单数值表，运行时加载
- **CSV**: 大批量数值表（如境界×期数矩阵）——策划用表格软件编辑

所有外部数据加载后缓存为 C++ runtime 结构（HashMap/数组），避免运行时重复解析。
