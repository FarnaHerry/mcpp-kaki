#ifndef CPP_KAKI_CURRENCY_SYSTEM_H
#define CPP_KAKI_CURRENCY_SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

// 灵石货币系统（design/world-map.md 经济；session 012）：
// 通用货币独立成钱包，不占背包。四阶：下品/中品/上品/极品，每档 ×10 价值
// （1 极品 = 10 上品 = 100 中品 = 1000 下品）。价格一律以下品为基准单位。
// 职责：①钱包余额（各档持有 + 总价值）②spend 自动找零（按高档优先回填）
// ③exchange 任意两档互兑（按 RATIO 保值）④存档。
class CurrencySystem : public Node {
	GDCLASS(CurrencySystem, Node);

public:
	static CurrencySystem *get_singleton() { return _singleton; }
	~CurrencySystem();

	enum Tier { TIER_LOW = 0, TIER_MID = 1, TIER_HIGH = 2, TIER_PEAK = 3 };
	static const int TIER_COUNT = 4;
	static const int RATIO = 10; // 每档价值 ×10

	static int tier_value(int p_tier);          // 下品=1 中品=10 上品=100 极品=1000
	static String tier_name(int p_tier);        // "下品" "中品" "上品" "极品"
	static String tier_item_id(int p_tier);     // 对应拾取物 item id（spirit_stone / _mid / _high / _peak）

	int get_amount(int p_tier) const { return _stones[p_tier]; }
	void add(int p_tier, int p_amount);
	bool spend(int p_value_base);   // 扣总价值（下品基准），自动找零/破大额
	bool can_afford(int p_value_base) const;
	int get_total() const;          // 钱包总价值（下品单位）
	bool exchange(int p_from, int p_qty, int p_to); // from→to 保值兑换（可破零/合成）

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static CurrencySystem *_singleton;
	int _stones[TIER_COUNT] = { 0, 0, 0, 0 };
};

} // namespace godot

#endif // CPP_KAKI_CURRENCY_SYSTEM_H
