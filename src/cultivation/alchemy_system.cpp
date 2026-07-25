#include "alchemy_system.h"

#include "cultivation_system.h"
#include "gongfa_system.h"
#include "../inventory/inventory.h"
#include "../inventory/item.h"
#include "../inventory/item_database.h"
#include "../nodes/player.h"
#include "../utils/text.h"
#include "../core/data_loader.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <string>

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

	// 固定配方表（design/alchemy.md 第三节，7 配方；成功率 v1 全 100%）
	// 金丹 = realm index 3（0凡人 1炼气 2筑基 3金丹）
	static const AlchemySystem::Recipe RECIPES[] = {
		{ "healing_pill", "回春丹",
		  { "zhi_xue_cao", nullptr, nullptr }, { 3, 0, 0 }, 1,
		  0, 0, 1.0f, "回血 30" },
		{ "qi_pill", "聚气丹",
		  { "ju_ling_cao", nullptr, nullptr }, { 3, 0, 0 }, 1,
		  0, 0, 1.0f, "回灵 50" },
		{ "bing_xin_dan", "冰心丹",
		  { "bing_xin_lian", "ju_ling_cao", nullptr }, { 2, 1, 0 }, 2,
		  1, 0, 1.0f, "水抗+15% 300s" },
		{ "chi_yan_dan", "赤焰丹",
		  { "chi_yan_hua", "zhi_xue_cao", nullptr }, { 2, 1, 0 }, 2,
		  1, 0, 1.0f, "攻击+15% 300s" },
		{ "jin_gang_dan", "金刚丹",
		  { "jin_gang_teng", "zhi_xue_cao", nullptr }, { 2, 1, 0 }, 2,
		  1, 0, 1.0f, "防御+20% 300s" },
		{ "wu_dao_dan", "悟道丹",
		  { "wu_dao_cha", "ju_ling_cao", nullptr }, { 1, 2, 0 }, 2,
		  2, 3, 1.0f, "修为+200" },
		{ "da_huan_dan", "大还丹",
		  { "qian_nian_ling_zhi", "bing_xin_lian", nullptr }, { 1, 1, 0 }, 2,
		  2, 3, 1.0f, "回血50%+修为100" },
	};
	static constexpr int RECIPE_COUNT = sizeof(RECIPES) / sizeof(RECIPES[0]);

	const AlchemySystem::Recipe *AlchemySystem::find_recipe(const StringName &p_id) {
		for (const Recipe &r : RECIPES) {
			if (StringName(r.id) == p_id) return &r;
		}
		return nullptr;
	}

	int AlchemySystem::get_recipe_count() { return RECIPE_COUNT; }

	const AlchemySystem::Recipe *AlchemySystem::get_recipe(int p_idx) {
		if (p_idx < 0 || p_idx >= RECIPE_COUNT) return nullptr;
		return &RECIPES[p_idx];
	}

	void AlchemySystem::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_player", "player"), &AlchemySystem::set_player);
		ClassDB::bind_method(D_METHOD("can_craft", "id"), &AlchemySystem::can_craft);
		ClassDB::bind_method(D_METHOD("craft", "id"), &AlchemySystem::craft);
		ClassDB::bind_method(D_METHOD("get_recipe_list"), &AlchemySystem::get_recipe_list);
		ClassDB::bind_method(D_METHOD("get_last_message"), &AlchemySystem::get_last_message);
	}

	bool AlchemySystem::is_realm_locked(const Recipe *p_r) const {
		if (!p_r || p_r->min_realm <= 0) return false;
		if (!_player || !_player->get_cultivation()) return true;
		return _player->get_cultivation()->get_realm_index() < p_r->min_realm;
	}

	bool AlchemySystem::can_craft(const StringName &p_id) const {
		const Recipe *r = find_recipe(p_id);
		if (!r || !_player || !_player->get_inventory()) return false;
		if (is_realm_locked(r)) return false;
		Inventory *inv = _player->get_inventory();
		for (int i = 0; i < r->mat_count; i++) {
			if (inv->get_item_count(StringName(r->mat_id[i])) < r->mat_qty[i]) return false;
		}
		return true;
	}

	bool AlchemySystem::craft(const StringName &p_id) {
		const Recipe *r = find_recipe(p_id);
		if (!r || !_player || !_player->get_inventory()) {
			_last_message = TXT("无法炼制");
			return false;
		}
		if (is_realm_locked(r)) {
			_last_message = TXT("境界不足，丹方难解（金丹起）");
			return false;
		}
		Inventory *inv = _player->get_inventory();
		if (!can_craft(p_id)) {
			_last_message = TXT("材料不足");
			return false;
		}

		// 扣材料
		for (int i = 0; i < r->mat_count; i++) {
			inv->remove_item(StringName(r->mat_id[i]), r->mat_qty[i]);
		}

		// 炼制 = 练气行为（成败皆练，每炉 +5）
		if (_player->get_gongfa()) {
			_player->get_gongfa()->feed(GongfaSystem::SCHOOL_QI, 5.0f);
		}

		// 成功率 roll（v1 全 100%，失败机制预留：失败则材料损毁无产出）
		if (UtilityFunctions::randf() > r->success_rate) {
			_last_message = String(TXT("炼制失败，药材尽毁……"));
			return false;
		}

		inv->add_item(StringName(r->id), 1);
		_last_message = String(TXT("炼成 「")) + TXT(r->name) + TXT("」");
		return true;
	}

	Array AlchemySystem::get_recipe_list() const {
		Array out;
		Inventory *inv = (_player && _player->get_inventory()) ? _player->get_inventory() : nullptr;
		ItemDatabase *db = ItemDatabase::get_singleton();
		for (int ri = 0; ri < RECIPE_COUNT; ri++) {
			const Recipe &r = RECIPES[ri];
			Dictionary d;
			d["id"] = StringName(r.id);
			d["name"] = TXT(r.name);
			d["grade"] = r.grade;
			d["effect"] = TXT(r.effect_desc);
			d["realm_locked"] = is_realm_locked(&r);
			d["min_realm"] = r.min_realm;
			Array mats;
			bool enough = true;
			for (int i = 0; i < r.mat_count; i++) {
				StringName mid(r.mat_id[i]);
				const Item *idef = db ? db->get_item(mid) : nullptr;
				Dictionary m;
				m["id"] = mid;
				m["name"] = idef ? idef->name : String(mid);
				m["need"] = r.mat_qty[i];
				m["have"] = inv ? inv->get_item_count(mid) : 0;
				if ((int)m["have"] < r.mat_qty[i]) enough = false;
				mats.push_back(m);
			}
			d["mats"] = mats;
			d["can_craft"] = enough && !bool(d["realm_locked"]);
			out.push_back(d);
		}
		return out;
	}

} // namespace godot
