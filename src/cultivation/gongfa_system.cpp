module;

#include "../utils/text.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>

#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.cultivation;
import mcpp_kaki.utils;
import mcpp_kaki.core;
namespace godot {

// 功法定义表（JSON data/gongfas.json 优先，此为硬编码兜底）
// 品级四阶层数上限（精简定案）：黄 3 / 玄 4 / 地 5 / 天 6
static const GongfaSystem::Def GONGFA_DEFS[] = {
	// 炼体系
	{ "mang_niu_jin", "莽牛劲", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_HUANG, 3,
	  0.04f, 0.03f, 0.02f,  0, 0, 0, 0 },
	{ "tie_bu_shan", "铁布衫", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_XUAN, 4,
	  0.06f, 0.05f, 0.03f,  0, 0, 0, 0 },
	{ "jin_gang_jue", "金刚诀", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_DI, 5,
	  0.09f, 0.08f, 0.05f,  0, 0, 0, 0 },
	// 天品（大乘境界自动领悟）
	{ "long_xiang_gong", "龙象功", GongfaSystem::SCHOOL_BODY, GongfaSystem::GRADE_TIAN, 6,
	  0.12f, 0.10f, 0.07f,  0, 0, 0, 0 },
	// 练气系
	{ "tu_na_jue", "吐纳诀", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_HUANG, 3,
	  0, 0, 0,  0.05f, 0.04f, 0.03f, 0.01f },
	{ "zi_xia_gong", "紫霞功", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_XUAN, 4,
	  0, 0, 0,  0.08f, 0.06f, 0.05f, 0.02f },
	{ "tai_xuan_jing", "太玄经", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_DI, 5,
	  0, 0, 0,  0.12f, 0.09f, 0.08f, 0.03f },
	// 天品（大乘境界自动领悟）
	{ "tai_qing_jing", "太清经", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_TIAN, 6,
	  0, 0, 0,  0.15f, 0.12f, 0.10f, 0.04f },
	// 先天仙品（grade=GRADE_XIAN）：孙悟空《大品天仙诀》——西游记载菩提祖师所传，
	// 躲三灾变化之法的根本。天生仙品，无需飞升晋升，数值即仙品档位（高于天品，
	// 隐藏级）。**暂不投放获得途径**（equip_gongfa 对先天仙品拒绝）：西游记里此诀出自
	// 灵台方寸山斜月三星洞，游戏里已有 rooms/xieyue_sanxing_dong 秘境，将来可在那里
	// 接菩提祖师获得线（届时开专用 grant 通道，勿走 equip_gongfa 门槛）。
	{ "da_pin_tian_xian_jue", "大品天仙诀", GongfaSystem::SCHOOL_QI, GongfaSystem::GRADE_XIAN, 6,
	  0, 0, 0,  0.18f, 0.15f, 0.12f, 0.05f },
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
	ClassDB::bind_method(D_METHOD("is_xian_promoted"), &GongfaSystem::is_xian_promoted);
	ClassDB::bind_method(D_METHOD("get_hp_mult"), &GongfaSystem::get_hp_mult);
	ClassDB::bind_method(D_METHOD("get_atk_mult"), &GongfaSystem::get_atk_mult);
	ClassDB::bind_method(D_METHOD("get_spell_mult"), &GongfaSystem::get_spell_mult);
	ClassDB::bind_method(D_METHOD("get_slot_info", "school"), &GongfaSystem::get_slot_info);
	ClassDB::bind_method(D_METHOD("get_def_info", "id"), &GongfaSystem::get_def_info);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &GongfaSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &GongfaSystem::load_from_dict);
	// 信号回调（Callable(this,"_on_*") 必须绑定，否则静默失效——CLAUDE.md 潜伏 bug 教训）
	ClassDB::bind_method(D_METHOD("_on_realm_changed", "old_realm", "new_realm", "name"),
	                     &GongfaSystem::_on_realm_changed);

	ADD_SIGNAL(MethodInfo("gongfa_changed"));
}

GongfaSystem::GongfaSystem() {
	// SignalBus 由场景装配（world_common）先于 Player 创建，此处通常已可连；
	// 连不上也不碍——feed/equip/load 里 _ensure_bus_connected 惰性补连
	_ensure_bus_connected();
}

void GongfaSystem::_ensure_bus_connected() {
	if (_bus_connected) return;
	SignalBus *bus = SignalBus::get_singleton();
	if (!bus) return;
	Callable cb(this, "_on_realm_changed");
	if (!bus->is_connected("realm_changed", cb)) {
		bus->connect("realm_changed", cb);
	}
	_bus_connected = true;
}

void GongfaSystem::_on_realm_changed(int p_old, int p_new, const String &p_name) {
	(void)p_old; (void)p_name;
	_maybe_grant_tian(p_new);
	if (p_new >= 10) { // 真仙：飞升仙化
		_promote_to_xian();
	}
}

void GongfaSystem::_maybe_grant_tian(int p_realm) {
	if (p_realm < TIAN_GRANT_REALM) return;
	ensure_defs_loaded();
	SignalBus *bus = SignalBus::get_singleton();
	for (const Def &d : s_defs) {
		if (d.grade != GRADE_TIAN) continue;
		StringName id(d.id);
		if (_known.has(id)) continue;                    // 已习得（幂等）
		if (!_slots[d.school].empty() && _slots[d.school].id == id) continue; // 已在修
		// 境界自动领悟：无换装 UI，直接换装（旧功法入 _known 保留熟练）
		equip_gongfa(id);
		UtilityFunctions::print(TXT("[功法] 境界感悟，自动领悟天品功法《"), LOC(d.name), TXT("》"));
		if (bus) {
			bus->emit_signal("interaction_prompt",
			                 String(LOC("境界感悟，领悟天品功法《")) + LOC(d.name) + LOC("》！"), true);
		}
	}
}

