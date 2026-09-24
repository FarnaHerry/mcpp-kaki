# 开发路线图（截至 session 023 + 2026-09-24）

## 〇、路线图状态刷新说明（2026-08-28）

第二节/第三节的勾选状态已按实际进度刷新（session 015-021 及 8/17-8/23 提交），
第一节的历史总结（session 002-014）保留原样作为记录；session 015-021/022-023 概要见下方追加块。

### Session 015-021 概要（详见 docs/sessions/session-015~021.md）

- **015/016**（未单独成文，并入 017 主题）：显示设置迭代前奏（动态 gcd 视口尝试）
- **017**：分辨率终案 v1——引擎托管整数缩放（Celeste 式 keep+integer）；凌霄宝殿主线叙事 +
  飞升结局（NarrativeNode/CondPortal 通用组件 + GameManager flags 持久化 + boss_dead flag）
- **018**：渲染分辨率与窗口解耦终案（16:9 固定基准 + aspect=keep 黑边 + 分数倍缩放 +
  动态帧率档（系统刷新率上限）+ 垂直同步；game_menu 显示设置 v7 轮次）
- **019**：元婴分叉（肉身/元神双轨喂养 + 合体形神合一）+ 渡劫 v2（三灾齐至并发 + 天罚使 Boss
  斩之即飞升 + 双过法减免联动）；Enemy 自身 boss_died 信号补发修复
- **020**：敌人定义数据化（EnemyDatabase + enemies.json 42 条）+ 掉落表 v2（tables/min_realm/
  Boss 专属/精英表）+ 品级五色（凡白/灵蓝/地紫/天金/仙青，光柱/格子底染）+ 功法四阶（黄玄地天）
  + Boss 血条境界名；多 agent worktree 并行（baseRef=head 流程）
- **021**：四洲秘境副本（古剑冢/大雁塔地宫/地心火窟/荒古冰墓，Portal 房间模式 + 数据契约预注册）+
  DropSystem 掉落挂点修复（挂玩家当前父节点，房间内击杀可拾取）

### Session 022-023 概要（8/31-9/24 提交，未单独成文，git log 为准）

- **022 · 设置与外抽**（8/31）：键位配置 UI——设置页「键位」子页（InputMap 运行时改绑 32 项、
  冲突拒绑、user://keybinds.cfg 持久化、恢复默认）；能力解锁表外抽 data/abilities.json
  （22 条=15 主动+7 被动，AbilityManager ensure_loaded JSON 优先+兜底、能力页渲染接表）
- **022 · 洞天补完**（8/31-9/13）：药童（委托照料自动收获成熟灵田入洞天仓库）+ 灵兽栏
  （闯阵灵兽半血以下 X 降伏入驻，打坐倍率 +5%/只 上限 3）；傀儡 NPC（下品灵石×1500 激活，
  驻守=灵田生长+50% / 随行=弱化分身实体出战，战毁 60s 冷却重召）
- **022 · 渡劫与雷系**（8/31-9/13）：天罚使升级正式多阶段 Boss（tian_fa_shi 入 enemies.json，
  三阶段=雷球直射/雷链扇形弹/雷域落雷圈 + 潜伏开火 bug 修复）；雷元素管线补全——敌人投射物
  proj_element 字段数据驱动（雷兽/天罚使/珠母精弹体走 ELEM_LEI 结算吃雷抗）+ 雷纹佩
  （雷抗20% 地品，天界飞檐秘藏投放）+ 雷弹染色
- **022 · 丹道**（9/12-9/13）：丹毒机制（同种丹 buff 剩余>50% 再服药效 6 折；60s 窗口内同种
  ≥3 次积 buff_dan_du——攻防-10% 120s，丹毒期同种丹再减半且刷新）；炼丹失败机制（黄/玄 100%、
  地品 80%、天品 70%，recipes.json success_rate 固定种子判定，失败出丹渣+毁料一半+喂练气+1）；
  BuffSystem 真接 buffs.json（ensure_defs_loaded 实现，修复旃檀佛光等纯 JSON 条目静默失效；
  elem_all=全元素抗性语义）
