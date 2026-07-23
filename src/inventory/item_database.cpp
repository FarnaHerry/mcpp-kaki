#include "item_database.h"

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

ItemDatabase *ItemDatabase::_singleton = nullptr;

ItemDatabase::ItemDatabase() {
	// Singleton set in _ready() after entering scene tree
}

ItemDatabase::~ItemDatabase() {
	if (_singleton == this) {
		_singleton = nullptr;
	}
}

void ItemDatabase::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	_singleton = this;
	_register_items();
}

void ItemDatabase::_bind_methods() {
	// get_item returns const Item* which godot-cpp can't bind — only for C++ use
	ClassDB::bind_method(D_METHOD("get_item_count"), &ItemDatabase::get_item_count);
}

void ItemDatabase::_register_items() {
	// ---- Consumables ----

	// 回春丹 — Healing Pill
	{
		Item pill;
		pill.id = "healing_pill";
		pill.name = TXT("回春丹");
		pill.description = TXT("恢复 30 点生命值。基础疗伤丹药。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 20;
		pill.heal_amount = 30.0f;
		_items[pill.id] = pill;
	}

	// 聚气丹 — Qi Gathering Pill
	{
		Item pill;
		pill.id = "qi_pill";
		pill.name = TXT("聚气丹");
		pill.description = TXT("吸收后获得 50 点灵力。修炼者日常必备。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 30;
		pill.energy_amount = 50.0f;
		_items[pill.id] = pill;
	}

	// 筑基丹 — Foundation Pill
	{
		Item pill;
		pill.id = "foundation_pill";
		pill.name = TXT("筑基丹");
		pill.description = TXT("突破时提升 20% 成功率。筑基期以下有效。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 5;
		pill.breakthrough_bonus = 0.2f;
		_items[pill.id] = pill;
	}

	// ---- Materials ----

	// 灵石 — Spirit Stone (currency)
	{
		Item stone;
		stone.id = "spirit_stone";
		stone.name = TXT("灵石");
		stone.description = TXT("蕴含灵气的晶石，修炼界的通用货币。");
		stone.type = Item::MATERIAL;
		stone.max_stack = 999;
		_items[stone.id] = stone;
	}

	// 飞剑 — Flying Sword (flight artifact: 筑基御剑飞行必备，金丹以上无需借助)
	{
		Item sword;
		sword.id = "flying_sword";
		sword.name = TXT("飞剑");
		sword.description = TXT("低阶飞行法器。筑基期御之可短暂凌空飞行，持续消耗灵力；金丹之后肉身自飞，此物仅作代步。");
		sword.type = Item::KEY_ITEM;
		sword.max_stack = 1;
		_items[sword.id] = sword;
	}

	// 铁剑 — Iron Sword (equipment: weapon)
	{
		Item sword;
		sword.id = "iron_sword";
		sword.name = TXT("铁剑");
		sword.description = TXT("普通的铁制长剑。可在锻造铺升级。");
		sword.type = Item::EQUIPMENT;
		sword.equip_slot = Item::SLOT_WEAPON;
		sword.max_stack = 1;
		sword.attack_bonus = 5.0f;
		_items[sword.id] = sword;
	}

	// 护体法衣 — Protective Robe (equipment: armor)
	{
		Item robe;
		robe.id = "protect_robe";
		robe.name = TXT("护体法衣");
		robe.description = TXT("附有简单防护法阵的衣袍。");
		robe.type = Item::EQUIPMENT;
		robe.equip_slot = Item::SLOT_ARMOR;
		robe.max_stack = 1;
		robe.defense_bonus = 3.0f;
		_items[robe.id] = robe;
	}
}

const Item *ItemDatabase::get_item(const StringName &p_id) const {
	const Item *found = _items.getptr(p_id);
	return found;
}

} // namespace godot
