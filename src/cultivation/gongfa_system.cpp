#include "gongfa_system.h"

#include "../utils/text.h"
#include "../core/data_loader.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>

#include <godot_cpp/core/class_db.hpp>

namespace godot {

// 功法定义表（v1 静态表；.tres 数据驱动迁移候选，见 roadmap OOP 抽取）
static const GongfaSystem::Def GONGFA_DEFS[] = {
	// 炼体系
	{ "mang_niu_jin", "莽牛劲", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_HUANG, 3,
	  0.04f, 0.03f, 0.02f,  0, 0, 0, 0 },
	{ "tie_bu_shan", "铁布衫", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_XUAN, 5,
	  0.06f, 0.05f, 0.03f,  0, 0, 0, 0 },
	{ "jin_gang_jue", "金刚诀", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_DI, 7,
	  0.09f, 0.08f, 0.05f,  0, 0, 0, 0 },
	// 练气系
	{ "tu_na_jue", "吐纳诀", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_HUANG, 3,
	  0, 0, 0,  0.05f, 0.04f, 0.03f, 0.01f },
	{ "zi_xia_gong", "紫霞功", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_XUAN, 5,
	  0, 0, 0,  0.08f, 0.06f, 0.05f, 0.02f },
	{ "tai_xuan_jing", "太玄经", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_DI, 7,
	  0, 0, 0,  0.12f, 0.09f, 0.08f, 0.03f },
};

std::vector<GongfaSystem::Def> GongfaSystem::s_defs;
bool GongfaSystem::s_defs_loaded = false;

void GongfaSystem::ensure_defs_loaded() {
	if (s_defs_loaded) return;
	s_defs_loaded = true;
	static std::vector<std::string> s_strings;
	SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Node *scene = st ? st->get_current_scene() : nullptr;
	DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
	if (dl) {
		Array all = dl->get_all_gongfas();
		if (all.size() > 0) {
			s_strings.reserve(all.size() * 2);
			for (int i = 0; i < all.size(); i++) {
				Dictionary d = all[i];
				s_strings.push_back(String(d["id"]).utf8().get_data());
				s_strings.push_back(String(d["name"]).utf8().get_data());
				Def def;
				def.id = s_strings[s_strings.size() - 2].c_str();
				def.name = s_strings[s_strings.size() - 1].c_str();
				def.school = School(int(d["school"]));
				def.grade = Grade(int(d["grade"]));
				def.max_layer = int(d["max_layer"]);
				def.hp = float(d["hp"]); def.def = float(d["def"]); def.atk = float(d["atk"]);
				def.mana = float(d["mana"]); def.regen = float(d["regen"]);
				def.spell = float(d["spell"]); def.spd = float(d["spd"]);
				s_defs.push_back(def);
			}
			return;
		}
	}
	for (const Def &d : GONGFA_DEFS) { s_defs.push_back(d); }
}

void GongfaSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("equip_gongfa", "id"), &GongfaSystem::equip_gongfa);
	ClassDB::bind_method(D_METHOD("feed", "school", "base"), &GongfaSystem::feed);
	ClassDB::bind_method(D_METHOD("get_hp_mult"), &GongfaSystem::get_hp_mult);
	ClassDB::bind_method(D_METHOD("get_atk_mult"), &GongfaSystem::get_atk_mult);
	ClassDB::bind_method(D_METHOD("get_spell_mult"), &GongfaSystem::get_spell_mult);
	ClassDB::bind_method(D_METHOD("get_slot_info", "school"), &GongfaSystem::get_slot_info);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &GongfaSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &GongfaSystem::load_from_dict);

	ADD_SIGNAL(MethodInfo("gongfa_changed"));
}

const GongfaSystem::Def *GongfaSystem::find_def(const StringName &p_id) {
	ensure_defs_loaded();
	for (const Def &d : s_defs) {
		if (StringName(d.id) == p_id) return &d;
	}
	return nullptr;
}

String GongfaSystem::grade_name(Grade p_g) {
	switch (p_g) {
		case GRADE_HUANG: return TXT("黄品");
		case GRADE_XUAN:  return TXT("玄品");
		case GRADE_DI:    return TXT("地品");
		case GRADE_TIAN:  return TXT("天品");
	}
	return TXT("?");
}

bool GongfaSystem::equip_gongfa(const StringName &p_id) {
	const Def *def = find_def(p_id);
	if (!def) return false;

	SlotState &slot = _slots[def->school];
	// 旧功法入 _known（保留熟练）
	if (!slot.empty() && slot.id != p_id) {
		_known[slot.id] = { slot.layer, slot.prof };
	}
	if (slot.id == p_id) return true; // 已装配

	// 恢复以前修过的进度，否则从 1 层起步
	if (_known.has(p_id)) {
		auto prev = _known[p_id];
		slot.id = p_id;
		slot.layer = prev.first;
		slot.prof = prev.second;
	} else {
		slot.id = p_id;
		slot.layer = 1;
		slot.prof = 0.0f;
	}
	emit_signal("gongfa_changed");
	return true;
}

