#include "sect_system.h"

#include "../utils/text.h"
#include "../core/data_loader.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>

namespace godot {

// 宗门注册表（design/sect-pressure.md 第一节）
static const SectSystem::Def SECT_DEFS[] = {
	//            攻（外/内/真）  灵力            回灵            生命            防御            击杀修为
	{ "shushan", "蜀山剑派", "剑修攻伐，一剑破万法。", "wan_jian_gui_zong",
	  { 0.06f, 0.10f, 0.15f }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
	{ "kunlun", "昆仑道宗", "练气长生，道法自然。", "tai_qing_shen_guang",
	  { 0, 0, 0 }, { 0.10f, 0.16f, 0.24f }, { 0.10f, 0.15f, 0.22f }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } },
	{ "penglai", "蓬莱仙岛", "性命双修，寿与天齐。", "xuan_gui_hu_ti",
	  { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0.08f, 0.12f, 0.18f }, { 0.04f, 0.06f, 0.10f }, { 0, 0, 0 } },
	{ "moluo", "魔罗教", "杀伐精进，以战养战。", "xue_ying_zhan",
	  { 0.03f, 0.05f, 0.08f }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0.15f, 0.25f, 0.40f } },
};

std::vector<SectSystem::Def> SectSystem::s_defs;
bool SectSystem::s_defs_loaded = false;

void SectSystem::ensure_defs_loaded() {
	if (s_defs_loaded) return;
	s_defs_loaded = true;
	static std::vector<std::string> s_strings;
	SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Node *scene = st ? st->get_current_scene() : nullptr;
	DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
	if (dl) {
		Array all = dl->get_all_sects();
		if (all.size() > 0) {
			s_strings.reserve(all.size() * 4);
			for (int i = 0; i < all.size(); i++) {
				Dictionary d = all[i];
				s_strings.push_back(String(d["id"]).utf8().get_data());
				s_strings.push_back(String(d["name"]).utf8().get_data());
				s_strings.push_back(String(d["desc"]).utf8().get_data());
				s_strings.push_back(String(d["skill_id"]).utf8().get_data());
				Def def;
				def.id = s_strings[s_strings.size() - 4].c_str();
				def.name = s_strings[s_strings.size() - 3].c_str();
				def.desc = s_strings[s_strings.size() - 2].c_str();
				def.skill_id = s_strings[s_strings.size() - 1].c_str();
				Array arr;
				arr = d["atk"]; for (int j = 0; j < 3; j++) def.atk[j] = float(arr[j]);
				arr = d["mana"]; for (int j = 0; j < 3; j++) def.mana[j] = float(arr[j]);
				arr = d["regen"]; for (int j = 0; j < 3; j++) def.regen[j] = float(arr[j]);
				arr = d["hp"]; for (int j = 0; j < 3; j++) def.hp[j] = float(arr[j]);
				arr = d["def"]; for (int j = 0; j < 3; j++) def.def[j] = float(arr[j]);
				arr = d["kill_xp"]; for (int j = 0; j < 3; j++) def.kill_xp[j] = float(arr[j]);
				s_defs.push_back(def);
			}
			return;
		}
	}
	for (const Def &d : SECT_DEFS) { s_defs.push_back(d); }
}

static const int RANK_COST[SectSystem::RANK_COUNT] = { 0, 100, 300 }; // 外门/内门/真传

void SectSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("in_sect"), &SectSystem::in_sect);
	ClassDB::bind_method(D_METHOD("get_sect_id"), &SectSystem::get_sect_id);
	ClassDB::bind_method(D_METHOD("get_contribution"), &SectSystem::get_contribution);
	ClassDB::bind_method(D_METHOD("get_rank"), &SectSystem::get_rank);
	ClassDB::bind_method(D_METHOD("get_rank_name"), &SectSystem::get_rank_name);
	ClassDB::bind_method(D_METHOD("can_join", "id", "realm"), &SectSystem::can_join);
	ClassDB::bind_method(D_METHOD("join", "id", "realm"), &SectSystem::join);
	ClassDB::bind_method(D_METHOD("leave"), &SectSystem::leave);
	ClassDB::bind_method(D_METHOD("on_kill", "boss"), &SectSystem::on_kill);
	ClassDB::bind_method(D_METHOD("get_atk_mult"), &SectSystem::get_atk_mult);
	ClassDB::bind_method(D_METHOD("get_mana_mult"), &SectSystem::get_mana_mult);
	ClassDB::bind_method(D_METHOD("get_regen_mult"), &SectSystem::get_regen_mult);
	ClassDB::bind_method(D_METHOD("get_hp_mult"), &SectSystem::get_hp_mult);
	ClassDB::bind_method(D_METHOD("get_def_mult"), &SectSystem::get_def_mult);
	ClassDB::bind_method(D_METHOD("get_kill_xp_mult"), &SectSystem::get_kill_xp_mult);
	ClassDB::bind_method(D_METHOD("get_sect_list"), &SectSystem::get_sect_list);
	ClassDB::bind_method(D_METHOD("get_sect_info"), &SectSystem::get_sect_info);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &SectSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &SectSystem::load_from_dict);
}

