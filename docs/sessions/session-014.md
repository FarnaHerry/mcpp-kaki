# Session 014 — 多 agent 并行（法宝完善 / 仙元体系 / 洞天设施）

**日期**: 2026-08-10
**分支**: main
**起始提交**: be3a54c（session-013 文档）
**结束提交**: （本会话提交）

## 缘起

用户在天界收尾主线没想好时拍板：「先做 234」——即 roadmap 里的
**② 法宝系统完善 / ③ 仙元体系 / ④ 洞天设施补全** 三块，主线后置。
三个任务边界独立（artifact / cultivation / dongtian），并行 3 个 agent worktree 开发。

session 中途用户实测发现**法宝页是纯只读**——无法选择、无法创建本命法宝——
补发给法宝 agent 一并做掉。

## 编排

| 任务 | agent | 分支 | 提交 | 结果 |
|---|---|---|---|---|
| 法宝系统完善 | F | artifact-v2 | 7b21f21 | 合并 fb360b6 |
| 仙元体系（飞升后） | G | xianyuan | 824f951 | 合并 98d25bc |
| 洞天设施补全 | H | dongtian-facilities | dc167fb | 合并 4eb8fa9 |

三个 agent 这次都正常跑完（k3 配额够用），无主 agent 抢救。合并顺序 G→H→F，
仅 game_menu.cpp 被 F/H 同改（H 加 ESC 守卫 / F 改法宝页，区域不同）→ git 自动并无冲突。

## 各任务内容

### 法宝系统完善 v2（agent F，7b21f21）
- **3 件次要法宝**（ARTIFACT_DEFS + items.json learn_artifact，化神/炼虚/合体境界突破赐残篇入包，
  X 使用习得）：**八卦炉**(辅助·攻+15%) / **捆仙绳**(攻击·FX_BLINK 特例：300px 索敌→瞬身贴身
  →suppress(2.5) 束缚→一击) / **定风珠**(辅助·风抗+30%)；均按次要系数 1.0→1.2→1.5 两段温养缩放。
- **飞升 6 槽**：真仙 realm_changed → `unlock_secondary_slots()`（次要 2→5，共 6 槽），
  `get_slot_limit()`=解锁标记或境界兜底（旧档兼容），存档 `secondary_unlocked`；
  GameMenu/HUD 本就按 slot_limit/locked 自适应。
- **渡劫「只带本命法宝」规则生效**：`Player::enter/exit_tribulation`（flag 置空式——
  装备攻/防/速/元素抗性全豁免，**不动背包/装备槽数据**，本命法宝保留）；BreakthroughManager
  进 arena `_enter_tribulation` + 成败 `_on_tribulation_finished` + 战死 `_fail_cleanup` 双路恢复；
  HUD 渡劫中次要法宝槽灰显。
- **法宝页可交互化**（用户实测发现没有）：槽位总览 3×2 只读格（本命金色/锁定灰「飞升解锁」）+
  已拥有法宝**可选中 GridList**（↑/↓←/→）+ **X 设本命**（觉醒后拒「本命已锁定」）+
  **A~H 装入对应槽** + 选中详情行 + 操作提示行。

### 仙元体系（飞升后）（agent G，824f951）
- **关键发现：地基早已在 main**——`_xianyuan` 字段、`get/set_xianyuan/is_immortal`、
  存档 `cd["xianyuan"]`、HUD 法力条「灵力→仙元」都已实现，本次是**收尾纠偏**。
- **飞升九九归一**：`_set_realm_internal` 入真仙时 `_xianyuan=0` + `_lingqi=0`
  （凡尘修为清零，呼应"渡劫后灵力清零转仙元"）。
- **9 系门槛**：金仙 REALM_CAPS 500000→**999,999**（真仙 99,999 本即符合）；
  大圆满是金仙经验满的期数、混元走 `attain_hunyuan()` 特殊解锁（设计原文"不是经验堆出来的"）。
- **HUD**：修为条前缀随境界切换（真仙+ 显「仙元 N%」），`_on_language_changed` 按当前境界重取前缀
  （原无条件重置灵力，仙阶切语言会错显）。
- 未碰 player.cpp（`gain_spiritual_energy→accumulate_energy` 已按境界自动路由仙元）。

### 洞天设施补全（agent H，dc167fb）
- **灵泉打坐点 MeditateSpot**（scripts/spots/meditate_spot.gd，x=415 灵泉右）：
  X 经 `Input.action_press("cultivate")` 复用 Player Q 打坐管线（入坐/收功同键），
  吃聚灵阵倍率；提示激活期 attack_just_pressed 被 `_interact_prompt_active` 压制（X 不出刀）。
- **丹房 PillLab**（scripts/spots/pill_lab.gd + pill_lab_panel.gd，x=316）：X 开 PillLabPanel
  （CanvasLayer 115，GDScript 复刻 GameMenu 炼丹页：GridList 3 列卡片 + ↑/↓←/→ + X 炼制 + ESC/O 关，
  打开暂停还原原态），复用 `AlchemySystem get_recipe_list/craft/get_last_message`；
  game_menu.cpp +4 行 ESC/I 防抢守卫。
- **灵植采集点×2**（scripts/spots/dongtian_herb_spot.gd，StorageChest 模板）：聚灵草×2(120s)/
  千年灵芝×1(600s)，采集入包+喂练气+2，枯萎变暗 0.5s 轮询复生；状态宿主 DongtianManager
  `HERB_SPOTS` 静态表 + `get/gather/debug_age_herb_spot`，存档 `data["dongtian"].herb_spots` 段。
- 摆点 dongtian.gd：3 设施 + 2 个浮空苗圃单向高台（x=296/420，跳跃可达）。

## 测试

- 新增 3 个测试：test_artifact_v2.gd（48 PASS）/ test_xianyuan.gd（38 PASS）/
  test_dongtian_facilities.gd（29 PASS）。
- 三方各自全量回归均绿；合并后主 agent 在 main 上复跑全量（见末行）。
- 已知 flake：test_double_jump 偶发 FAIL（时序敏感，单跑 main/worktree 均过，与本次无关）。

## 关键坑

- **worktree GDExtension 不加载**：新建 worktree 缺 `.godot/` 缓存（gitignored），
  .so 不被 dlopen → 扩展类全未注册。解法：`godot --headless --import` 一次，或 `cp -r main/.godot`。
  三个 agent 都踩到，已记入各自交付备注。
- **渡劫禁用用 flag 置空而非物理卸下**：不动背包/装备槽数据，恢复零成本；
  三灾全元素结算中装备抗性同样豁免。
- **法宝页交互激活期 X 压制**：复用 `_interact_prompt_active`，避免提示激活时 X 误出刀/瞬断打坐。

## 合并后回归

主 agent 在 main 全量复跑：**49 PASS / 1 FAIL**——唯一挂的 `test_difu.gd` 单跑 **ALL PASS**
（地府测试涉及读档重生，批量并发共享 user:// 存档易受污染的 flake，非本次破坏）。三方共存无回归。
