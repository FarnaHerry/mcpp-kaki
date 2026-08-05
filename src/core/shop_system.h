#ifndef CPP_KAKI_SHOP_SYSTEM_H
#define CPP_KAKI_SHOP_SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

class Player;

// 商店系统：长安坊市灵石买卖（业务层，design/world-map.md 南赡部洲长安坊市）。
// 灵石 = spirit_stone（MATERIAL）当货币：买扣灵石加物品，卖扣物品回灵石。
// 价格走 Item.buy_price/sell_price（0=不可买卖）。
class ShopSystem : public Node {
	GDCLASS(ShopSystem, Node);

public:
	static ShopSystem *get_singleton() { return _singleton; }
	~ShopSystem();

	void _ready() override;

	bool buy(Player *p, const StringName &p_item_id);  // 买：扣灵石+入库
	bool sell(Player *p, const StringName &p_item_id); // 卖：扣物品+回灵石
	int get_spirit_stones(Player *p) const;            // 玩家灵石数
	Array get_stock() const;                           // 商店货架 Array of {id,name,price}

protected:
	static void _bind_methods();

private:
	static ShopSystem *_singleton;
};

} // namespace godot

#endif // CPP_KAKI_SHOP_SYSTEM_H
