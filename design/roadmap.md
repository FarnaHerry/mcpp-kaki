# 开发路线图（截至 session 009）

## 一、最近修改总结

### Session 009（本次）：机缘突破事件系统
- **BreakthroughManager**（src/cultivation/）：所有突破机缘的唯一入口。
  Q → SignalBus `breakthrough_requested` → 受理分发（叙事/战斗秘境/三灾）；
  +`breakthrough_event_started/finished` 信号；叙事 overlay（暂停+逐行推进）
- **事件全覆盖**：引气入体/百日闭关/三花聚顶/出窍游历/形神合一/功德因果（叙事），
  心魔劫（金丹→元婴）/三尸劫（化神→炼虚，恶念→执念→贪欲三连 wave，属性随玩家境界缩放），
  三灾连考（大乘→渡劫之地→真仙）
- **TribulationController**：雷灾（预警落雷走位）→ 阴火（体内 DoT 生存）→
  赑风（控制反转+罡风推移+风蚀）；渡劫过渡态复用 DU_JIE（失败退回大乘，经验保持封顶）
- **经验封顶**：accumulate_energy 到顶卡境界（设计原定），过机缘后继续累计
- **秘境 arena**：heart_demon_arena.tscn / dujie_arena.tscn（复用 Portal 模式：
  场景加载 + 玩家重挂载 + 相机锁定，进入锁死直到分出生死）
- **F5 语义调整**：调试开关只免经验门槛，机缘事件始终触发
- **附带修复**：重生 Timer 被死亡暂停冻结（process_mode→ALWAYS，潜伏老 bug）；
  Enemy 加 `no_drops`（幻境之敌不掉落）；HUD 修为圆满显示「机缘已至 [Q]」

### Session 002-006（已提交前积压）
- SaveSystem 存档/读档（ConfigFile → user://savegames）
- Inventory / ItemDatabase / ItemPickup 物品体系
- 装备系统（武器/护甲/饰品三槽）+ InventoryPanel 背包 UI
- Projectile 抛射物、敌人种类（近战/远程/飞行/Boss 多阶段）
- 修仙体系全量定稿（design/cultivation-realms.md）：13 境界、int64 累计经验、
  期数（前/中/后/大圆满）、四轴（门派/五仙身份/出身/果位）、混元一气、
  TitleComposer 称号组合、三灾独立枚举、本命法宝、纳戒容量、能力门槛表

### Session 008（本次）
**系统**
- 灵力/修为拆分：修为经验（int64，HUD 只显示%）≠ 灵力法力池（放技能/催法宝，
  上限随境界，2%/s 自动回复，consume_mana 预留给技能）
- 生命上限随境界（100 × 防御倍率，突破回满）
- 突破调试开关 `_free_breakthrough`（默认 ON，F5 热切换；上线改 false）
- Q 键（cultivate 动作）触发突破
- 飞行系统：第 8 个 Player 状态 Fly（渐加速、速度随境界、空中攻击/冲刺后
  自动恢复飞行 was_flying）；筑基需飞剑+耗灵力，金丹+无条件
- DropSystem：所有掉落物的唯一入口（敌人种类掉落表 v1，公开 spawn_drop 接口）
- 飞剑物品（KEY_ITEM）

**架构**
- UI 三类分立：GameHUD（游戏 HUD）/ TelemetryPanel（遥测，F3）/ InventoryPanel
- HUD 开关接口：set_hud_visible / set_all_visible（过场动画预留）
- TXT() 统一文本编码（src/utils/text.h，禁直接 String("...")）
- C++23（我们的代码；godot-cpp 保持 17）
- 相机跟随增益随距离缩放（高速飞行不落后）

**重要 bug 修复**
- ItemPickup 六年老 bug：set() 需要 ADD_PROPERTY，否则全部拾取物失效
- bootstrap.gd 启动顺序（portal_prompt Nil、Polygon2D 匿名节点）
- String(const char*) Latin-1 乱码 → 全面 TXT()
- 物理刷新期改状态报错 → call_deferred / set_deferred
- InventoryPanel::refresh 信号参数不匹配
- 读档顺序：境界恢复后再回填存档血量

## 二、后续计划（按优先级）

1. ~~**机缘突破事件**~~（session 009 已完成 v1）——后续可做：机缘事件失败惩罚细化、
   心魔用玩家外观/招式、三灾「硬抗 vs 躲避」双过法（肉身/元神分叉检验）
2. **洞天系统**（design/dongtian.md 待定稿）：炼虚解锁随身小世界，
   安全处 G 键进出 → 后花园种植/聚灵阵/扩张
3. **技能/法术系统**：消耗灵力的主动技能（飞行已有 consume_mana 接口）
4. **法宝系统完善**：次要法宝物品、槽位 UI（飞升前 3/后 6）、温养来源；
   渡劫之地「只带本命法宝」规则未生效（v1 未禁用装备加成）
5. ~~**食物/辟谷**~~（session 007 已完成 v1：饱食度衰减/饥饿debuff/食物倍率/筑基辟谷/洞天种灵米）
6. ~~**地府/生死簿/勾魂**~~（session 006 已完成 v1：寿元/勾魂使/黄泉路/免死；后续可做 地府审判/鬼仙路线/改簿永生）
7. **数值平衡**：攻防速倍率、掉落表、灵力消耗、心魔/三灾数值均为草案
8. ~~**工程清理**~~（session 006 已完成：Player/GameManager 析构 memdelete 成员，退出 0 泄漏）

## 三、需要抽成 OOP 的候选

| 现状 | 抽取方向 | 优先级 |
|---|---|---|
| 物品硬编码在 ItemDatabase::_register_items | Item 定义迁移 .tres Resource（数据驱动，CLAUDE.md 原定方向） | 高 |
| 掉落表硬编码在 DropSystem::_roll_drops | DropTable Resource + 稀有度/品质系统 | 高 |
| 敌人种类 = Enemy 上的 bool 标志（is_ranged/is_flying/is_boss） | EnemyDefinition Resource 或行为组件（RangedBehavior/FlyMovement/BossPhases） | 高 |
| 技能不存在，只有 consume_mana 接口 | Skill 基类 + SkillManager（法术/神通继承体系） | 高 |
| 法宝 = Player 上的 benming 字段 + flying_sword 特判 | Artifact 类 + ArtifactManager（槽位/温养/觉醒统一） | 中 |
| bootstrap.gd 里手写场景搭建（敌人/拾取物/传送门坐标） | WorldBuilder / LevelManager C++ 类 + 场景数据 | 中 |
| ~~机缘事件不存在~~ | ~~BreakthroughEvent 基类 + EventManager~~（session 009 已实现：BreakthroughManager + TribulationController） | ~~中~~ |
| 交互提示两处重复（bootstrap _interact_hint + GameHUD _interact_label） | 统一收进 GameHUD（SignalBus interaction_prompt 已有） | 低 |
| 食物/buff 不存在 | BuffSystem（食物/丹药/状态统一为 Buff） | 低 |
