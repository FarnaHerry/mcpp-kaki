# 开发路线图（截至 session 014）

## 一、最近修改总结

### Session 014（本次）：多 agent 并行（法宝完善 / 仙元体系 / 洞天设施）
- **流程**：3 并行 agent worktree（F 法宝 / G 仙元 / H 洞天设施），主线没想好用户拍板先做 234；
  session 中途用户实测发现**法宝页纯只读无法选择/创建本命** → 补发给法宝 agent 一并做；
  合并 G→H→F，仅 game_menu.cpp 被 F/H 同改（区域不同）自动并无冲突，全量回归绿
- **法宝完善 v2**：3 件次要法宝（八卦炉攻+15%/捆仙绳瞬身束缚一击/定风珠风抗+30%，化神/炼虚/合体赐残篇）+
  飞升真仙解锁 6 槽（`unlock_secondary_slots`）+ 渡劫只带本命法宝（`enter/exit_tribulation` flag 置空装备加成）+
  **法宝页可交互**（↑/↓ 选法宝、X 设本命、A~H 装槽）
- **仙元体系（飞升后）**：地基本已在，本次收尾——飞升凡尘修为清零（灵力转仙元）+ 金仙门槛 999,999（9系）+
  HUD 修为条真仙+ 显「仙元」
- **洞天设施**：灵泉打坐点（X 打坐吃聚灵阵倍率）+ 丹房（X 就地炼丹，GDScript 复刻炼丹页）+
  灵植采集点×2（聚灵草/千年灵芝，枯萎现实时间复生）

### Session 013：多 agent 并行（天界/分身/平衡/洞天v4/龙宫）
- **流程**：4 并行 agent worktree 隔离（A 天界 / B 身外化身 / C 平衡 / D 洞天v4）+
  kimi k3 配额耗尽后 2 个新 agent 失败 → C 的未提交工作主 agent 抢救提交、
  D 与第 5 项「龙宫秘境」由主 agent 亲做；合并 C→B→D→A 无冲突，全量回归绿
- **天界 v1**（真仙 realm 10，北俱芦洲南天门 ↑ 登天，不渡云海）：南天门外（天兵+增长天将）→
  天庭街市（琼楼高台+隐藏秘藏）→ 兜率宫+蟠桃园（老君丹炉+蟠桃）→ 巨灵神 Boss（金仙级 HP4000）
- **身外化分身实体**：CloneAvatar 协同作战 30s（属性快照 HP50%/攻60%/速80%，索敌近战，
  击杀修为转发本体，至多 2 个顶老），占位 buff 升级
- **平衡遗留 5 项**：三灾元素结算（雷/火/风走 DMG_ELEMENTAL，防御不可减）/ 心魔镜像 realm 同境 /
  普攻走 get_effective_attack 全乘区 / AlchemySystem 接 recipes.json / Boss ×5 血量幂等补偿
- **洞天 v4**：扩张碑 X 买灵田 6→12 块（灵石递增）+ 阵眼 X 升级聚灵阵 0→2 级（打坐 +0.5/级）
- **东海龙宫秘境**（东胜神洲 x=8600 入口）：弱水走廊禁飞 + 虾兵/蟹将/镇守将 +
  避水珠（**装备元素抗性管线**：Item elem_resist[8] 进 take_damage_typed）+ 千年珍珠

### Session 012：灵石四阶通用货币（独立钱包 + 兑换）
- **CurrencySystem**（`src/core/currency_system.*`）：下品/中品/上品/极品 四档，每档 ×10 价值，
  独立钱包不占背包；spend 自动破零找零、exchange 保值兑换；存档 + 老档迁移
- **拾取路由**：Item 加 `currency_tier`；灵石拾取直入钱包；SignalBus `currency_changed`
- **商店**：ShopSystem 走钱包；ShopPanel 四阶余额 + 第三栏「兑换」（Q/E 三栏循环，6 条保值兑换 X 全额）
- **投放**：boss 掉中品、三星洞中品、玄冰窟上品、玄冥上品×3、南天门极品

