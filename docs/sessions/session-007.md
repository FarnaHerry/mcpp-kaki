# Session 007 — 食物 / 辟谷系统 v1

**日期**: 2026-08-05
**分支**: main
**起始提交**: 6e1ceb6（ObjectDB 泄漏清理）
**结束提交**: （本会话提交）

## 完成内容

### 饱食度（Player 直接成员，design/cultivation-realms.md 饮食）

- `_fullness/_max_fullness`（默认 100）+ getter/setter/bind
- **衰减**：凡人/炼气 0.3/s（满→空约 5.5 分钟）；**辟谷（筑基）不再衰减**
- **饥饿 debuff**：饱食归零 → `buff_hunger`（攻防 -20%，BuffSystem force-managed：归零 apply/回正 remove），HUD buff 行自动显示
- **食物倍率** `get_food_mult()`：凡人 1.0 / 炼气 1.2
- **辟谷** `is_bigu()` = realm ≥ 筑基：不衰减 + 饱食度锁定满 + HUD 条隐藏 + 食物转纯 buff（不回饱食度）；`_on_cultivation_realm_changed` 到筑基触发 `bigu_changed(true)`

### 数据（物品 + buff，JSON 驱动 + 硬编码 fallback）

- Item 新字段 `fullness_amount`（item_database JSON 加载 + get_item_info + fallback）
- 食物物品：**糙米饭**（full15+果腹防5% 600s）/ **干粮**（full25+干粮攻5% 600s）/ **灵米**（full40+饱足攻防8% 900s，**plantable + grow 180s → 洞天灵田可种**）
- buff：`buff_fullness_low/mid/high` + `buff_hunger`（-0.2 攻/防，99999s force-managed）；data/buffs.json + BUFF_DEFS 同步

### 接入

- `use_consumable`：fullness_amount × get_food_mult() 回饱食度（非辟谷），同时解除饥饿；辟谷只走 buff
- HUD：饱食度条（y60，橙色，仿灵力条 `_build_bar`），境界标签 62→78、寿元 80→96；`on_fullness_changed`（归零红闪）+ `on_bigu_changed`（隐藏条）
- SignalBus：`fullness_changed(float,float)` + `bigu_changed(bool)`
- 存档：pd["fullness"]（辟谷从境界推导不存 max）
- 食物来源：起始干粮×3 + 落霞村外围拾取（糙米饭×2/干粮×2）+ 小怪掉落（25%/15%，drops.json + fallback 表）

### 测试

`scripts/test_food.gd`（10 步）全链路：初始满 → 衰减 → 饥饿 debuff（攻防-20%）→ 食物恢复+解除 → 炼气 120% → 存档恢复 → 筑基辟谷不衰减 → 辟谷食物转 buff → HUD 条隐藏。**全量回归通过**。

### 关键坑

- GDScript `== 1.2` 浮点精度：用 `abs(x - 1.2) < 0.001`
- match 分支各自作用域：跨步变量需成员字段
- 存档测试注意读档后衰减继续（辟谷时 save 的 fullness 会被锁定满覆盖，故存档测试放炼气段）
- 饱食度条/境界/寿元布局下移不影响现有测试（只查文本）
