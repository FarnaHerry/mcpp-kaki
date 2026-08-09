# Session 013 — 多 agent 并行（天界/分身/平衡/洞天v4/龙宫）

**日期**: 2026-08-09
**分支**: main
**起始提交**: c98cedf（灵石四阶余额显示）
**结束提交**: （本会话提交）

## 缘起

用户：「继续开发」「多 agent 分开完成以上人物（天界/分身/平衡遗留/洞天v4），
注意 k3 的并发限制，超过了一个一个减少，兵法测试限制最大值，按照最大值跑」，
并追加第 5 项「龙宫秘境（推荐）」。

## 编排过程

| 任务 | agent | worktree 分支 | 结果 |
|---|---|---|---|
| 天界·南天门之后 | A | worktree-agent-a024fd226fd249054 | 合并 ✓ |
| 身外化分身实体 | B | worktree-agent-abe4153cdfd3568e1 | 合并 ✓ |
| 数值平衡遗留 | C | worktree-agent-a1306fdf0a25f39ed | **quota 中途死亡** → 主 agent 抢救 |
| 洞天 v4 扩张经营 | D | worktree-agent-abb5c1ff1736256e6 | **k3 不支持报错** → 未生成，主 agent 亲做 |
| 龙宫秘境 | E | — | **k3 不支持报错** → 未生成，主 agent 亲做 |

- **模型切换事件**：session 进行中 kimi k3 配额耗尽，`/model` 切到 deepseek-v4-flash
  （只对**新会话**生效）；k3 已无法再 spawn 新 subagent（400 model not supported）。
  Agent C 途中因 403 quota 死亡留下未提交改动——主 agent 检查其 worktree diff
  （改动小，68 行）、在其 worktree 里跑通测试（ALL PASS）后代为提交 764ba5d。
- **工作树注意**：新建 worktree 缺 `.godot/extension_list.cfg`（gitignored）→
  GDExtension 静默不加载；需从主 checkout 拷贝。批量回归中边构建边跑测试会
  读到被撕裂的 .so → 测试无输出（"?" 判定），要串行构建。
- **merge 顺序** C→B→D→A 无冲突；merge 前 `git checkout mcpp.lock` 清掉脏文件。

## 各任务内容

### 天界 v1（agent A，commit ebdb000）
- `scenes/continents/tianjie.tscn` + `scripts/continents/tianjie.gd`：
  **南天门外**（云海石阶 + 天兵×2 + 增长天将精英远程）→ **天庭街市/凌霄殿外**
  （琼楼高台 + 天将×2 + 飞檐隐藏秘藏上品灵石×2）→ **兜率宫**（老君丹炉 + 遗丹玄龙丹）+
  **蟠桃园**（桃树 + 蟠桃拾取×2）→ **巨灵神** Boss（realm 11 金仙级 HP4000 守关，
  Boss 身后蟠桃+极品灵石赏格）。检查点 200/1700/2750；世界尽头墙 3500。
- `scripts/gates/tianjie_gate.gd`：北俱芦洲上古荒原「南天门序章」地标处 ↑ 触发，
  realm<10 拒行「天威浩荡，真仙方可登天」；realm≥10 `travel_to_direct("tianjie")`
  （真仙腾云直达，不渡云海——云海是金丹门控的凡俗强渡）。SceneGate 交互模板。
- 新物品 `pan_tao`；data/continents.json 注册 tianjie（真仙门槛）。
- 主 agent 修复：ContinentManager 原为占位硬编码，agent A 的测试因 JSON 未接线卡住
  → 主 agent 补 JSON 接线（commit 83a8165，见下）。

### 身外化分身实体（agent B，commit 583f41a）
- `src/nodes/clone_avatar.*`（268 行）：**CloneAvatar** CharacterBody2D——
  金色半透明人形，`setup_from_player` 属性快照（HP×50% / 攻×60% / 速×80%），
  索敌 enemies 组 300px → 追击 → 贴身近战（HitBox layer5/mask4，monitoring 重扫），
  无目标跟随玩家 40px 偏移；HurtBox layer3/mask6 可被击杀；
  击杀修为经 `gain_spiritual_energy` 转发本体（p_source 唯一入口）；30s 到寿消散。
- Player `_summon_clone`：`exec_skill_self_buff` 收到 `buff_shen_wai` 时
  buff（攻+35% 30s）之外再召唤分身，**至多同时 2 个**——第 3 次施放顶掉最老分身。
  与玩家同层 add_child（换场景随父节点自然清理）。
- 身外化身从「占位 buff」升级为实体协同作战；仍非境界授予（水帘洞残卷习得）。

### 数值平衡遗留 5 项（agent C，commit 764ba5d）
1. **三灾元素结算**：雷灾/阴火/赑风改走 `take_damage_typed(DMG_ELEMENTAL, ELEM_LEI/HUO/FENG)`
   ——元素抗性可减免，物理防御不可（渡劫非堆防硬抗）。