void GongfaSystem::_promote_to_xian() {
	if (_xian_promoted) return; // 幂等：已仙化不重复晋升
	_xian_promoted = true;
	UtilityFunctions::print(TXT("[功法] 飞升仙化：所有功法晋升仙品（每层效果 x"),
	                        XIAN_LAYER_MULT, TXT(")"));
	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->emit_signal("interaction_prompt",
		                 LOC("飞升仙化：所修功法尽皆晋升仙品！"), true);
	}
	emit_signal("gongfa_changed");
}

void GongfaSystem::_check_current_realm() {
	// 兜底：读档/老档 realm 已达门槛但标记/天品缺失时补触发（场景未就绪时跳过）
	SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Node *scene = st ? st->get_current_scene() : nullptr;
	Object *p = scene ? scene->find_child("Player", true, false) : nullptr;
	if (!p) return;
	Object *cult = Object::cast_to<Object>(p->call("get_cultivation"));
	if (!cult) return;
	int realm = int(cult->call("get_realm_index"));
	_maybe_grant_tian(realm);
	if (realm >= 10) {
		_promote_to_xian();
	}
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
		case GRADE_HUANG: return LOC("黄品");
		case GRADE_XUAN:  return LOC("玄品");
		case GRADE_DI:    return LOC("地品");
		case GRADE_TIAN:  return LOC("天品");
		case GRADE_XIAN:  return LOC("仙品");
	}
	return LOC("?");
}

bool GongfaSystem::equip_gongfa(const StringName &p_id) {
	const Def *def = find_def(p_id);
	if (!def) return false;
	// 先天仙品（大品天仙诀）暂无获得途径，常规装配/习得一律拒绝
	// （将来斜月三星洞菩提祖师线开专用通道，见 GONGFA_DEFS 注释）
	if (def->grade == GRADE_XIAN) return false;

	_ensure_bus_connected();

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
		slot.layer = MIN(prev.first, def->max_layer); // 老档超上限 clamp
		slot.prof = slot.layer >= def->max_layer ? 0.0f : prev.second;
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
	_ensure_bus_connected();
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
	float per_layer = def->*p_field;
	// 仙化乘区：先天仙品（GRADE_XIAN）数值本身即仙品档位，不吃晋升加成；
	// 其余功法在飞升（_xian_promoted）后每层效果 ×XIAN_LAYER_MULT
	if (def->grade != GRADE_XIAN && _xian_promoted) {
		per_layer *= XIAN_LAYER_MULT;
	}
	return 1.0f + per_layer * float(slot.layer);
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
	// 仙品显示：先天仙品（grade==GRADE_XIAN）或后天仙化（_xian_promoted）
	const bool xian = (def->grade == GRADE_XIAN) || _xian_promoted;
	d["id"] = String(slot.id);
	d["name"] = (xian ? String(LOC("仙·")) : String()) + LOC(def->name);
	d["grade"] = (int)def->grade;
	d["grade_name"] = xian ? LOC("仙品") : grade_name(def->grade);
	d["xian"] = xian;
	d["layer"] = slot.layer;
	d["max_layer"] = def->max_layer;
	d["prof"] = slot.prof;
	d["threshold"] = prof_threshold(slot.layer);
	return d;
}

Dictionary GongfaSystem::get_def_info(const StringName &p_id) const {
	Dictionary d;
	const Def *def = find_def(p_id);
	if (!def) return d;
	d["id"] = String(def->id);
	d["name"] = LOC(def->name);
	d["school"] = (int)def->school;
	d["grade"] = (int)def->grade;
	d["grade_name"] = grade_name(def->grade);
	d["max_layer"] = def->max_layer;
	d["innate_xian"] = (def->grade == GRADE_XIAN); // 先天仙品标记（区别于后天仙化）
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
	d["xian_promoted"] = _xian_promoted; // 后天仙化标记随档
	return d;
}

void GongfaSystem::load_from_dict(const Dictionary &p_data) {
	_slots[SCHOOL_BODY] = SlotState();
	_slots[SCHOOL_QI] = SlotState();
	_known.clear();
	_xian_promoted = bool(p_data.get("xian_promoted", false));

	static const char *KEYS[SCHOOL_COUNT] = { "body", "qi" };
	for (int i = 0; i < SCHOOL_COUNT; i++) {
		if (!p_data.has(KEYS[i])) continue;
		Dictionary sd = p_data[KEYS[i]];
		SlotState s;
		s.id = StringName(String(sd.get("id", "")));
		s.layer = int(sd.get("layer", 1));
		s.prof = float(sd.get("prof", 0.0f));
		if (const Def *def = find_def(s.id)) {
			if (s.layer > def->max_layer) { // 老档层数超新上限：clamp 截断不丢档
				s.layer = def->max_layer;
				s.prof = 0.0f;
			}
			_slots[i] = s;
		}
	}
	if (p_data.has("known")) {
		Dictionary kd = p_data["known"];
		Array keys = kd.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = keys[i];
			Dictionary e = kd[k];
			int layer = int(e.get("layer", 1));
			float prof = float(e.get("prof", 0.0f));
			if (const Def *def = find_def(StringName(k))) {
				if (layer > def->max_layer) { layer = def->max_layer; prof = 0.0f; }
			}
			_known[StringName(k)] = { layer, prof };
		}
	}
	_ensure_bus_connected();
	_check_current_realm(); // 老档迁移：realm 已达门槛但标记缺失时补领悟/补晋升
	emit_signal("gongfa_changed");
}

} // namespace godot