### Session 011（本次）：数值平衡（4 agent 并行分析 + 主 agent 统筹）
- **流程**：4 只读分析 agent 并行（成长曲线/敌人Boss/技能法宝/经济掉落）→ 主 agent 交叉核对定值
- **成长曲线**：修为门槛金丹起 ~3.3×/境；击杀修为随境界 `15×(1+realm)`；金仙/天尊攻防平滑（32/30、60/50）
- **敌人/Boss**：三 Boss HP ×10（魔狼150/螭龙300/玄冥3000 攻100）；漏配 HP 杂兵补齐；北俱/南赡后期威胁提升（攻 30~100）
- **技能**：突进斩 5.0→3.5；破空斩硬编码 cd/power 互换修正；灵压耗灵 60→45
- **经济**：炼丹成品卖价 ≤ 材料卖价（堵印钞）；玄龙丹 攻防20% 900s；人参果 800→600；灵石掉率 1~3→2~4

### Session 010（本次）：北俱芦洲 v1 + 寿元无限 + 物品说明
- **北俱芦洲 v1**（design/world-map.md v5，渡劫门槛 realm 9，最后一洲）：
  极北冰原（冰面打滑 IceZone：Idle 摩擦骤减 + Run 渐进加速）→ 玄冰高原
  （极寒 ColdZone：减速30% + 冰伤 dot，冰心丹可减免；玄冰窟秘境）→
  上古荒原（上古巨兽·玄冥 Boss realm 10 → 炼体圣地 RefineSpot → 南天门序章）
- **新数据**：龙骨/玄冰参/玄龙丹（渡劫丹方 攻防+15% 600s）；buff_lianti/buff_xuan_long
- **寿元无限**：成仙（真仙 realm 10+）实际寿元 ∞（HUD 显示「寿 簿上/∞」金），勾魂使不再刷
- **物品说明**：items.json 全物品 desc 补全/加强 + 背包面板选中项显示说明行

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
2. ~~**洞天系统**~~（session 009 洞天 v1 + v2 种植 + v3 聚灵阵 + session 013 v4 扩张经营 +
   session 014 设施补全：灵泉打坐点/丹房就地炼丹/灵植采集点×2；design/dongtian.md）——
   后续可做：洞天内小怪/更多采集点
3. ~~**技能/法术系统**~~（session 010 已落地 SkillSystem 管线：主动 13/被动 6 + 8 槽装配 + FX 9 种）——后续：组合技/派生
4. ~~**法宝系统完善**~~（session 011 ArtifactSystem v1 + session 014 v2：次要法宝×3 补全/
   飞升解锁 6 槽/渡劫只带本命法宝规则生效/**法宝页可交互**——可选中、X 设本命、A~H 装槽）——
   后续可做：法宝图标/温养来源扩充
5. ~~**食物/辟谷**~~（session 007 已完成 v1：饱食度衰减/饥饿debuff/食物倍率/筑基辟谷/洞天种灵米）
6. ~~**地府/生死簿/勾魂**~~（session 006 v1 + session 008 进阶：审判叙事 + 划名阴寿豁免；地府入口正式搬到南赡部洲长安）
6b. ~~**南赡部洲（长安坊市）**~~（session 008 已完成 v1：商店系统灵石买卖 + 五庄观人参果 + 地府入口）
6c. ~~**西牛贺洲**~~（session 009 已完成 v1：火焰山环境火伤+芭蕉扇灭火 + 灵台方寸山三星洞秘境 + 流沙河弱水禁飞）
6d. ~~**北俱芦洲**~~（session 010 已完成 v1：冰面打滑/极寒/玄冰窟/玄冥 Boss/炼体圣地/南天门序章）
6e. ~~**天界**~~（session 013 已完成 v1：南天门登天真仙门槛 + 兜率宫蟠桃园 + 巨灵神守关）——
   后续可做：凌霄宝殿主线叙事、飞升结局
6f. ~~**东海龙宫秘境**~~（session 013 已完成：弱水走廊 + 虾兵蟹将/镇守将 + 避水珠/千年珍珠）
7. ~~**数值平衡**~~（session 011 一轮：成长曲线/敌人Boss/技能/经济 +
   session 013 遗留 5 项补齐：三灾元素结算/心魔realm/普攻全乘区/recipes.json/Boss×5 幂等）
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