void GongfaSystem::feed(School p_school, float p_base) {
	if (p_base <= 0.0f) return;
	// 主系 100%，副系 20%（分系喂养但不偏科）
	_feed_slot(p_school, p_base);
	_feed_slot(p_school == SCHOOL_BODY ? SCHOOL_QI : SCHOOL_BODY, p_base * 0.2f);
}

void GongfaSystem::_feed_slot(School p_s, float p_amount) {
	SlotState &slot = _slots[p_s];
	if (slot.empty()) return;
	const Def *def = find_def(slot.id);
	if (!def || slot.layer >= def->max_layer) return; // 已满层

	slot.prof += p_amount;
	bool changed = false;
	while (slot.layer < def->max_layer && slot.prof >= prof_threshold(slot.layer)) {
		slot.prof -= prof_threshold(slot.layer);
		slot.layer++;
		changed = true;
	}
	if (slot.layer >= def->max_layer) {
		slot.prof = 0.0f;
	}
	if (changed || p_amount > 0.0f) {
		emit_signal("gongfa_changed"); // 层数变化 + 熟练变化都刷新（显示%用）
	}
}

float GongfaSystem::_slot_mult(School p_s, float Def::*p_field) const {
	const SlotState &slot = _slots[p_s];
	if (slot.empty()) return 1.0f;
	const Def *def = find_def(slot.id);
	if (!def) return 1.0f;
	return 1.0f + def->*p_field * float(slot.layer);
}

float GongfaSystem::get_hp_mult() const    { return _slot_mult(SCHOOL_BODY, &Def::hp); }
float GongfaSystem::get_def_mult() const   { return _slot_mult(SCHOOL_BODY, &Def::def); }
float GongfaSystem::get_atk_mult() const   { return _slot_mult(SCHOOL_BODY, &Def::atk); }
float GongfaSystem::get_mana_mult() const  { return _slot_mult(SCHOOL_QI, &Def::mana); }
float GongfaSystem::get_regen_mult() const { return _slot_mult(SCHOOL_QI, &Def::regen); }
float GongfaSystem::get_spell_mult() const { return _slot_mult(SCHOOL_QI, &Def::spell); }
float GongfaSystem::get_speed_mult() const { return _slot_mult(SCHOOL_QI, &Def::spd); }

Dictionary GongfaSystem::get_slot_info(int p_school) const {
	Dictionary d;
	if (p_school < 0 || p_school >= SCHOOL_COUNT) return d;
	const SlotState &slot = _slots[p_school];
	if (slot.empty()) return d;
	const Def *def = find_def(slot.id);
	if (!def) return d;
	d["id"] = String(slot.id);
	d["name"] = TXT(def->name);
	d["grade_name"] = grade_name(def->grade);
	d["layer"] = slot.layer;
	d["max_layer"] = def->max_layer;
	d["prof"] = slot.prof;
	d["threshold"] = prof_threshold(slot.layer);
	return d;
}

Dictionary GongfaSystem::save_to_dict() const {
	Dictionary d;
	for (int i = 0; i < SCHOOL_COUNT; i++) {
		const SlotState &s = _slots[i];
		if (s.empty()) continue;
		Dictionary sd;
		sd["id"] = String(s.id);
		sd["layer"] = s.layer;
		sd["prof"] = s.prof;
		d[i == SCHOOL_BODY ? "body" : "qi"] = sd;
	}
	// _known 也存（换下的功法熟练）
	Dictionary kd;
	for (const auto &kv : _known) {
		Dictionary e;
		e["layer"] = kv.value.first;
		e["prof"] = kv.value.second;
		kd[String(kv.key)] = e;
	}
	d["known"] = kd;
	return d;
}

void GongfaSystem::load_from_dict(const Dictionary &p_data) {
	_slots[SCHOOL_BODY] = SlotState();
	_slots[SCHOOL_QI] = SlotState();
	_known.clear();

	static const char *KEYS[SCHOOL_COUNT] = { "body", "qi" };
	for (int i = 0; i < SCHOOL_COUNT; i++) {
		if (!p_data.has(KEYS[i])) continue;
		Dictionary sd = p_data[KEYS[i]];
		SlotState s;
		s.id = StringName(String(sd.get("id", "")));
		s.layer = int(sd.get("layer", 1));
		s.prof = float(sd.get("prof", 0.0f));
		if (find_def(s.id)) {
			_slots[i] = s;
		}
	}
	if (p_data.has("known")) {
		Dictionary kd = p_data["known"];
		Array keys = kd.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = keys[i];
			Dictionary e = kd[k];
			_known[StringName(k)] = { int(e.get("layer", 1)), float(e.get("prof", 0.0f)) };
		}
	}
	emit_signal("gongfa_changed");
}

} // namespace godot