const SectSystem::Def *SectSystem::find_def(const StringName &p_id) {
	ensure_defs_loaded();
	for (const Def &d : s_defs) {
		if (p_id == StringName(d.id)) return &d;
	}
	return nullptr;
}

String SectSystem::rank_name(int p_rank) {
	switch (p_rank) {
		case RANK_OUTER: return TXT("外门弟子");
		case RANK_INNER: return TXT("内门弟子");
		case RANK_TRUE:  return TXT("真传弟子");
		default:         return TXT("散修");
	}
}

int SectSystem::get_rank() const {
	if (!in_sect()) return -1;
	int r = 0;
	for (int i = RANK_COUNT - 1; i >= 0; i--) {
		if (_contribution >= RANK_COST[i]) { r = i; break; }
	}
	return r;
}

String SectSystem::get_rank_name() const {
	return rank_name(get_rank());
}

bool SectSystem::can_join(const StringName &p_id, int p_realm) const {
	return !in_sect() && p_realm >= 1 && find_def(p_id) != nullptr;
}

bool SectSystem::join(const StringName &p_id, int p_realm) {
	if (!can_join(p_id, p_realm)) return false;
	_sect_id = p_id;
	_contribution = 0;
	return true;
}

void SectSystem::leave() {
	_sect_id = StringName();
	_contribution = 0;
}

void SectSystem::on_kill(bool p_boss) {
	if (!in_sect()) return;
	_contribution += p_boss ? 10 : 1;
}

// ---- 乘区钩子 ----

float SectSystem::get_atk_mult() const {
	const Def *d = find_def(_sect_id);
	return d ? 1.0f + d->atk[get_rank()] : 1.0f;
}
float SectSystem::get_mana_mult() const {
	const Def *d = find_def(_sect_id);
	return d ? 1.0f + d->mana[get_rank()] : 1.0f;
}
float SectSystem::get_regen_mult() const {
	const Def *d = find_def(_sect_id);
	return d ? 1.0f + d->regen[get_rank()] : 1.0f;
}
float SectSystem::get_hp_mult() const {
	const Def *d = find_def(_sect_id);
	return d ? 1.0f + d->hp[get_rank()] : 1.0f;
}
float SectSystem::get_def_mult() const {
	const Def *d = find_def(_sect_id);
	return d ? 1.0f + d->def[get_rank()] : 1.0f;
}
float SectSystem::get_kill_xp_mult() const {
	const Def *d = find_def(_sect_id);
	return d ? 1.0f + d->kill_xp[get_rank()] : 1.0f;
}

// ---- 数据面 ----

Array SectSystem::get_sect_list() const {
	Array out;
	for (const Def &d : SECT_DEFS) {
		Dictionary c;
		c["id"] = String(d.id);
		c["name"] = TXT(d.name);
		c["desc"] = TXT(d.desc);
		c["skill_id"] = String(d.skill_id);
		c["joined"] = _sect_id == StringName(d.id);
		out.push_back(c);
	}
	return out;
}

Dictionary SectSystem::get_sect_info() const {
	Dictionary d;
	if (!in_sect()) return d;
	const Def *def = find_def(_sect_id);
	if (!def) return d;
	d["id"] = String(def->id);
	d["name"] = TXT(def->name);
	d["desc"] = TXT(def->desc);
	d["rank"] = get_rank();
	d["rank_name"] = get_rank_name();
	d["contribution"] = _contribution;
	int r = get_rank();
	d["next_rank_cost"] = r + 1 < RANK_COUNT ? RANK_COST[r + 1] : -1;
	return d;
}

Dictionary SectSystem::save_to_dict() const {
	Dictionary d;
	d["id"] = String(_sect_id);
	d["contribution"] = _contribution;
	return d;
}

void SectSystem::load_from_dict(const Dictionary &p_data) {
	_sect_id = StringName(String(p_data.get("id", "")));
	_contribution = int(p_data.get("contribution", 0));
	if (!find_def(_sect_id)) { // 脏数据按散修计
		_sect_id = StringName();
		_contribution = 0;
	}
}

} // namespace godot
