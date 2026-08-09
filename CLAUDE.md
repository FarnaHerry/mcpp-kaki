# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

修仙题材的 2D 动作游戏，类银河恶魔城（Metroidvania）风格。Godot 4.6 项目，所有游戏逻辑用 C++ GDExtension 编写，不使用 GDScript。

## Game Design

- **类型**: 修仙 × ACT，银河恶魔城式关卡设计
- **核心特征**: 2D 横版动作战斗，互联地图，能力门控探索
- **世界观**: 修仙体系 —— 境界突破、功法修炼、法宝、丹药、灵气融入玩法
- **分辨率**: 480×270 内部分辨率，canvas_items stretch 到 1920×1080

## Key Project Settings

- **Engine**: Godot 4.6 (config_version=5)
- **Renderer**: gl_compatibility (2D)
- **Display**: 480×270 → 1920×1080, canvas_items stretch
- **Texture filter**: Nearest (pixel art)
- **Project name**: `mcpp-kaki`

## Build / Development

This project uses **mcpp** as its build system. CMake is no longer required.

```bash
# Build (mcpp fetches the godot-cpp dependency on first run + copies .so files to bin/)
./scripts/build_mcpp.sh

# Output: bin/libmcpp-kaki.linux.editor.x86_64.so
#         bin/libmcpp-kaki.linux.template_debug.x86_64.so

# Run game (requires display — headless mode does NOT load GDExtensions)
godot
```

### Manual build steps

If you prefer to run the steps separately:

```bash
# 1. Build the GDExtension shared library with mcpp
mcpp build

# 2. Copy the resulting library to bin/ where Godot expects it
cp target/x86_64-linux-gnu/*/bin/libmcpp-kaki.so \
   bin/libmcpp-kaki.linux.template_debug.x86_64.so
```

### Build requirements

- **mcpp** must be installed and have a toolchain available (e.g. `mcpp toolchain install llvm`).
- godot-cpp comes from mcpp packages: **`compat:godot-cpp@10.0.0-rc1`** (Godot 4.6 pre-generated bindings — no Python/SCons/submodule; header include path for plain .cpp) and **`godotengine:godot-cpp-m@10.0.0-rc1`** (C++23 module layer: the 6 `.cppm` interfaces use `import godot_cpp;` + `#include <godot-cpp-m/macros.h>` for GDCLASS etc.; HashMap/HashSet are NOT re-exported by the module — keep textual `#include <godot_cpp/templates/hash_*.hpp>`). Builds once into mcpp's global cache; clean rebuilds reuse it.
- The default target is `template_debug` for development, matching Godot 4.6.
- mcpp build artifacts live in `target/` and are ignored by git.

GDExtension is registered via `mcpp_kaki.gdextension` at project root (entry symbol: `mcpp_kaki_library_init`) and explicitly referenced in `project.godot` via `[native_extensions] paths=["res://mcpp_kaki.gdextension"]`.

**Important**: C++ classes cannot be placed directly in `.tscn` files due to GDExtension registration timing. Use `scripts/bootstrap.gd` (`call_deferred` + `ClassDB.instantiate`) to create C++ nodes at runtime. All game logic remains in C++ — the bootstrap only handles node creation.

## Architecture

### Design Principles

- **组合优于继承**: Godot Node 组合 + Component 模式
- **数据驱动**: 角色属性、敌人、物品用 Godot `.tres` (Resource) 定义，C++ 只读
- **信号解耦**: 系统间通过全局 `SignalBus` autoload 通信

### Directory Structure

```
src/              # C++ 源码 (GDExtension)
  register_types.cpp/h  # 入口，注册所有 class
  core/            # 纯 C++ 系统 (CultivationSystem, AbilityManager, etc.)
  nodes/           # Godot Node 派生类 (Player, Enemy, CameraRoom2D, etc.)
  combat/          # 战斗逻辑 (HitBox, HurtBox, CombatSystem)
  cultivation/     # 修仙数据定义
  inventory/       # 物品/背包数据
  utils/           # 工具类 (StateMachine, InputBuffer, SignalBus)
  resources/       # Godot Resource 绑定
scenes/           # Godot .tscn 场景文件
resources/        # .tres 数据定义 (items, enemies, abilities)
assets/           # 美术/音频/字体
bin/              # 编译产物 (.so)，gitignored
```

### Currently Implemented

- **`StateMachine<Owner>`** (`src/utils/state_machine.h`) — Generic FSM template, used by Player and Enemy
- **`InputBuffer`** (`src/utils/input_buffer.h`) — Input buffering for responsive platforming
- **`TXT()`** (`src/utils/text.h`) — 统一文本编码包装（所有字符串字面量必须用它，禁直接 `String("...")`，否则中文乱码）
- **`Player`** (`src/nodes/player.h/cpp`) — 8 states: Idle, Run, Jump, Fall, WallCling, Dash, Attack, Fly
  - Variable jump height, coyote time, jump buffering, wall slide/jump, air dash
  - 飞行：空中再按跳进入，方向键全向渐加速，速度随境界；筑基需飞剑+耗灵力（10/s），金丹+无条件；攻击/冲刺后自动恢复飞行（was_flying）
  - HitBox (layer 5) + HurtBox (monitors layer 6) for combat
  - 本命法宝：温养 120%→150% → 渡劫觉醒 200%，飞升后锁定
