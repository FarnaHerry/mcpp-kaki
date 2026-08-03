module;

#include "../utils/text.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.inventory;
#include <godot_cpp/variant/utility_functions.hpp>

import mcpp_kaki.core;
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

	// Try DataLoader (JSON external data) first; fall back to hardcoded
	Node *scene = get_tree()->get_current_scene();
	DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
	if (dl) {
		Array all = dl->get_all_items();
		if (all.size() > 0) {
			for (int i = 0; i < all.size(); i++) {
				Dictionary d = all[i];
				Item it;
				it.id = StringName(d["id"]);
				it.name = String(d["name"]);
				it.description = String(d["desc"]);
				it.type = Item::Type(int(d.get("type", 0)));
				it.max_stack = int(d.get("max_stack", 99));
				it.grade = int(d.get("grade", 0));
				if (d.has("heal_amount")) it.heal_amount = float(d["heal_amount"]);
				if (d.has("heal_pct")) it.heal_pct = float(d["heal_pct"]);
				if (d.has("mana_amount")) it.mana_amount = float(d["mana_amount"]);
				if (d.has("energy_amount")) it.energy_amount = float(d["energy_amount"]);
				if (d.has("buff_id")) { StringName b = d["buff_id"]; if (!b.is_empty()) it.buff_id = b; }
				if (d.has("learn_skill")) { StringName ls = d["learn_skill"]; if (!ls.is_empty()) it.learn_skill = ls; }
				if (d.has("breakthrough_bonus")) it.breakthrough_bonus = float(d["breakthrough_bonus"]);
				if (d.has("equip_slot")) it.equip_slot = Item::EquipSlot(int(d["equip_slot"]));
				if (d.has("attack_bonus")) it.attack_bonus = float(d["attack_bonus"]);
				if (d.has("defense_bonus")) it.defense_bonus = float(d["defense_bonus"]);
				if (d.has("speed_bonus")) it.speed_bonus = float(d["speed_bonus"]);
				_items[it.id] = it;
			}
			return;
		}
	}
	// Fallback: hardcoded items
	_register_items();
}

void ItemDatabase::_bind_methods() {
	// get_item returns const Item* which godot-cpp can't bind — only for C++ use
	ClassDB::bind_method(D_METHOD("get_item_count"), &ItemDatabase::get_item_count);
	ClassDB::bind_method(D_METHOD("has_item", "id"), &ItemDatabase::has_item);
	ClassDB::bind_method(D_METHOD("get_item_info", "id"), &ItemDatabase::get_item_info);
}

Dictionary ItemDatabase::get_item_info(const StringName &p_id) const {
	Dictionary d;
	const Item *it = get_item(p_id);
	if (!it) return d;
	d["id"] = it->id;
	d["name"] = it->name;
	d["description"] = it->description;
	d["type"] = (int)it->type;
	d["max_stack"] = it->max_stack;
	d["grade"] = it->grade;
	d["heal_amount"] = it->heal_amount;
	d["heal_pct"] = it->heal_pct;
	d["mana_amount"] = it->mana_amount;
	d["energy_amount"] = it->energy_amount;
	d["buff_id"] = it->buff_id;
	d["learn_skill"] = it->learn_skill;
	return d;
}