2. **心魔劫镜像 realm**：`e->realm = cs->get_realm_index()` 与玩家同境，
   威压/灵压不可慑服劫数（否则镜像战形同虚设）。
3. **普攻走全乘区**：普攻 HitBox 从 `attack_damage × realm_mult × combo_mult`
   改为 `get_effective_attack() × combo_mult`——装备/功法/buff/被动/宗门/本命/境界全计入，与技能口径一致。
4. **AlchemySystem 接入 recipes.json**：配方访问统一走 `s_recipes`
   （DataLoader JSON 优先 + 硬编码兜底）；JSON 装入 `std::vector<Recipe>` 前
   **必须 reserve 容量**（vector 扩容会使已存 Recipe 的 c_str 指针悬垂）。
5. **Boss ×5 血量时序修复**：脚本 `add_child` 后才 `set("is_boss")`（_ready 已跑完）时
   原 ×5 不生效 → `set_is_boss` 改为实现体，在置位时 `_apply_boss_hp_scale()` 幂等补偿；
   之后脚本再显式 set max_health 的以显式值为准。

### 洞天 v4 扩张经营（agent D，commit 4cb0485）
- **扩张碑 ExpandMonument**（`scripts/spots/expand_monument.gd`）：X 灵石购买开辟灵田
  6 → 最多 12 块，价格递增（下品基准），走 CurrencySystem 四阶钱包；已满拒买。
- **阵眼 JlzEye**（`scripts/spots/jlz_eye.gd`）：X 升级聚灵阵 0→2 级，
  每级打坐倍率 +0.5（上品×5 / 上品×15）；`Player::get_dongtian_meditate_mult` 叠加。
- DongtianManager：`BASE_PLOTS 6 / MAX_PLOTS 12`、`get_expand_cost/expand_plot`、
  `get_jlz_upgrade_cost/upgrade_jlz/get_jlz_bonus`；存档 `data["dongtian"]` 段扩展。

### ContinentManager JSON 接线（主 agent，commit 83a8165）
- 占位 `ensure_loaded()` 换成真接线：遍历 `dl->get_all_continents()` → 转 Def，
  按 min_realm 排序（云游页升序），空则退回 `CONTINENT_DEFS` 硬编码。
- const char* 生命期：`std::vector` 重分配会悬垂 SSO 缓冲 → 用
  `std::deque<std::string>` 静态池承接字符串。

### 东海龙宫秘境（主 agent，commit d53d952，agent E 计划落空）
- `scenes/rooms/longgong.tscn` + `scripts/rooms/longgong.gd`（Portal 房间模式，
  东胜神洲东海之滨 x=8600 入口 `[↑] 入东海龙宫`）：深蓝海底 + 光柱 + 珊瑚柱 +
  海底台 + 弱水走廊 NoFlyZone + 虾兵×2（HP200 realm5）/ 蟹将精英（HP450 realm6）/
  **镇守将** Boss（is_boss 后 max_health 800，realm7）；秘藏避水珠 + 千年珍珠 + 灵石。
- **装备元素抗性管线**：Item 加 `float elem_resist[8]`（JSON 解析 + 兜底）；
  `Player::_take_damage_typed` 汇总装备元素抗性进结算。避水珠（饰品槽，
  ELEM_SHUI 抗 20%）——水抗配弱水禁飞，过海底的配套生存装备。
- 新物品：**避水珠 bi_shui_zhu**（装备，水抗20%）/ **千年珍珠 qian_nian_zhen_zhu**
  （消耗，修为+2000 + 回灵，低能量自动服用）。

## 测试

- 新增 5 个测试：test_tianjie.gd（9 PASS）/ test_shenwai_clone.gd（15 PASS）/
  test_balance_fixes.gd（19 PASS）/ test_dongtian_v4.gd（20 PASS）/ test_longgong.gd（ALL PASS）。
- 回归修两处断言：test_continents / test_travel 洲数 4→5、未解锁 3→4（加天界）。
- **水抗实测漂移坑**：避水珠测试放房间内做精确断言会挂——房间有敌人受击时
  炼体功法层数上涨 → max_health 208→216 漂移 → 水抗数值失真；
  改在主场景测（数值稳定），房间内只做拾取/能量断言。
- 全量回归绿（46 旧 + 5 新）；double_jump 已知抖动复跑过；pressure/projectile
  批量中"?"为构建撕裂 .so 误报，单跑均通过。

## 关键坑

- **模型配额硬限制**：多 agent 编排中途配额耗尽后新 subagent 全部失败——
  老代码/测试/数据留在 worktree 未提交，主 agent 兜底。worktree 未合并分支
  需手动 `git worktree remove --force` 清理。
- **set_is_boss 时序**：`.tscn` 直摆 is_boss 在 _ready 前生效（_ready ×5）；
  脚本 add_child 后再 set 走 set_is_boss 补偿——两路靠 `_boss_hp_scaled` 幂等。
- **JSON 字符串池 reserve**：vector 扩容悬垂 c_str 是三处（alchemy/continents/
  skills）共有的坑，统一 reserve + deque 承接。
