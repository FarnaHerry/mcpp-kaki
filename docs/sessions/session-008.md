# Session 008 — 南赡部洲（长安坊市）+ 商店系统 + 地府进阶

**日期**: 2026-08-06
**分支**: main
**起始提交**: f2315be（食物/辟谷系统）
**结束提交**: （本会话提交）

## 完成内容

### 商店系统（长安坊市灵石买卖，design/world-map.md 南赡部洲）

- **Item 新字段** `buy_price`/`sell_price`（0=不可买卖）：inventory.cppm + item_database JSON 解析 + get_item_info + data/items.json 全量定价（丹药/食物/材料/装备；灵石货币本身/关键物/残卷 0）
- **ShopSystem**（`src/core/shop_system.*`，Node+singleton）：`buy(player,id)` 查价→扣灵石→入库（背包满回滚）；`sell(player,id)` 扣物品→回灵石（货币不可卖、sell_price 0 拒收）；`get_stock()` 硬编码货架（糙米饭/干粮/灵米/回春丹/聚气丹/冰心丹/赤焰丹/金刚丹/护体法衣/人参果）
- **ShopPanel**（`src/nodes/shop_panel.*`，CanvasLayer 116，StoragePanel 双栏模板）：0=商店货架「名×买价」1=玩家背包「名×持有」（只列可卖），Q/E 切栏 X 买卖，顶部灵石余额，打开暂停，ESC/I 关
- **ShopKeeper**（`src/nodes/shop_keeper.*`，Area2D StorageChest 模板）：青衫掌柜，「[X] 交易」→ 打开 ShopPanel
- world_common.gd 挂 ShopSystem/ShopPanel；人参果 buff（攻防+15% 900s）

### 南赡部洲长安（fill stub，炼虚门槛）

- `nanzhanbu.gd`：长安坊市（ShopKeeper 掌柜 1500）+ 五庄观（人参果拾取 + 灵石20）+ **地府入口正式版**（长安城内 DifuGate `enter_difu`）+ 城郊山贼/蛊雕 + 检查点
- 花果山旧 DifuGate 从 bootstrap.gd 移除（地府入口唯一化到长安）
- 人参果新物品（80%回血 + 800修为 + 饱食100 + 攻防15% buff，地品）

### 地府进阶

- **划名豁免**：SoulLedgerSystem 加永久 `_struck` 标记——`mark_soul_exempt()` 置 `_struck=true`（划名=脱离生死轮回），勾魂 spawn 判定 `if (_struck) return`（划名后不再被勾魂）；与免死 `_soul_protection` 分离（免死消耗不影响 `_struck`）；存档持久化
- **审判**：UnderworldInteractNode 加 `MODE_TRIAL`（秦广王黑袍金冠视觉），X → `ledger_inspect_requested` 带 `trial` 数据 → GameHUD overlay 渲染一殿初审核对生死簿叙事（出身/原身/寿元 + 「阳寿未绝放还阳 / 改簿划名永离勾魂」）；difu.gd 加 QinGuangWang 节点

### 测试

- `test_shop.gd`：灵石余额/买（扣+入库）/不足拒买/不售拒买/卖（扣+回）/货币与关键物不可卖/面板开关
- `test_nanzhanbu.gd`：炼虚解锁 → travel_to_direct → 长安内容（掌柜/地府入口/ShopSystem）→ 入口↑进地府 → 还阳回长安
- `test_difu.gd` 重写：勾魂测试移前（未划名才刷无常），新增 秦广王审判 / 划名豁免 / 免死与豁免分离
- **全量回归通过**（test_double_jump 偶发时序抖动，复跑通过）

### 关键坑

- 划名豁免 `_struck` 会「污染」勾魂测试：勾魂 spawn 必须在划名前测（test_difu 重排）
- 花果山入口移除后，地府入口唯一化到长安；test_difu 改用 `gm.enter_difu()` 直进（入口由 test_nanzhanbu 测）
- 灵石（货币）和关键物/残卷设 price 0 防买卖；卖价 buy×0.5 显式写入 sell_price