void ItemDatabase::_register_items() {
	// ---- Consumables ----

	// 回春丹 — Healing Pill
	{
		Item pill;
		pill.id = "healing_pill";
		pill.name = LOC("回春丹");
		pill.description = LOC("恢复 30 点生命值。基础疗伤丹药。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 20;
		pill.heal_amount = 30.0f;
		_items[pill.id] = pill;
	}

	// 聚气丹 — Qi Gathering Pill
	{
		Item pill;
		pill.id = "qi_pill";
		pill.name = LOC("聚气丹");
		pill.description = LOC("吸收后恢复 50 点灵力。修炼者日常必备。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 30;
		pill.mana_amount = 50.0f;
		_items[pill.id] = pill;
	}

	// 筑基丹 — Foundation Pill
	{
		Item pill;
		pill.id = "foundation_pill";
		pill.name = LOC("筑基丹");
		pill.description = LOC("突破时提升 20% 成功率。筑基期以下有效。");
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
		stone.name = LOC("灵石");
		stone.description = LOC("蕴含灵气的晶石，修炼界的通用货币。");
		stone.type = Item::MATERIAL;
		stone.max_stack = 999;
		_items[stone.id] = stone;
	}

	// 飞剑 — Flying Sword (flight artifact: 筑基御剑飞行必备，金丹以上无需借助)
	{
		Item sword;
		sword.id = "flying_sword";
		sword.name = LOC("飞剑");
		sword.description = LOC("低阶飞行法器。筑基期御之可短暂凌空飞行，持续消耗灵力；金丹之后肉身自飞，此物仅作代步。");
		sword.type = Item::KEY_ITEM;
		sword.max_stack = 1;
		_items[sword.id] = sword;
	}

	// 铁剑 — Iron Sword (equipment: weapon)
	{
		Item sword;
		sword.id = "iron_sword";
		sword.name = LOC("铁剑");
		sword.description = LOC("普通的铁制长剑。可在锻造铺升级。");
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
		robe.name = LOC("护体法衣");
		robe.description = LOC("附有简单防护法阵的衣袍。");
		robe.type = Item::EQUIPMENT;
		robe.equip_slot = Item::SLOT_ARMOR;
		robe.max_stack = 1;
		robe.defense_bonus = 3.0f;
		_items[robe.id] = robe;
	}

	// ---- 花果山/东海之滨（design/world-map.md 东胜神洲补完）----

	// 仙桃 — 花果山桃林灵果（大回血+修为）
	{
		Item peach;
		peach.id = "xian_tao";
		peach.name = LOC("仙桃");
		peach.description = LOC("花果山桃林所结灵果，三千年一熟。食之气血充盈，修为精进。");
		peach.type = Item::CONSUMABLE;
		peach.max_stack = 10;
		peach.grade = 1;
		peach.heal_pct = 0.5f;
		peach.energy_amount = 300.0f;
		_items[peach.id] = peach;
	}

	// 身外化身残卷 — 水帘洞秘藏（使用习得神通「身外化身」）
	{
		Item scroll;
		scroll.id = "shen_wai_can_juan";
		scroll.name = LOC("身外化身残卷");
		scroll.description = LOC("水帘洞石壁暗格所藏残卷，记载齐天大圣成名神通。参悟可习得「身外化身」。");
		scroll.type = Item::CONSUMABLE;
		scroll.max_stack = 1;
		scroll.grade = 2;
		scroll.learn_skill = StringName("shen_wai_hua_shen");
		_items[scroll.id] = scroll;
	}

	// 定海神针铁 — 东海之滨镇海神铁（武器，重一万三千五百斤）
	{
		Item rod;
		rod.id = "ding_hai_shen_zhen";
		rod.name = LOC("定海神针铁");
		rod.description = LOC("大禹治水时测定江海深浅的神铁，重一万三千五百斤。大圣归去后沉寂东海之滨，静待有缘。");
		rod.type = Item::EQUIPMENT;
		rod.equip_slot = Item::SLOT_WEAPON;
		rod.max_stack = 1;
		rod.grade = 2;
		rod.attack_bonus = 25.0f;
		_items[rod.id] = rod;
	}

	// ---- 草药（MATERIAL，design/alchemy.md 第二节）----

	auto herb = [&](const char *id, const char *name, const char *desc, int grade) {
		Item h;
		h.id = id;
		h.name = LOC(name);
		h.description = LOC(desc);
		h.type = Item::MATERIAL;
		h.max_stack = 99;
		h.grade = grade;
		_items[h.id] = h;
	};
	herb("zhi_xue_cao", "止血草", "最常见的药草，捣汁可止血生肌。回春丹主材。", 0);
	herb("ju_ling_cao", "聚灵草", "叶脉含灵气脉络，凝聚天地灵气。聚气丹主材。", 0);
	herb("bing_xin_lian", "冰心莲", "生于高寒之地的雪莲，触手生凉。冰心丹主材。", 1);
	herb("chi_yan_hua", "赤焰花", "洞穴深处吞吐地火的红花。赤焰丹主材。", 1);
	herb("jin_gang_teng", "金刚藤", "攀于绝壁的铁色藤蔓，坚逾精钢。金刚丹主材。", 1);
	herb("wu_dao_cha", "悟道茶", "传说古修坐化处生出的茶树，一叶一悟。悟道丹主材。", 2);
	herb("qian_nian_ling_zhi", "千年灵芝", "千年灵木根际所生紫芝，药力浑厚。大还丹主材。", 2);

	// ---- 新丹药（CONSUMABLE，design/alchemy.md 第三节配方产物）----

	// 冰心丹 — 水抗 buff
	{
		Item pill;
		pill.id = "bing_xin_dan";
		pill.name = LOC("冰心丹");
		pill.description = LOC("服之百脉清凉，水寒不侵。水抗+15%，持续 300 息。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 10;
		pill.grade = 1;
		pill.buff_id = "buff_bing_xin";
		_items[pill.id] = pill;
	}

	// 赤焰丹 — 攻击 buff
	{
		Item pill;
		pill.id = "chi_yan_dan";
		pill.name = LOC("赤焰丹");
		pill.description = LOC("地火炼就，服之气血沸腾。攻击+15%，持续 300 息。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 10;
		pill.grade = 1;
		pill.buff_id = "buff_chi_yan";
		_items[pill.id] = pill;
	}

	// 金刚丹 — 防御 buff
	{
		Item pill;
		pill.id = "jin_gang_dan";
		pill.name = LOC("金刚丹");
		pill.description = LOC("金铁之气淬体，刀枪难伤。防御+20%，持续 300 息。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 10;
		pill.grade = 1;
		pill.buff_id = "buff_jin_gang";
		_items[pill.id] = pill;
	}

	// 悟道丹 — 修为
	{
		Item pill;
		pill.id = "wu_dao_dan";
		pill.name = LOC("悟道丹");
		pill.description = LOC("一叶一悟，豁然开朗。修为+200。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 5;
		pill.grade = 2;
		pill.energy_amount = 200.0f;
		_items[pill.id] = pill;
	}

	// 大还丹 — 大回血+修为
	{
		Item pill;
		pill.id = "da_huan_dan";
		pill.name = LOC("大还丹");
		pill.description = LOC("生死人肉白骨。恢复 50% 生命，修为+100。");
		pill.type = Item::CONSUMABLE;
		pill.max_stack = 5;
		pill.grade = 2;
		pill.heal_pct = 0.5f;
		pill.energy_amount = 100.0f;
		_items[pill.id] = pill;
	}
}

const Item *ItemDatabase::get_item(const StringName &p_id) const {
	const Item *found = _items.getptr(p_id);
	return found;
}

} // namespace godot