- **022 · 秘境与修复**（9/13）：大雷音寺遗址秘境（南赡部洲 x=2450，心猿石像 Boss realm7
  HP4950 必掉旃檀功德香）+ 龙宫藏珍阁二层（龙宫内镇守将身后 x=438，龙王三太子 Boss realm8
  HP5500 必掉定海珠·水抗25%）；机缘突破战死重试卡死修复（_enemies_alive/_waves_left/
  _wave_check_pending 在 _start_event 与 _finish 双路归零）+ 预存断言清理（test_world 读
  json 算期望 / breakthrough 卡死护栏）
- **023 · 写死表清零**（9/24）：五表外抽，全走「FileAccess 直读+硬编码兜底」（词缀/阵点先例
  同模式）——法宝 data/artifacts.json（7 条 JSON 权威源）/ 商店货架 data/shops.json（顶层
  shops 字典预留多店、默认店 chang_an，价格仍走 items.json）/ 连招表 data/combos.json
  （4 条定向 prev→next 倍率/窗口/话术，skills.json combo_* 字段退役死数据）+ 寿元表并入
  realms.json 每境 lifespan 字段（SoulLedgerSystem 直读+TABLE 兜底，天尊 ∞ 判定留码）/
  境界授予映射 data/grants.json（炼气→真仙 8 境条目表驱动，player.cpp 原 if 链逐字兜底、
  机制行为留码）；data-externalization 状态总表同步成文

### 8/28-8/29 提交

- **境界参数外抽**（8/28）：data/realms.json（caps/stats/期数/灵力基底/混元/回复率）+
  CultivationSystem::ensure_defs_loaded（DataLoader 优先+兜底）；data-externalization P0-P2 全 ✅
- **云游图 v2**（8/29）：ESC 云游页升级可视化世界地图（地理方位岛体/云海航线/详情栏/←→ 导航）
- **城镇安全区 + NPC**（8/29）：SafeZone（区内敌人不索敌 + 玩家缓速休整，世界层守卫）+
  TownNpc（对话气泡/客栈歇息全恢复）+ WC.create_town 五洲落地（落霞村/避火庄/长安坊市/苦寒驿/
  天庭街市）；城镇落位硬约束 x>700（房间挂原点内容带重叠）
- **云游阵**（8/29）：快速传送网络——各地阵碑走近自动铭刻（GameManager flag 随档），
  X 开驾云面板选阵点：同洲落点直达，跨洲 travel_to_direct_to 沿用境界门控；
  data/teleports.json 10 阵点 + WC.setup 自动装配；test_teleports.gd 28 断言

### 8/17-8/23 提交（session 022+ 未成文，git log 为准）

- **W4 系列**（4 agent）：机缘事件接 events.json + 道心不稳 debuff；洞天灵兽闯阵 + 采集点×4；
  技能连招派生（combo_after/window/mult 数据驱动）；法宝温养来源扩充（精英/Boss/服丹/打坐四路 SignalBus）
- **W5 系列**（5 agent）：大品天仙诀获得线（斜月三星洞·菩提传法，金仙门槛）；心魔镜像用玩家
  已装配技能模拟施法；北俱芦洲·寒墨旧魔宫（墨鲛/寒渊龟/寒渊君 + 玄冥归元丹）；法宝温养进度
  可视化（get_nurture_progress）；秘境压制修为（Portal 房间属性压制到指定境界）
- **UI 大修**（8/21）：HUD 状态栏/技能栏/消耗品栏多轮重排定型（底部居中堆叠 + QWERTY 紧凑槽位）、
  拾取提示多条目滚动、掉落『获得 XXX』提示、修为经验圆形水位注入、图鉴页（Bestiary 三类 +
  存档持久化）、熔炼炉页（炼丹/装备铸造/法宝铸造/装备强化 4 子页）、打坐疗伤
- **数值终轮**（8/22）：线性成长 + 比例减伤 + 敌人随境界缩放全面重平衡；仙人抚顶/醍醐灌顶
  双机缘叙事（兜率宫太上老君/流沙河古佛化身）
- **Wave4**（8/23）：敌人行为组件化——EnemyBehavior 聚合结构（ranged/flying/boss + slow/heavy/
  summon）+ 命名策略函数取代 ~30 处硬编码分支 + JSON flags 扩展；is_ranged/is_flying/is_boss
  兼容保留

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

> 2026-08-28 状态刷新：1~8 主体全部落地（勾除原因见各项）。

