#include "skill_system.h"

#include "../cultivation/cultivation_system.h"
#include "../cultivation/gongfa_system.h"
#include "../nodes/player.h"
#include "../utils/text.h"

namespace godot {

// 技能定义表（v1 静态表；示例 武技×2 + 法术×2）
static const SkillSystem::Def SKILL_DEFS[] = {
	// 武技（物理，冷却驱动，凡人可用）
	{ "po_kong_zhan", "破空斩", SkillSystem::TYPE_MARTIAL, DMG_PHYSICAL, ELEM_NONE,
	  0.0f, 0.0f, 3.0f, 2.5f, SkillSystem::FX_MELEE_SWING, 0, 0.0f, Color(), 0.0f },
	{ "tu_jin_zhan", "突进斩", SkillSystem::TYPE_MARTIAL, DMG_PHYSICAL, ELEM_NONE,
	  0.0f, 0.0f, 5.0f, 1.8f, SkillSystem::FX_LUNGE, 0, 0.0f, Color(), 0.0f },
	// 法术（元素伤害，耗灵力，炼气解锁）
	{ "huo_dan_shu", "火弹术", SkillSystem::TYPE_SPELL, DMG_ELEMENTAL, ELEM_HUO,
	  15.0f, 0.0f, 2.0f, 2.0f, SkillSystem::FX_PROJECTILE, 1, 220.0f, Color(1.0f, 0.45f, 0.15f), 0.0f },
	{ "bing_zhui_shu", "冰锥术", SkillSystem::TYPE_SPELL, DMG_ELEMENTAL, ELEM_SHUI,
	  25.0f, 0.0f, 4.0f, 3.0f, SkillSystem::FX_PROJECTILE, 1, 260.0f, Color(0.5f, 0.8f, 1.0f), 0.0f },
	// 神通（法则产物，耗法则之力，化神解锁）——示例：缩地成寸（空间法则）
	{ "suo_di_cheng_cun", "缩地成寸", SkillSystem::TYPE_SHENTONG, DMG_PHYSICAL, ELEM_NONE,
	  0.0f, 30.0f, 8.0f, 0.0f, SkillSystem::FX_BLINK, 5, 0.0f, Color(), 120.0f },
};

void SkillSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("learn", "id"), &SkillSystem::learn);
	ClassDB::bind_method(D_METHOD("is_known", "id"), &SkillSystem::is_known);
	ClassDB::bind_method(D_METHOD("assign", "slot", "id"), &SkillSystem::assign);
	ClassDB::bind_method(D_METHOD("cast_slot", "slot"), &SkillSystem::cast_slot);
	ClassDB::bind_method(D_METHOD("get_slot_info", "slot"), &SkillSystem::get_slot_info);
	ClassDB::bind_method(D_METHOD("get_known_list"), &SkillSystem::get_known_list);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &SkillSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &SkillSystem::load_from_dict);

	ADD_SIGNAL(MethodInfo("skill_cast", PropertyInfo(Variant::STRING_NAME, "id")));
	ADD_SIGNAL(MethodInfo("skills_changed"));
}

const SkillSystem::Def *SkillSystem::find_def(const StringName &p_id) {
	for (const Def &d : SKILL_DEFS) {
		if (StringName(d.id) == p_id) return &d;
	}
	return nullptr;
}

String SkillSystem::type_name(SkillType p_t) {
	switch (p_t) {
		case TYPE_MARTIAL: return TXT("武技");
		case TYPE_SPELL: return TXT("法术");
		case TYPE_SHENTONG: return TXT("神通");
		case TYPE_XIANFA: return TXT("仙法");
	}
	return TXT("?");
}

double SkillSystem::_now() const {
	return _player ? _player->get_time() : 0.0;
}

bool SkillSystem::learn(const StringName &p_id) {
	const Def *def = find_def(p_id);
	if (!def) return false;
	if (_known.has(p_id)) return true;
	_known.insert(p_id);
	emit_signal("skills_changed");
	return true;
}

bool SkillSystem::is_known(const StringName &p_id) const {
	return _known.has(p_id);
}

bool SkillSystem::assign(int p_slot, const StringName &p_id) {
	if (p_slot < 0 || p_slot >= SLOT_COUNT) return false;
	const Def *def = find_def(p_id);
	if (!def || !_known.has(p_id)) return false;
	if (def->type != slot_type(p_slot)) return false;
	_slots[p_slot] = p_id;
	emit_signal("skills_changed");
	return true;
}