- **`Enemy`** (`src/nodes/enemy.h/cpp`) — 6 states: Idle, Patrol, Chase, Attack, Hurt, Death
  - 种类标志：近战/远程（Projectile）/飞行/Boss（多阶段）；**Boss ×5 血量幂等补偿**：`.tscn` 直摆 is_boss 在 _ready 前生效，脚本 add_child 后再 `set("is_boss")` 的由 `set_is_boss` 实现体 `_apply_boss_hp_scale()` 补偿（`_boss_hp_scaled` 幂等；之后脚本再显式 set max_health 以显式值为准）
- **`Portal`** (`src/nodes/portal.h/cpp`) — Self-contained room transition (composition over inheritance)；门口交互用 **↑（上键）**进入/离开房间，提示 `[↑] 进入/离开`，X 已从门口交互移除（Portal `_process` 只轮询 `up`）
- **`CameraRoom2D`** (`src/nodes/camera_room_2d.h/cpp`) — 跟随增益随距离缩放（高速飞行不落后）+ room lock
- **`CultivationSystem`** (`src/cultivation/`) — 13 境界、int64 累计修为经验（9系门槛，**到顶封顶卡境界**）、期数、四轴（门派/五仙/出身/果位）、混元一气、TitleComposer 称号、灵力法力池（与修为分离，自动回复）、生命/攻防速随境界、突破调试开关。**数值平衡（session 011）**：修为门槛金丹起 ~3.3×/境（非×10，打怪路线可持续）、击杀修为随境界 `15×(1+realm)`、顶端金仙/天尊攻防平滑（42/38→32/30，100/100→60/50）
- **`AbilityManager`** (`src/cultivation/`) — 境界门控能力（纳戒/飞行/云游等）
- **`BreakthroughManager`** (`src/cultivation/`) — 机缘突破唯一入口：Q → SignalBus `breakthrough_requested` → 叙事事件×6 / 心魔劫·三尸劫（战斗秘境，属性随玩家缩放，**镜像 realm=玩家同境**——威压/灵压不可慑服劫数，Enemy `no_drops`）/ 三灾连考；秘境复用 Portal 模式（场景加载+玩家重挂载+相机锁定）；失败境界不变、经验保持封顶可重试
- **`TribulationController`** (`src/cultivation/`) — 三灾：雷灾（预警落雷走位）→ 阴火（DoT 生存）→ 赑风（控制反转 `input_inverted`+罡风推移+风蚀）；**三灾伤害全走 `take_damage_typed(DMG_ELEMENTAL, ELEM_LEI/HUO/FENG)`**（元素抗性可减免，物理防御不可——渡劫非堆防硬抗）；渡劫过渡态 DU_JIE（失败退回大乘）
- **`Inventory` / `ItemDatabase` / `ItemPickup`** (`src/inventory/`, `src/nodes/`) — 24→999 纳戒扩容、装备三槽、掉落拾取
- **`DropSystem`** (`src/core/drop_system.h/cpp`) — 所有掉落物的唯一入口（掉落表+生成）
- **`SaveSystem` / `GameManager`** (`src/core/`) — ConfigFile 存档、检查点、重生、击杀统计
- **`GameHUD` / `TelemetryPanel` / `InventoryPanel`** (`src/nodes/`) — UI 三类分立：游戏 HUD（生命/灵力/修为%条，F4）/ 遥测（F3/F5）/ 背包（I）。**背包类型筛选**：物品标题行右侧筛选行（[全部] 消耗品 材料 装备 关键物品，[活动项] 括起），网格顶行再按 ↑ 进筛选行，←/→ 循环切类型（网格按类型过滤，`_filter_matches`），↓/X 返回网格筛选保持；重开背包回「全部」；独立打开（I）与 GameMenu 背包页共用 `ext_navigate/ext_navigate_h/ext_use`。**选中项说明行**：网格下方 ItemDesc 单行显示物品 desc（效果数值/来源/可种时长，全部 31 物品均有说明；网格 6 行窗口）。**灵石余额**：网格下方右侧 CurrencyLabel 四阶显示（下/中/上/极，`currency_changed` 实时刷新）
- **`GridList`** (`src/nodes/grid_list.*`, nodes.cppm 导出 + GDREGISTER) — 统一格子列表组件：数据 = `Array of Dictionary {text, color?, dim?}`，host 驱动交互（`move_selection/get_selected`），cell 池（frame+bg+Label 11px），行数按 size.y 推导尺寸变化自动重建；`set_active(false)`=只读无选中高亮（被动/锁定项灰显）。**2D 网格导航** `move_selection(dx,dy)`：横向（dx）行内钳制不跨行回卷、末行不满退行末；纵向（dy）±列数。已接入背包物品、仓库双栏、技能页主动/被动、能力页、法宝页、炼丹页——物品/技能/法宝/丹方统一格子呈现（暂无图标用名字）
- **DamageNumbers** (`src/nodes/`) — 伤害数字唯一入口：SignalBus `damage_dealt(pos,amount,is_player_victim)` → 世界坐标上浮淡出（敌=金/玩家=红）
- **DamageCalculator** (`src/combat/damage_calculator.h`, header-only) — 伤害统一结算：物理(防御减免,min1)/法术(抗性比例,cap0.9)/元素(五行抗性,克制×1.25只增伤)；`DamageInfo`+`DefenseProfile`；HitBox/Projectile 携带 `damage_category`+`element`，投射物经 `take_damage_typed` 入口
- **GongfaSystem** (`src/cultivation/gongfa_system.*`) — 功法：炼体/练气双槽(1+1)，黄/玄/地品(3/5/7层)，行为喂养主系100%/副系20%(受击/近战击杀养炼体，耗灵养练气)，层数乘区(1+每层×层数)，切换保留熟练(_known)，存档 pd["gongfa"]
- **SkillSystem** (`src/combat/skill_system.*`) — 武技/法术/神通/仙法/被动统一 Skill 管线：8槽(A/S武技 D/F法术 G/H战斗页闲置 T神通 Y仙法)，武技=物理+冷却驱动(凡人起步破空斩/突进斩)，法术=元素伤害+耗灵+冷却(炼气授予火弹/冰锥)，神通=耗法则之力(化神授予缩地成寸,FX_BLINK碰撞安全瞬移)；存档 pd["skills"]
  - 主动 13 个：武技×4(+旋风斩AOE/升龙击上跃，炼气) 法术×5(+雷咒雷元素/土盾自buff，筑基；御剑术3发剑扇，金丹) 神通×3(+金刚不坏2.5s无敌/三昧真火，化神) 仙法×1(天雷引，真仙)；FX 9 种(MELEE_SWING/LUNGE/PROJECTILE/BLINK/AOE_SWING/RISING/SELF_BUFF/PROJ_FAN/INVULN)
  - 被动 TYPE_PASSIVE×6（学会即常驻不占槽，乘区添头）：神行百变(炼气,速12%)/剑心通明(筑基,攻10%)/铁布衫(金丹,防15%)/灵台清明(金丹,回灵25%)/风雷双翼(元婴,飞速15%)/道法自然(化神,法则回复25%)；挂钩 攻→get_effective_attack / 防→take_damage / 速→_update_move_speed / 飞速→FlyState / 回灵→mana_regen_mult(功法×被动) / 法则→_law_regen_mult
  - 雷元素 ELEM_LEI：不入五行克制环
