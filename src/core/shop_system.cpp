#include "shop_system.h"

#include "currency_system.h"
#include "../nodes/player.h"
#include "../utils/text.h"

import mcpp_kaki.inventory; // Item / Inventory / ItemDatabase
import mcpp_kaki.utils;

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <iterator>
#include <string>
#include <vector>

namespace godot {

namespace {

// 商店货架清单：JSON 优先（res://data/shops.json → shops.chang_an.stock）+ 硬编码兜底。
// 幂等 once 缓存；id 拷贝进 std::vector<std::string> 静态池（无 const char* 生命期悬垂）。
// 顶层 "shops" 为多店留余地，ShopSystem 现只读默认的 "chang_an"。
const std::vector<std::string> &_stock_ids() {
	static const char *STOCK[] = {
		"brown_rice", "dry_ration", "spirit_rice",
		"healing_pill", "qi_pill",
		"bing_xin_dan", "chi_yan_dan", "jin_gang_dan",
		"protect_robe", "ren_shen_guo",
	};
	static std::vector<std::string> s_ids;
	static bool s_loaded = false;
	if (s_loaded)
		return s_ids;
	s_loaded = true;

	// 硬编码兜底（与 data/shops.json 的 chang_an.stock 同值）
	s_ids.reserve(std::size(STOCK));
	for (const char *id : STOCK)
		s_ids.emplace_back(id);

	// JSON 优先：读到合法清单则整表替换兜底
	const String path = "res://data/shops.json";
	if (!FileAccess::file_exists(path))
		return s_ids;
	String raw = FileAccess::get_file_as_string(path);
	Variant parsed = JSON::parse_string(raw);
	if (parsed.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr(TXT("ShopSystem: shops.json 顶层须为对象，使用硬编码货架"));
		return s_ids;
	}
	Variant shops_v = Dictionary(parsed).get("shops", Variant());
	if (shops_v.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr(TXT("ShopSystem: shops.json 缺 shops 对象，使用硬编码货架"));
		return s_ids;
	}
	Variant shop_v = Dictionary(shops_v).get("chang_an", Variant());
	if (shop_v.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr(TXT("ShopSystem: shops.json 缺 chang_an 店铺，使用硬编码货架"));
		return s_ids;
	}
	Variant stock_v = Dictionary(shop_v).get("stock", Variant());
	if (stock_v.get_type() != Variant::ARRAY) {
		UtilityFunctions::printerr(TXT("ShopSystem: chang_an 缺 stock 数组，使用硬编码货架"));
		return s_ids;
	}
	Array stock = stock_v;
	std::vector<std::string> ids;
	ids.reserve(stock.size()); // reserve 后再 push，防重分配
	for (int i = 0; i < stock.size(); i++) {
		Variant v = stock[i];
		if (v.get_type() != Variant::STRING)
			continue;
		ids.emplace_back(String(v).utf8().get_data()); // 拷贝进 string 池，id 全 ASCII
	}
	if (ids.empty())
		return s_ids; // 空清单视为无效配置，保留兜底
	s_ids.swap(ids);
	return s_ids;
}

} // namespace

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
	Array out;
	ItemDatabase *db = ItemDatabase::get_singleton();
	for (const std::string &id : _stock_ids()) {
		const Item *def = db ? db->get_item(StringName(id.c_str())) : nullptr;
		if (!def || def->buy_price <= 0)
			continue;
		Dictionary d;
		d["id"] = StringName(id.c_str()); // id 全 ASCII
		d["name"] = def->name;
		d["price"] = def->buy_price;
		out.push_back(d);
	}
	return out;
}

} // namespace godot