bool SkillSystem::cast_slot(int p_slot) {
	if (p_slot < 0 || p_slot >= SLOT_COUNT || !_player) return false;
	const StringName &id = _slots[p_slot];
	if (id == StringName()) return false;
	const Def *def = find_def(id);
	if (!def || !_known.has(id)) return false;

	// 境界门控（法术需炼气起）
	CultivationSystem *cult = _player->get_cultivation();
	if (cult && cult->get_realm_index() < def->min_realm) return false;

	// 冷却
	double now = _now();
	auto cd = _cooldown_until.find(id);
	if (cd && cd->value > now) return false;

	// 灵力（法术/仙法——仙法耗仙元，飞升后灵力池即仙元）
	if (def->mana_cost > 0.0f) {
		if (!cult || !cult->consume_mana(def->mana_cost)) return false;
	}
	// 法则之力（神通；独立能量条，不耗灵力）
	if (def->law_cost > 0.0f) {
		if (!cult || !cult->consume_law_power(def->law_cost)) return false;
	}

	_cooldown_until[id] = now + double(def->cooldown);
	_execute(def);

	// 行为喂养功法（design: 近战行为养炼体，耗灵行为养练气）
	if (GongfaSystem *gf = _player->get_gongfa()) {
		if (def->type == TYPE_MARTIAL) {
			gf->feed(GongfaSystem::SCHOOL_BODY, 5.0f);
		} else if (def->mana_cost > 0.0f) {
			gf->feed(GongfaSystem::SCHOOL_QI, def->mana_cost);
		}
	}

	emit_signal("skill_cast", id);
	return true;
}

void SkillSystem::_execute(const Def *p_def) {
	switch (p_def->effect) {
		case FX_MELEE_SWING:
			_player->exec_skill_melee(p_def->power, p_def->category, p_def->element);
			break;
		case FX_LUNGE:
			_player->exec_skill_lunge(p_def->power, p_def->category, p_def->element);
			break;
		case FX_PROJECTILE:
			_player->exec_skill_projectile(p_def->power, p_def->category, p_def->element,
			                               p_def->proj_speed, p_def->proj_color);
			break;
		case FX_BLINK:
			_player->exec_skill_blink(p_def->effect_param);
			break;
	}
}

double SkillSystem::get_cooldown_remaining(const StringName &p_id) const {
	auto cd = _cooldown_until.find(p_id);
	if (!cd) return 0.0;
	double rem = cd->value - _now();
	return rem > 0.0 ? rem : 0.0;
}

StringName SkillSystem::get_slot_skill(int p_slot) const {
	if (p_slot < 0 || p_slot >= SLOT_COUNT) return StringName();
	return _slots[p_slot];
}

Dictionary SkillSystem::get_slot_info(int p_slot) const {
	Dictionary d;
	if (p_slot < 0 || p_slot >= SLOT_COUNT) return d;
	const StringName &id = _slots[p_slot];
	if (id == StringName()) return d;
	const Def *def = find_def(id);
	if (!def) return d;
	d["id"] = String(id);
	d["name"] = TXT(def->name);
	d["type"] = int(def->type);
	d["type_name"] = type_name(def->type);
	d["cooldown"] = def->cooldown;
	d["cd_remaining"] = get_cooldown_remaining(id);
	d["mana_cost"] = def->mana_cost;
	d["law_cost"] = def->law_cost;
	d["power"] = def->power;
	return d;
}

Array SkillSystem::get_known_list() const {
	Array out;
	for (const Def &def : SKILL_DEFS) {
		if (_known.has(StringName(def.id))) {
			Dictionary d;
			d["id"] = String(def.id);
			d["name"] = TXT(def.name);
			d["type"] = int(def.type);
			d["type_name"] = type_name(def.type);
			d["cooldown"] = def.cooldown;
			d["mana_cost"] = def.mana_cost;
			d["law_cost"] = def.law_cost;
			d["power"] = def.power;
			d["min_realm"] = def.min_realm;
			out.append(d);
		}
	}
	return out;
}

Dictionary SkillSystem::save_to_dict() const {
	Dictionary d;
	Array known;
	for (const StringName &id : _known) {
		known.append(String(id));
	}
	d["known"] = known;
	Array slots;
	for (int i = 0; i < SLOT_COUNT; i++) {
		slots.append(String(_slots[i]));
	}
	d["slots"] = slots;
	return d;
}

void SkillSystem::load_from_dict(const Dictionary &p_data) {
	_known.clear();
	for (int i = 0; i < SLOT_COUNT; i++) _slots[i] = StringName();

	if (p_data.has("known")) {
		Array known = p_data["known"];
		for (int i = 0; i < known.size(); i++) {
			StringName id = StringName(String(known[i]));
			if (find_def(id)) _known.insert(id);
		}
	}
	if (p_data.has("slots")) {
		Array slots = p_data["slots"];
		for (int i = 0; i < slots.size() && i < SLOT_COUNT; i++) {
			StringName id = StringName(String(slots[i]));
			const Def *def = find_def(id);
			if (def && _known.has(id) && def->type == slot_type(i)) {
				_slots[i] = id;
			}
		}
	}
	emit_signal("skills_changed");
}

} // namespace godot