1. ~~**机缘突破事件**~~（session 009 v1 + 019 渡劫 v2 三灾齐至/天罚使 + 元婴双过法分叉 +
   W4-1 事件数据化 events.json/道心不稳 debuff + W5-2 心魔镜像玩家招式 + 8/22 仙人抚顶/
   醍醐灌顶双机缘）——后续可做：~~天罚使换正式 Boss~~（session 022 已落地：tian_fa_shi 入
   enemies.json 三阶段 Boss，见上方概要块）
2. ~~**洞天系统**~~（v1-v4 全落地 + W4-2 灵兽闯阵/采集点×4 + session 014 设施补全）——
   后续可做：洞天时间流速（design/dongtian.md 待敲定 Q4，成本高需拍板）；~~药童/傀儡 NPC、灵兽栏~~
   （session 022 已落地：洞天 v5 药童+灵兽栏 / v6 傀儡 NPC）
3. ~~**技能/法术系统**~~（session 010 管线 + W4-3 连招派生 combo_after/window/mult）
4. ~~**法宝系统完善**~~（session 011 v1 + 014 v2 + W4-4 温养来源扩充 + W5-4 温养进度可视化）——
   后续可做：法宝图标（依赖美术管线，design/art-assets.md）
5. ~~**食物/辟谷**~~（session 007 v1）
6. ~~**地府/南赡/西牛/北俱/天界/龙宫**~~（session 006-013 + 017 飞升结局全落地）
7. ~~**数值平衡**~~（session 011 + 013 遗留 + 8/22 线性成长/比例减伤/敌境缩放终轮）
8. ~~**工程清理**~~（session 006 完成；2026-08-28 补：境界参数外抽 data/realms.json——
   REALM_CAPS/REALM_STATS/期数/灵力基底/混元/回复率全表数据驱动，见 design/data-externalization.md）

### 剩余可做（无阻塞，按价值排序）

- **美术素材管线**（design/art-assets.md 仅规划未实现——全游戏程序化绘制，接入外部贴图是大升级，
  需要素材生成/审图工具链配合）
- ~~**云游图 UI**~~（2026-08-29 已落地：GameMenu 云游页升级可视化世界地图 v2——地理方位岛体/
  云海航线/详情栏，见上方提交块与 CLAUDE.md「GameMenu 云游页」条）
- ~~**键位配置 UI**~~（session 022 已落地：设置页「键位」子页 32 项改绑/冲突拒绑/keybinds.cfg
  持久化/恢复默认，见 data-externalization §15）
- ~~**能力解锁表外抽**~~（session 022 已落地：data/abilities.json 22 条——15 主动+7 被动，
  见 data-externalization §12.1）

## 三、需要抽成 OOP 的候选

| 现状 | 抽取方向 | 优先级 |
|---|---|---|
| ~~物品硬编码~~ | ~~Item 迁移数据驱动~~（items.json，session 002-006 落地） | ~~高~~✅ |
| ~~掉落表硬编码~~ | ~~DropTable 数据驱动~~（drops.json v2 + min_realm + Boss 专属，session 020） | ~~高~~✅ |
| ~~敌人 bool 标志~~ | ~~行为组件~~（Wave4：EnemyBehavior 聚合结构 + 命名策略 + JSON flags 扩展；
   敌人定义本身 session 020 已 enemies.json 数据化） | ~~高~~✅ |
| ~~技能不存在~~ | ~~SkillSystem 管线~~（session 010 主动13/被动6 + W4-3 连招派生） | ~~高~~✅ |
| ~~法宝 = Player 字段~~ | ~~ArtifactSystem~~（session 011 v1 + 014 v2 + W4/W5 温养两轮） | ~~中~~✅ |
| bootstrap/洲脚本手写场景搭建 | WorldCommon + 各洲/房间脚本已分工清晰；剩坐标级「场景数据外抽」
  （洲布局 JSON），收益中等、破坏面大（全部秘境/洲测试依赖现脚本），暂缓 | 低 |
| ~~机缘事件不存在~~ | ~~BreakthroughManager~~（session 009；W4-1 接 events.json） | ~~中~~✅ |
| ~~交互提示两处重复~~ | portal hint（world_common）与 GameHUD _interact_label 已各自收敛用法：
  portal 提示走 hint Label，交互类提示走 SignalBus interaction_prompt → GameHUD——
  若要彻底归一需 portal 也发 SignalBus 信号，收益小 | 低 |
| ~~食物/buff 不存在~~ | ~~BuffSystem~~（buffs.json 数据驱动） | ~~低~~✅ |
