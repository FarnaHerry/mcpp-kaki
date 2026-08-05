# Session 012 — 灵石四阶通用货币（独立钱包 + 兑换）

**日期**: 2026-08-06
**分支**: main
**起始提交**: 55f4623（数值平衡）
**结束提交**: （本会话提交）

## 设计（用户拍板 + 我定的兑换比例）

用户要求：灵石直接改为通用货币，独立出来，分下品/中品/上品/极品，都是货币，有兑换比例（比例我定）。

- **兑换比例**：每档 ×10 价值 → 1 极品 = 10 上品 = 100 中品 = 1000 下品。价格一律以下品为基准单位。
- **独立钱包**：灵石不占背包，`CurrencySystem` 独立系统持有四档余额。

## 实现

### CurrencySystem（`src/core/currency_system.*`，singleton + WorldCommon 装配）
- `add(tier, amount)` / `spend(value_base)`：扣总价值，自动破大额 + 找零按**高档优先回填**（130 中品花 25 → 剩 105 = 上品1 + 下品5）
- `exchange(from, qty, to)`：保值兑换，破零（1 上品 → 10 中品）与合成（10 中品 → 1 上品）双向
- `save_to_dict/load_from_dict`；存档 `data["currency"]`

### 拾取路由（Item 新字段 `currency_tier`）
- Item 加 `currency_tier`（-1=普通；0下 1中 2上 3极）；物品 `spirit_stone`(下品)/`spirit_stone_mid/high/peak`
- `Player::pickup_item`：货币直入钱包，不进背包；SignalBus 加 `currency_changed`

### 商店
- ShopSystem buy/sell 改走钱包（卖回下品）
- ShopPanel 顶部**四阶余额**显示 + **第三栏「兑换」**（Q/E 三栏循环，6 条保值兑换 X 全额）

### 存档
- `data["currency"]`；**老档迁移**：inventory 里的 spirit_stone 自动移入钱包下品

### 高阶灵石投放（进度感）
- boss 表加中品灵石；三星洞中品×5、玄冰窟上品×1、玄冥 arena 上品×3、南天门极品×1

## 测试

`test_currency.gd`（9 步）：钱包基本 → 兑换保值（破零/合成）→ spend 自动找零 → 拾取路由（进钱包不进背包）→ 极品破零 → 存档往返。test_shop 更新为钱包驱动。**全量回归通过**（double_jump 已知抖动复跑过）。

## 关键坑

- `register_types.cpp` 未 include currency_system.h → `GDREGISTER_CLASS(CurrencySystem)` 未声明编译错
- spend 找零方向：测试最初断言"中品10+下品5"，实现是**高档优先回填**（上品1+下品5，更紧凑）——改测试断言而非实现
