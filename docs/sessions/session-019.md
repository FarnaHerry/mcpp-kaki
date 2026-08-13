# Session 019 — 元婴分叉（肉身/元神）+ 渡劫 v2（三灾齐至 + 天罚使 Boss）

**日期**: 2026-08-13
**分支**: main
**前置**: session-018（渲染分辨率与窗口解耦）

## 需求

1. roadmap 收尾项：元婴分叉（肉身成圣 vs 元神修炼）+ 三灾双过法（design/cultivation-realms.md）。
2. 用户：「最后的渡劫不够难，也不够直观——三灾同时出现，调整三灾效果，然后出现一个 boss 作为考验
   （boss 未定好，可随意发挥，后期可能修改）」。

## 元婴分叉（CultivationSystem）

- 双轨经验 `_path_body_exp/_path_spirit_exp`（`feed_path(0=肉身/1=元神)`，100 经验 1 级共 5 级，元婴起喂养）。
- 喂养挂钩：武技施放→肉身+2 / 法术·神通·仙法→元神+2（skill_system cast_slot）；
  近战击杀→肉身+4 / 投射物击杀→元神+4 / 受击→肉身+1（player）。
- 加成：肉身→物攻+3%/级（get_effective_attack）+防御/生命+3%/级（折入 get_defense_multiplier，
  get_max_health=100×def 同步吃）+三灾减伤 8%/级；元神→法强+3%/级（两处 spell_mult）+
  灵力上限+3%/级（get_max_mana）+法则回复+5%/级（tick_law_regen）。
- 合体「形神合一」：跨 HE_TI 时弱侧补 80% 差值、`_path_merged`，此后双轨同步喂养。
- focus 称号轴自动跟随高侧（`_update_focus_from_paths`，choose_focus 手动选择仍保留）。
- 存档 `cd["path_body"/"path_spirit"/"path_merged"]`；功法页新增分叉分区
  （等级/进度/加成明细，已汇合显示「形神合一」）。

## 渡劫 v2（TribulationController 重写）

- **三灾齐至**：删 Phase 分阶段，雷/火/风全程并发——
  落雷 2.2s 间隔/1s 预警（Boss 半血激怒→1.4s）、阴火 1s tick 常压、
  赑风罡风推移+风蚀+**控制反转按阵风周期重掷**（不再全程锁反转）。
- **天罚使**：劫云化身 Boss（Enemy 运行时装配，雷法远程 is_ranged，HP2500 显式值压过 ×5 补偿，
  realm 镜像玩家=9，no_drops）——**斩之即渡劫成**（`boss_died`→`tribulation_finished(true)`）。
  半血激怒：三灾加剧（雷 1.4s/风 1.8s/阴火 tick×0.7）。Boss 后期可换（用户预留）。
- 双过法联动：肉身等级→三灾伤害×(1-8%/级)（硬抗道）；元神等级→雷预警+15%/级、
  风反转概率-18%/级（min 10%）、阴火辅减免 6%/级（躲避道）。
- 失败路径不变：战死→abort（天罚使清场+效果还原）→退回大乘，经验保持封顶可重试。
- intro/outro 文案更新（「三灾齐至，天罚使代天行罚」「斩天罚使则飞升成仙」）。

## 坑

- **Enemy 自身 `boss_died` 信号只声明从未 emit**——`enemy.cpp` 只发 SignalBus 版，
  天界巨灵神的 `jl.connect("boss_died")` 一直是死连接。已在 `emit_signal("enemy_died")` 旁补发
  自身 `boss_died`（is_boss 守卫），天罚使/巨灵神同步生效。
- cultivation.cppm 有**前置声明** `export class TribulationController;`（第 29 行）——
  python 文本替换块定位必须匹配带 `: public Node` 的完整定义，否则从第 29 行切到 namespace 尾，
  把整个模块类定义全删（本次翻车一次，git checkout 恢复后改精确匹配）。
- GDScript 无 `bool()`/`float()` 构造——Variant 断言用 `== true`/`call()` 绑定 getter；
  Player `current_health`/`input_inverted` 是裸成员无属性，新增 `is_input_inverted()` 绑定探针。

## 测试

- `test_paths.gd`（23 断言）：元婴前喂养无效/等级/四维加成/focus 跟随/封顶/合体汇合 80%/
  汇合后同步+封顶/存档字段/功法页分叉分区。
- `test_tribulation_v2.gd`（19 断言）：controller 启动/realm=9/天罚使配置（名/realm/no_drops/HP2500）/
  HUD 血条/进场即反转/挂机持续掉血（三灾并发）/落雷在场/斩杀→真仙/效果还原；
  失败路径：再入→战死→退回大乘+Boss 清场+controller 撤+反转还原。
- 回归：paths/tribulation_v2/breakthrough/lingxiao/combat/bossbar/multi_bossbar/artifacts/gongfa/skills 全绿；
  全量批量回归见尾注。

## 遗留

- 天罚使数值/招式为即兴版（HP2500 雷法远程），用户预留后期换 Boss——换时只需改 `_spawn_boss`。
- 真机验证：渡劫战手感（三灾并发压力 vs 双过法减免是否够「难而直观」）。
