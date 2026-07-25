# 宗门系统 + 威压/灵压系统（定稿）

> 2026-07-25 定稿。宗门=凡间拜师修炼体系（数值/专属技/职位），
> 与 CultivationSystem 的「门派轴」（大罗/太乙/散仙，真仙后称号，无数值）是两个概念。
> 威压/灵压按需求先**分开**实现（两个键、两套结算），后续可合一。

## 一、宗门系统（SectSystem）

### 1. 四宗一散

| id | 宗门 | 宗旨 | 职位加成（外门/内门/真传） | 入门专属技 |
|---|---|---|---|---|
| shushan | 蜀山剑派 | 剑修攻伐，一剑破万法 | 攻击 +6%/10%/15% | 武技「万剑归宗」（剑气成扇，金系） |
| kunlun | 昆仑道宗 | 练气长生，道法自然 | 灵力上限 +10%/16%/24%，回灵 +10%/15%/22% | 法术「太清神光」（水系神光） |
| penglai | 蓬莱仙岛 | 性命双修，寿与天齐 | 生命 +8%/12%/18%，防御 +4%/6%/10% | 法术「玄龟护体」（自buff 防+25%） |
| moluo | 魔罗教 | 杀伐精进，以战养战 | 击杀修为 +15%/25%/40%，攻击 +3%/5%/8% | 武技「血影斩」（突进血斩） |
| （无） | 散修 | 无门无派，自由自在 | 无 | 无 |

### 2. 规则

- **拜师**：炼气（realm≥1）可拜；GameMenu「宗门」页选宗拜入（v1 无实体驻地，云游四方皆可拜师；驻地/接引使后做）。
- **职位**：外门(0 起)/内门(贡献 100)/真传(贡献 300)；加成档位随职位升。
- **贡献**：击杀 +1，Boss +10（SignalBus enemy_killed，killer=玩家）。
- **叛门**：自由，贡献清零；已学专属技**保留**（逐出师门不夺修为）。
- **存档**：`pd["sect"] = {id, contribution}`。
- **乘区挂钩**（全走既有 `_refresh_regen_mults` 组合点）：
  - 攻 → `Player::get_effective_attack` ×sect
  - 灵力上限/回灵 → `set_mana_max_mult(功法×宗门)` / `set_mana_regen_mult(功法×被动×宗门)`
  - 生命 → `_refresh_max_health` ×sect；防御 → take_damage 防御乘区 ×sect
  - 击杀修为 → SectSystem 听 enemy_killed，追加 base(15/150)×(mult-1)

## 二、威压 / 灵压（先分开）

### Enemy 侧基础

- 新增 `realm` 字段（int，大境界，默认 0）+ ADD_PROPERTY；bootstrap 按怪标注
  （小怪 0~1、精英 2~3、BOSS 赤瞳魔狼 2 / 幽谷螭龙 4；三洲 stub 按洲境标注）。
- 新增 `suppress(t)`：**慑服**——`_suppress_t` 倒计时期间不跑状态机（定身），
  modulate 压灰，结束复原。
- `Enemy::_ready` 加入 `enemies` group（施放扫描用）。

### 威压（V 键，pressure_wei）

- 耗灵 30，冷却 8s，半径 240px，金色环波。
- 生效：`enemy.realm < player.realm` → 慑服 `2 + 0.5×境界差` 秒（cap 5s）。
- 无效：realm ≥ 玩家（高人不吃这套）。
- **护佑/反弹**：半径内有 `realm ≥ 玩家` 的高级修士 → 其周身 300px 低阶全免，
  且玩家遭**反噬**：5% 生命震荡 + 提示「对方有高人坐镇」。

### 灵压（R 键，pressure_lin）

- 耗灵 60，冷却 15s，半径 200px，紫黑环波。
- 生效（"低很多"）：`enemy.realm ≤ player.realm - 2` → 法术伤害
  `攻击 × (2 + 0.5×差)`（走 spell_resist）；`差 ≥ 4` → **直接镇杀**（元婴镇凡人/炼气）。
- 无效：差 < 2（境界接近，灵压不侵）。
- **护佑/反弹**：同威压——高级修士在场护佑低阶免伤，玩家反噬 8% 生命。

### 交互提示

- 命中数/反弹/无目标，经 SignalBus `interaction_prompt` 底部闪现（2s）。
- v1 无 UI 冷却条（内部计时），无存档。

## 三、实施清单

1. SectSystem（src/cultivation/）+ 注册表 + 乘区挂钩 + 存档 + 贡献
2. 技能表 +4（万剑归宗/太清神光/玄龟护体/血影斩）+ buff_xuan_gui
3. GameMenu 第 6 页「宗门」（背包/能力/功法/技能/法宝/**宗门**/云游/炼丹/设置，9 页）
4. Enemy realm/suppress + enemies group + bootstrap realm 标注
5. Player 威压/灵压（input map 加 pressure_wei=V / pressure_lin=R）
6. harness：test_sect.gd + test_pressure.gd + 全量回归
