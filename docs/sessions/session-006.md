# Session 006 — 地府 / 生死簿 / 勾魂 v1

**日期**: 2026-08-05
**分支**: main
**起始提交**: 18a199a（炼丹卡片化 + 背包筛选）
**结束提交**: （本会话提交）

## 完成内容

### SoulLedgerSystem（`src/core/soul_ledger_system.*`，生死簿独立数据系统）

设计定稿"不塞 CultivationSystem"（cultivation-realms.md L490），独立 Node。

- **簿上寿元 = 物种默认（凡人 100）**，实际寿元随境界（`lifespan_for_realm`：凡人100/炼气150/筑基250/金丹500/元婴2000/化神5000…）——信息差是勾魂错抓的默认状态
- API：`get_ledger_lifespan/get_actual_lifespan/get_original_body/get_origin_name`、`mark_soul_exempt()/consume_soul_protection()`（改簿划名=免死一次）、`was_killed_by_reaper(Node*)`、`save_to_dict/load_from_dict`
- 存档 `pd["soul_ledger"]`；HUD 寿元小标签「寿 簿上/实际」（实际>簿上绿）+ SignalBus 新信号 `lifespan_changed/soul_protection_changed/ledger_inspect_requested`
- 挂载：world_common.gd `WC.setup` 在 GameManager 后创建，`set_player` + `gm.set_soul_ledger`

### 勾魂使者（黑白无常）

- `SoulLedgerSystem::_process`：玩家 HP<20%（濒死）+ 玩家在主场景根 + 非地府 → 刷黑/白无常（Enemy `is_soul_reaper`+`no_drops`+`show_hp_bar`+`realm≥玩家`，血量=玩家0.7，伤害=玩家HP0.12）
- 回血>50% 复位本轮触发（每场限次）；反杀双无常 → +30 修为 + 提示「反杀勾魂使！地府暂记错抓」；重生清无常
- **Player 加 `last_damage_source`**（`_take_damage_typed` 统一记录，respawn/读档置 nullptr）→ 死亡路由判断是否勾魂使击杀

### 死亡三分支（`GameManager::on_player_died`）

1. **免死**（最高优先）：划名未用 → 原地满血复活，不暂停不进 GAME_OVER（emit player_respawned 清 overlay/勾魂状态 + 2s 提示自动清）
2. **勾魂使击杀** → `enter_difu(true)` 魂魄入地府
3. **地府内死亡** → `huan_yang()`（防 respawn 放 difu 本地坐标）/ **正常死亡** → 原回检查点

`enter_difu/huan_yang`：`set_pause(false)` + `collect_save_data` → `set_travel_bridge` → `set_travel_target` → `change_scene_to_file`（**不走 `request_scene_change`**，防污染 `_respawn_scene`；还阳落点=检查点）。

### 地府场景 + 交互

- `scenes/continents/difu.tscn` + `scripts/continents/difu.gd`（黄泉路：判官/生死簿/还阳出口）
- `UnderworldInteractNode`（`src/nodes/underworld_interact.*`，StorageChest 模板）：`MODE_INSPECT` 查簿（HUD overlay 显 出身/原身/簿上寿元/实际寿元/划名状态）、`MODE_AMEND` 改簿划名→免死一次
- `SceneGate`（`scripts/gates/scene_gate.gd`）：通用场景门，↑ 调 `gm_method`（enter_difu/huan_yang）
- 东胜神洲入口：bootstrap.gd 花果山段（x=6250）`DifuGate` → 地府；地府 `HuanYangGate` → 还阳检查点

### 测试

`scripts/test_difu.gd` 全链路（21 步）：数据初始化 → 改簿免死单元 → 寿元随境界+信号 → 入口进地府 → 判官查簿 → 改簿划名 → 还阳 → 免死复活 → 濒死刷无常 → 勾魂死亡入地府 → 还阳 → 反杀+60修为。**全量回归通过**（test_double_jump 之前的偶发失败本次通过，属时序抖动）。

### 关键坑

- `Callable(this, "_enter_difu_from_death")` 必须 `ClassDB::bind_method`（CLAUDE.md 潜伏 bug 教训重演，未绑定时 timeout 静默失效）
- `constexpr Vector2` 不合法 → `static const Vector2 DIFU_SPAWN` + cpp 定义
- 场景切换新建 SignalBus，测试需重连
- 无常 spawn 后 0.2s 内触发勾魂死亡（无常很快能杀死 10% 血玩家）

## 追加：工程清理——退出时 ObjectDB 内存泄漏（2026-08-05 补）

- **根因**：`memnew` 创建的 Object（非 RefCounted、非 Node）成员不随拥有者释放。
  - `Player` 的 9 个系统成员（`_cultivation/_abilities/_gongfa/_skills/_artifacts/_buffs/_sect/_alchemy/_inventory`）
  - `GameManager::_save_system`（SaveSystem）
- **修复**：`~Player()` / `~GameManager()` 析构 `memdelete` 各成员（null 守卫）
- **效果**：简单载入 + 地府全链路（场景切换/战斗/无常 spawn）均 0 泄漏（原 11 个）
