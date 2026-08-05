#include "shop_system.h"

#include "currency_system.h"
#include "../nodes/player.h"

import mcpp_kaki.inventory; // Item / Inventory / ItemDatabase
import mcpp_kaki.utils;

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

ShopSystem *ShopSystem::_singleton = nullptr;

ShopSystem::~ShopSystem() {
	if (_singleton == this)
		_singleton = nullptr;
}

void ShopSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("buy", "player", "item_id"), &ShopSystem::buy);
	ClassDB::bind_method(D_METHOD("sell", "player", "item_id"), &ShopSystem::sell);
	ClassDB::bind_method(D_METHOD("get_spirit_stones", "player"), &ShopSystem::get_spirit_stones);
	ClassDB::bind_method(D_METHOD("get_stock"), &ShopSystem::get_stock);
}

void ShopSystem::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;
	_singleton = this;
}

bool ShopSystem::buy(Player *p, const StringName &p_item_id) {
	if (!p)
		return false;
	Inventory *inv = p->get_inventory();
	if (!inv)
		return false;
	const Item *def = ItemDatabase::get_singleton()->get_item(p_item_id);
	if (!def || def->buy_price <= 0)
		return false; // 商店不售此物
	int price = def->buy_price;
	CurrencySystem *cs = CurrencySystem::get_singleton();
	if (!cs || !cs->can_afford(price))
		return false; // 灵石不足
	if (!cs->spend(price))
		return false;
	if (!inv->add_item(p_item_id, 1)) {
		// 背包满回滚
		cs->add(CurrencySystem::TIER_LOW, price);
		return false;
	}
	return true;
}

bool ShopSystem::sell(Player *p, const StringName &p_item_id) {
	if (!p)
		return false;
	Inventory *inv = p->get_inventory();
	if (!inv)
		return false;
	const Item *def = ItemDatabase::get_singleton()->get_item(p_item_id);
	if (!def || def->sell_price <= 0)
		return false; // 商店不收此物
	if (def->currency_tier >= 0)
		return false; // 货币不进背包、不可卖
	if (!inv->remove_item(p_item_id, 1))
		return false;
	CurrencySystem *cs = CurrencySystem::get_singleton();
	if (cs)
		cs->add(CurrencySystem::TIER_LOW, def->sell_price);
	return true;
}

int ShopSystem::get_spirit_stones(Player *p) const {
	CurrencySystem *cs = CurrencySystem::get_singleton();
	return cs ? cs->get_total() : 0;
}

Array ShopSystem::get_stock() const {
	static const char *STOCK[] = {
		"brown_rice", "dry_ration", "spirit_rice",
		"healing_pill", "qi_pill",
		"bing_xin_dan", "chi_yan_dan", "jin_gang_dan",
		"protect_robe", "ren_shen_guo",
	};
	Array out;
	ItemDatabase *db = ItemDatabase::get_singleton();
	for (const char *id : STOCK) {
		const Item *def = db ? db->get_item(StringName(id)) : nullptr;
		if (!def || def->buy_price <= 0)
			continue;
		Dictionary d;
		d["id"] = StringName(id);
		d["name"] = def->name;
		d["price"] = def->buy_price;
		out.push_back(d);
	}
	return out;
}

} // namespace godot