- **五区地图** (scenes/main.tscn + scripts/bootstrap.gd) — 落霞村外围(0~1300)/青竹林(1300~2600 单向竹台跳跃)/断崖绝壁(2600~3900 墙跳烟囱)/幽谷(3900~5200 飞行沟壑3900~4400，谷底y=420)/谷深处(5200~6000 BOSS+悟道崖飞行高台y=90)；检查点×4(1320/2820/4450/5250)；新敌人 竹妖/崖枭/崖弓/谷枭/雷兽/幽谷螭龙(BOSS 300血)；草药全闭环(悟道茶×2/冰心莲/赤焰花/金刚藤摆点)
- **四大部洲** (design/world-map.md，后西游设定) — **ContinentManager** (`src/core/continent_manager.*`)：洲定义表（东胜神洲main.tscn/西牛贺洲金丹/南赡部洲炼虚/北俱芦洲渡劫），`get_continent_list/can_travel/travel_to`；洲=根场景整景切换，**旅行桥**（GameManager 函数局部 static 收集存档→新场景 GameManager 应用，全恢复+落点；**严禁文件级 static Dictionary**——引擎内存初始化前构造必 segfault）；跨洲读档走同一桥；**WorldCommon** (`scripts/world_common.gd`，preload 无 class_name——CLI 全局缓存不可靠） 承载公共装配，各洲脚本只搭地形内容；HUD 洲名横幅（continent_changed→2.8s 淡入淡出，仅踏上洲土播报）
- **云海强渡** (scenes/continents/yunhai.tscn) — `travel_to`=先渡云海（检查点改写起云台，`cp["travel_dest"]`记目的洲**随档持久化**，GameManager `_travel_dest` 成员随 checkpoint 段存取）；`travel_to_direct`=直达（调试）；登岸区→`complete_travel`（到岸清 dest）；云海机制：无地面云墩+罡风带（**位移直推**——速度增量被状态机每帧覆写）+落雷柱（预警1s→劈落0.25s）+坠海遣返（y>420 回起云台+15%代价）+雷鸟；过渡场景 `_ready` 保持上一洲身份不播报
- **东胜神洲补完** — 花果山(6000~8000：桃林粉台+猿怪+仙桃)+水帘洞秘境(scenes/rooms/shuilian_dong.tscn + scripts/rooms/，Portal 房间模式，白猿老祖精英，秘藏**身外化身残卷**)+东海之滨(8000~9000：巡海夜叉精英远程+**定海神针铁**武器攻+25 地品)；世界尽头墙移至 9000；检查点 6200/8200；**东海龙宫秘境** (scenes/rooms/longgong.tscn + scripts/rooms/longgong.gd，x=8600 入口 `[↑] 入东海龙宫`，Portal 房间模式)：深蓝海底+光柱+珊瑚柱+海底台，**弱水走廊 NoFlyZone 禁飞**，虾兵×2(HP200 realm5)/蟹将精英(HP450 realm6)/**镇守将** Boss(is_boss 后 max_health 800 realm7)；秘藏 **避水珠 bi_shui_zhu**(饰品槽 **水抗20%**——配套弱水禁飞的海底生存装)/**千年珍珠 qian_nian_zhen_zhu**(修为+2000+回灵，低能量自动服用)/灵石
- **装备元素抗性管线** — Item 加 `float elem_resist[8]`（JSON `elem_resist` 数组 + fallback 兜底）；`Player::_take_damage_typed` 汇总**装备**元素抗性进结算（与 buff/被动/技能抗性同乘区）。避水珠：type 3 饰品槽 equip_slot 2，ELEM_SHUI 0.2
- **秘籍物品管线** — Item 新字段 `learn_skill`：use_consumable 统一入口习得技能（数据驱动，残卷/秘籍通用）；新物品：仙桃(50%回血+300修为)/身外化身残卷(地品)/定海神针铁
- **身外化身·分身实体** (`src/nodes/clone_avatar.*` + Player `_summon_clone`) — TYPE_SHENTONG，FX_SELF_BUFF→`buff_shen_wai`(30s 攻+35%) **之外再召唤 CloneAvatar 实体协同作战 30s**：属性快照（HP×50%/攻×60%/速×80%），索敌 enemies 组 300px→贴身近战（HitBox layer5/mask4 重扫），无目标跟随玩家，可被击杀（HurtBox layer3/mask6），击杀修为经 `gain_spiritual_energy` 转发本体，**至多同时 2 个**（第 3 次施放顶掉最老）；化神可施(法则50/冷却60s)，**非境界授予**，水帘洞残卷习得
- **GameMenu 云游页** — 第7页「云游」（背包/能力/功法/技能/法宝/宗门/**云游**/炼丹/设置）：四洲列表（名/描述/【当前】/未解锁+条件话术），↑/↓选洲 X前往（先关菜单还原暂停再 travel_to）；ContinentManager 在 _open_menu 惰性查找（WorldCommon 创建顺序在 GameMenu 之后）
- **ArtifactSystem** (`src/cultivation/artifact_system.*`) — 法宝：槽0=本命(镜像Player本命,飞升后锁定)+次要×2(飞升后+3)，威力系数本命1.2~2.0/次要1.0→1.2→1.5(两段温养)，攻击型祭出复用Skill效果管线(耗灵+冷却)，辅助型常驻被动入防御乘区；示例：飞剑(筑基)/照妖葫(金丹)/玄铁塔(元婴)；存档 pd["artifacts"]
- **B键切页**: `Player._skill_page` + `skill_page_changed` 信号；法宝页 A~H=法宝槽0..5(T/Y两页通用)；页机制通用可扩技能多页
- **法则之力** (CultivationSystem) — 化神解锁独立能量条(max100,回复3/s+击杀+10)，神通唯一消耗源；HUD右上角紫条；存档 cd["law_power"]
- **SectSystem** (`src/cultivation/sect_system.*`) — 宗门：四宗一散（蜀山攻/昆仑灵力回灵/蓬莱生命防御/魔罗击杀修为+攻），职位 外门(0)/内门(100)/真传(300) 加成随档升；贡献=击杀+1/Boss+10；炼气门槛拜师、叛门贡献清零、已学专属技保留；乘区全走既有组合点（攻/生命/灵力上限+回灵/防御/击杀修为）；专属技×4（万剑归宗金系剑扇/太清神光水系法术/玄龟护体自buff防25%/血影斩突进）；存档 pd["sect"]
- **威压/灵压** (`src/nodes/player.*` + `enemy.*`) — 威压 V：耗灵30/cd8s/r240px，realm<玩家→慑服(suppress:定身+灰显 2+0.5×gap s cap5)；灵压 R：耗灵60/cd15s/r200px，realm≤玩家-2→法伤 atk×(2+0.5×gap)，gap≥4→镇杀99999；护佑：敌方高阶(realm≥玩家)在场→300px低阶全免+反弹5%(V)/8%(R)生命；Enemy新增 realm 字段+suppress(t)+enemies group；bootstrap 全敌 realm 标注（0小怪→4螭龙→8北俱）
- **地府/生死簿/勾魂** (`design/cultivation-realms.md` §五) — **SoulLedgerSystem** (`src/core/soul_ledger_system.*`) 独立生死簿：簿上寿元=物种默认100 ≠ 实际寿元(随境界 凡人100/炼气150/筑基250/金丹500/元婴2000/化神5000/炼虚8000/合体12000/大乘20000/渡劫50000/真仙100000/金仙200000，**天尊（三清级）跳出五行寿元无限 ∞**——HUD 显「寿 簿上/∞」金，且天尊不再被勾魂)，信息差=勾魂错抓；出身/原身/**划名标记**：`_soul_protection`(免死一次)+`_struck`(永久阴寿豁免——划名后不再被勾魂，勾魂错抓的终点)，存档 pd["soul_ledger"]；HUD 寿元小标签「寿 簿上/实际」（实际>簿上绿）+ SignalBus `lifespan_changed/soul_protection_changed/ledger_inspect_requested`。**勾魂使**：濒死(HP<20%)刷黑/白无常（Enemy `is_soul_reaper`+no_drops，realm≥玩家），反杀+30修为、被击杀→魂魄入地府。**死亡三分支** (`GameManager::on_player_died`)：免死(划名→原地满血复活，最高优先)/勾魂使击杀→入地府/正常→回检查点；地府内死亡→还阳。**地府场景** `scenes/continents/difu.tscn`（全场景切换，`enter_difu/huan_yang` 走 change_scene_to_file+旅行桥，**不走 request_scene_change** 防污染 `_respawn_scene`）：判官(mode=查簿, HUD overlay 显出身/原身/寿元/划名状态)+**秦广王(mode=审判，一殿初审核对生死簿叙事)**+生死簿(mode=改簿划名→免死+阴寿豁免)+还阳出口；入口在南赡部洲长安 `SceneGate`(scripts/gates/, ↑ 触发 gm_method)；互斥：地府内不再刷无常、子空间/机缘中不刷
- **灵石货币（四阶通用钱包）** (`src/core/currency_system.*`) — 灵石独立成钱包不占背包：下品/中品/上品/极品 四档，每档 ×10 价值（1极=10上=100中=1000下），价格一律下品基准；`add/spend(自动破零+找零高档回填)/can_afford/get_total/exchange(from,qty,to 保值兑换)`；存档 data["currency"]+老档迁移（inventory spirit_stone→钱包下品）；拾取路由 Item 新字段 `currency_tier`（-1普通/0..3档位，拾取直入钱包）；SignalBus `currency_changed`；物品 `spirit_stone`(下品)/`spirit_stone_mid/high/peak`
- **商店系统** (`src/core/shop_system.*` + `src/nodes/shop_panel.*`/`shop_keeper.*`) — 长安坊市灵石买卖：Item 新字段 `buy_price/sell_price`(0=不可买卖，JSON+fallback)；**ShopSystem** buy(扣钱包+入库,不足拒)/sell(扣物品+回钱包下品,货币/关键物不可卖)/get_stock(硬编码货架)；**ShopPanel**(CanvasLayer 116，三栏 GridList 商店货架|玩家背包|灵石兑换，Q/E 切栏循环，X 购买/卖出/兑换，顶部四阶余额，打开暂停)；**ShopKeeper**(Area2D, StorageChest 模板, "[X] 交易")；人参果(五庄观镇观灵果, 80%回血+800修为+饱食+攻防15% 900s)
- **南赡部洲** (`scenes/continents/nanzhanbu.tscn` + `scripts/continents/nanzhanbu.gd`，炼虚门槛走云海) — 长安坊市(商店掌柜)+五庄观(人参果/灵石拾取)+**地府入口正式版**(长安城内 DifuGate，花果山旧入口已移除)
- **西牛贺洲** (`scenes/continents/xiniuhe.tscn` + `scripts/continents/xiniuhe.gd`，金丹门槛) — 三段：**火焰山**(赤岩台地 + **环境火伤** FireZone 岩浆池 + 火鸦/火牛 + **芭蕉扇**铁扇公主遗物)、**灵台方寸山**(高台 + **斜月三星洞秘境** rooms/xieyue_sanxing_dong 菩提道统)、**流沙河**(**弱水 NoFlyZone** 禁飞须跳石墩 + 沙怪)。机制：**FireZone** (`scripts/zones/fire_zone.gd`，Area2D dot 扣血，`extinguish()` 可灭，组 `fire_zones`)；**NoFlyZone** (`scripts/zones/no_fly_zone.gd`，置 Player `_flight_blocked` → `can_fly()` false + FlyState 进入即坠)；**芭蕉扇**(法宝 FX_PROJ_FAN 风刃，祭出时扇灭环境火开道，Item `learn_artifact` 字段=使用得法宝)；**菩提心法**(新被动 PAS_ELEM_RESIST 全元素抗性+10%，挂 Player 元素抗性计算，三星洞残卷习得)
- **北俱芦洲** (`scenes/continents/beijulu.tscn` + `scripts/continents/beijulu.gd`，渡劫门槛 realm 9，最后一洲) — 三段：**极北冰原**(**冰面打滑** IceZone：Player `_slippery` → Idle 摩擦骤减 + Run 渐进加速，惯性滑冰)、**玄冰高原**(**极寒 ColdZone**：Player `_chilled` 减速30% + 冰伤 dot，DMG_ELEMENTAL+ELEM_SHUI 冰心丹可减免；**玄冰窟秘境** rooms/xuanbing_ku 上古巨兽巢穴遗迹，龙骨/玄冰参秘藏)、**上古荒原**(**上古巨兽·玄冥** Boss realm10 守关 → **炼体圣地** RefineSpot X 交互 炼体 buff → **南天门序章** 天界之门地标)。机制：**IceZone** (`scripts/zones/ice_zone.gd`，Area2D 置 `slippery`)；**ColdZone** (`scripts/zones/cold_zone.gd`，置 `chilled` + dot)；**RefineSpot** (`scripts/spots/refine_spot.gd`，StorageChest 模板，X 炼体)。新物品：龙骨 long_gu(材料)/玄冰参 xuan_bing_shen(草药可种)/玄龙丹 xuan_long_dan(渡劫丹方：攻防+20% 900s)；BuffSystem 加 buff_lianti(防20% 600s)/buff_xuan_long(攻防20% 900s)
- **天界** (`scenes/continents/tianjie.tscn` + `scripts/continents/tianjie.gd` + `scripts/gates/tianjie_gate.gd`，真仙门槛 realm 10，**南天门登天**) — 北俱芦洲上古荒原「南天门序章」地标处 ↑ 触发 `travel_to_direct("tianjie")`（真仙腾云直达，**不渡云海**——云海是金丹门控的凡俗强渡；realm<10 拒行「天威浩荡，真仙方可登天」，SceneGate 交互模板）。三段：**南天门外**(云海石阶 + 天兵×2 + 增长天将精英远程)、**天庭街市/凌霄殿外**(琼楼高台 + 天将×2 + 飞檐隐藏秘藏上品灵石×2)、**兜率宫+蟠桃园**(老君丹炉 + 遗丹玄龙丹 + 桃树/蟠桃拾取×2 + **巨灵神** Boss realm11 金仙级 HP4000 守关，Boss 身后蟠桃+极品灵石赏格)。检查点 200/1700/2750；新物品 **pan_tao 蟠桃**；data/continents.json 注册（真仙门槛）
- **ContinentManager JSON 接线** (`src/core/continent_manager.cpp`) — 洲定义表已接 `data/continents.json`（JSON 优先 + `CONTINENT_DEFS` 硬编码兜底），云游页按 min_realm 升序；const char* 生命期用 `std::deque<std::string>` 静态池承接（vector 重分配会悬垂 SSO 缓冲）
- **GameMenu** (`src/nodes/game_menu.*`) — ESC 多页管理菜单（背包/能力/功法/技能/法宝/宗门/云游/炼丹/设置 共9页），托管 InventoryPanel（外部驱动 ext_navigate/ext_navigate_h/ext_use）；背包页=GridList 2D 导航（↑/↓ 行移 ←/→ 列移，顶行↑进类型筛选行）+ 筛选；能力页=主动/被动分区技能树总览（只读 v1）；技能页=主动装配（↑/↓←/→ 选已学主动，A/S/D/F/T/Y 装入对应槽，类型不符拒装提示）+被动分区（名+效果%）；宗门页=未入门四宗列表选宗拜入/已入门职位贡献加成总览+叛门；设置页含音量(持久化 user://settings.cfg)/保存/退出；嵌套暂停安全（还原原暂停状态）；页签条独立 CanvasLayer 130；**翻页严格只用 Q/E**（`_input` 原始键码，←/→ 不被顶部翻页拦截，留给页内横向导航/筛选）
- **HUD 底部技能栏**: 武技[A/S] 法术[D/F] 法宝[G/H]（技能系统已填充：显示装配技能名+冷却）
- **BuffSystem** (`src/cultivation/buff_system.*`) — 丹药/食物/状态统一 Buff：def 表（冰心水抗15%/赤焰攻15%/金刚防20%, 300s）、同名刷新不叠加、到期自消、攻/防/元素抗性乘区钩子、HUD buff 行（名+秒）、存档 pd["buffs"]
- **AlchemySystem** (`src/cultivation/alchemy_system.*`) — 炼丹：丹炉随身，7 固定配方（**已接 DataLoader recipes.json**——JSON 优先 + 硬编码兜底，装入 `std::vector<Recipe>` 前**必须 reserve** 防 c_str 悬垂；`get_recipe_count/get_recipe/find_recipe/get_recipe_list` 全走 `ensure_loaded` 后的 s_recipes），成功率字段 v1=100%（失败机制预留），地品金丹门控，每炉喂练气+5；GameMenu 第8页「炼丹」**卡片化**（3 列 GridList：锁定=灰、材料够=绿、不够=红），**2D 导航** ↑/↓ 行移 ←/→ 列移（行内钳制不跨行回卷），X 炼制；详情行（y=170）显示 丹方名+效果（锁定附「（金丹起）」）+材料行（y=184）
- **草药** — Item grade 字段（0凡/1灵/2地）；7 草药（MATERIAL）；**HerbNode** (`src/nodes/`) 采集点（[X] 采集入包+喂练气+2，枯萎，房间重进刷新）；小怪掉止血草/聚灵草，Boss 千年灵芝保底
- **use_consumable 统一入口** — Player::use_consumable(item_id)：扣数量+回血/比例回血/回灵(mana_amount)/修为(energy_amount)/buff；拾取自动用、背包面板、数字键栏全部走这里；聚气丹已迁移为回灵50
- **食物/辟谷** (`design/cultivation-realms.md` 饮食 L166-179) — **饱食度** `_fullness/_max_fullness`(Player)：凡人/炼气随时间衰减(0.3/s)，归零→`buff_hunger` 饥饿 debuff（攻防-20%，force-managed 吃食解除）；**食物倍率** 凡人1.0/炼气1.2；**筑基辟谷** `is_bigu()` 不再衰减+饱食度条隐藏+食物转纯 buff（不回饱食度）；HUD 饱食度条(y60，境界标签移至 y78/寿元 y96)；信号 `fullness_changed/bigu_changed`；存档 pd["fullness"]；食物物品 糙米饭(full15+果腹防5%)/干粮(full25+干粮攻5%)/灵米(full40+饱足攻防8% 900s，**洞天可种 180s**，联动灵田)；来源=起始干粮×3 + 地图拾取 + 小怪掉落(25%/15%)；test_food.gd
- **数字键消耗品栏** — 1~6 快捷栏（consume_1..6），拾取消耗品自动入栏（首个空位/耗尽槽），HUD 技能栏上方一行（名首字+数量），存档 pd["consumable_bar"]
- **调试键**: F3 遥测 / F4 HUD / F5 突破无经验门槛开关 / F6 读档 / Q 突破
- **潜伏 bug 教训**: `Player::_on_enemy_killed` 通过 `connect("enemy_killed", Callable(this, "_on_enemy_killed"))` 连接，但从未 `ClassDB::bind_method` → Callable 解析失败静默无效 → 击杀喂功法/法宝温养/法则击杀回复全部静默失效（自实现以来一直无效，2026-07-25 修复）
- **DataLoader** (`src/core/data_loader.*`) — 启动时加载 `data/*.json`（9 表）到 HashMap 缓存，各系统 `ensure_loaded()` 惰性初始化，优先 JSON 不可用时退回硬编码。已接入：items(22)/skills(24)/buffs(6)/gongfas(6)/sects(4)/drops/recipes(7)/continents(4)；events(10) 就绪待 BreakthroughManager 侧接入
- **DongtianManager** (`src/nodes/dongtian_manager.*`) — 洞天（design/dongtian.md）：炼虚解锁 `dongtian` 能力，安全状态按 **O** 进出随身小世界（scenes/rooms/dongtian.tscn，480×270 浮空灵地）；进出复用 Portal 模式（挂子场景+玩家重挂载+相机锁定），退出回到进入时位置；互斥检查：机缘事件（BreakthroughManager `is_active()`）/云海/Portal 房间（玩家父节点非主场景根）/战斗（enemies 组 chase/attack/boss_special），拒入经 interaction_prompt 给原因（2.5s 自消隐）；存档记返回点（洞天内坐标对外界无意义），读档 `force_exit_for_load()`；HUD 横幅复用洲名机制（SignalBus `dongtian_entered/exited`，退出时重播洲名）。**v2 灵田种植**：6 地块状态自持于 Manager（场景卸载不丢），现实时间生长（Unix 时间戳），空地 X 播种（背包品级最低可种草药）→ 生长倒计时 → 成熟 X 收获（种一收二+喂练气）；**FarmPlot** (`src/nodes/farm_plot.*`) 交互+视觉节点（scripts/rooms/dongtian.gd 运行时创建）；Item 新字段 `plantable`/`grow_seconds`（数据驱动，凡60s/灵180s/地600s）；存档 `data["dongtian"]` 段；`debug_age_plot` 测试拨快。**v2 仓库（储物箱）**：**StorageChest** (`src/nodes/storage_chest.*`) 交互节点（x=285 避开出生点下落走廊——x=260 时玩家右缘 248 蹭箱左缘 247，传送离开触发 body_exited 误清提示），贴近显 `[X] 打开仓库`；**StoragePanel** (`src/nodes/storage_panel.cpp`，CanvasLayer 115，nodes.cppm 导出) 双栏（背包|仓库）↑/↓ 选 **Q/E 切栏** X 移送整堆 ESC/O 关（打开暂停，GameMenu 防抢 ESC 守卫 + DongtianManager O 键守卫；切栏走 `_input` 原始 Q/E，与 GameMenu 翻页一致，←/→ 留给页内横向导航），48 槽自持于 Manager（`get_storage_slot/deposit_from_player/withdraw_to_player`），存档 `data["dongtian"].storage` 段；`scripts/test_dongtian_storage.gd`。**v3 聚灵阵**：洞天内打坐修为 ×N（`Player::get_dongtian_meditate_mult` 挂 `get_meditate_rate`，炼虚 ×2.0 每境 +0.25），打坐提示显「聚灵阵×N」，灵泉旁阵纹视觉。**v4 扩张经营**：**扩张碑 ExpandMonument** (`scripts/spots/expand_monument.gd`，X 灵石购买开辟灵田 **6→12 块**，价格递增下品基准，走 CurrencySystem 四阶钱包，已满拒买) + **阵眼 JlzEye** (`scripts/spots/jlz_eye.gd`，X 升级聚灵阵 **0→2 级**，每级打坐倍率 +0.5，上品×5/×15)；Manager `BASE_PLOTS 6/MAX_PLOTS 12`、`get_expand_cost/expand_plot/get_jlz_upgrade_cost/upgrade_jlz/get_jlz_bonus`；存档段扩展。**设施补全**：**灵泉打坐点 MeditateSpot**（`scripts/spots/meditate_spot.gd`，x=415 灵泉旁，X 模拟 cultivate 走既有打坐管线——提示激活期 attack_just_pressed 被交互压制不出刀，入坐/收功同键，倍率吃 `get_dongtian_meditate_mult`）+ **丹房 PillLab**（`scripts/spots/pill_lab.gd` + `pill_lab_panel.gd`，x=316，X 开 PillLabPanel=GDScript 版炼丹页：GridList 3 列卡片复用 AlchemySystem `get_recipe_list/craft/get_last_message`，↑/↓←/→ 选方 X 炼制 ESC/O 关，打开暂停；GameMenu/DongtianManager 按节点名 `PillLabPanel` 防抢 ESC/O）+ **灵植采集点×2**（`scripts/spots/dongtian_herb_spot.gd`，浮空苗圃单向高台 x=296/420：聚灵草×2·120s / 千年灵芝×1·600s，X 采集入包+喂练气，枯萎现实时间复生；Manager `get_herb_spot/gather_herb_spot/debug_age_herb_spot`，存档 `data["dongtian"].herb_spots` 段）；`scripts/test_dongtian_facilities.gd`
- **数据外抽** (`design/data-externalization.md`) — 全系统摸底 16 系统/170+ 条，P0-P2 已全部 JSON 化（8 系统完全 JSON 驱动）
- **纳戒磁吸** (`src/nodes/item_pickup.*` + `herb_node.*`) — 炼气解锁后 150px 内掉落物/草药自动飞向玩家，渐加速，接触即拾取（草药跳过 X 交互）；速度随境界缩放 `1 + realm × 0.3`（炼气 1.3x → 天尊 4.6x）；未解锁时零开销
- **HUD 消耗品栏** 移至屏幕左下角（x=8, y=246），右端(x=138)对接技能栏左端
- **HUD V/R 冷却指示器** 位于法则条下方（右上角），就绪亮色/冷却灰+秒数
- **GameMenu 能力页** 新增「战技」分区（威压 V / 灵压 R，始终可用）

### Input Map

方向键移动（WASD 已腾出给技能槽，DNF 式），X 普攻+交互+菜单确认合一（交互优先；采集/储物箱/背包使用装备/炼丹/设置确认都用 X，菜单内暂停不冲突；**门口传送门交互已改用 ↑**，X 不进门），C 跳跃（空中再按=飞行），Z 冲刺，V 威压，R 灵压，O 进出洞天（炼虚解锁），Space 确认副键，I 背包（背包页 ↑/↓ 行移 ←/→ 列移，网格顶行再按 ↑ 进类型筛选行、←/→ 循环切类型、↓/X 返回网格），Q 修炼突破，ESC 多页菜单（菜单内 **Q/E 翻页**——翻页严格只用 Q/E 任何行都生效，设置页音量/语言行 ←/→ 为调节，←/→ 不被顶部翻页拦截）；技能槽：A/S 武技、D/F 法术、T 神通、Y 仙法（预留），B 切法宝页（A~H=法宝槽）；数字键 1~6 消耗品快捷栏

### Collision Layers

| Layer | Name | Used By |
|---|---|---|
| 1 | Ground | World geometry |
| 2 | One Way Platform | Pass-through platforms |
| 3 | Player | Player body |
| 4 | Enemy | Enemy body |
| 5 | Player HitBox | Player attacks → detected by Enemy HurtBox |
| 6 | Enemy HitBox | Enemy attacks → detected by Player HurtBox |

### Key System Interactions

- **战斗**: Player.attack() → HitBox 激活（monitoring 关→开重扫重叠，每次激活都重新结算）→ **HitBox 侧驱动伤害**：检测到 HurtBox 即在 hurtbox 上 emit `hurtbox_hit` → owner take_damage（单边结算，HitBox 另发 hit_landed 给连击）。**不可反过来由 HurtBox 监控 HitBox**——持续重叠不发 exit/enter，只有第一下命中。普攻伤害 = `get_effective_attack() × 连击倍率`（装备/功法/buff/被动/宗门/本命/境界全乘区，与技能口径一致）
- **房间切换**: Player 进入 Portal → 加载目标场景 → Camera 约束更新
- **能力门控**: 获得 Ability → AbilityManager 标记 → 之前不可达的区域变为可达
- **修仙**: 击杀/丹药累积修为 → Q 突破（机缘事件待做）→ 境界提升 → 属性/生命/灵力上限提升 → 新能力解锁
- **掉落**: Enemy 死亡 → SignalBus enemy_killed → DropSystem roll 掉落表 → ItemPickup

后续计划与 OOP 抽取候选见 `design/roadmap.md`。
