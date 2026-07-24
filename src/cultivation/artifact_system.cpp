#include "artifact_system.h"

#include "../nodes/player.h"
#include "cultivation_system.h"
#include "gongfa_system.h"
#include "../utils/text.h"

namespace godot {

// 法宝定义表（v1 静态表；示例 攻击×2 + 辅助×1）
static const ArtifactSystem::Def ARTIFACT_DEFS[] = {
	// 飞剑：筑基飞行道具升级为法宝，剑气祭出（金元素）
	{ "fei_jian", "飞剑", ArtifactSystem::KIND_ATTACK, DMG_ELEMENTAL, ELEM_JIN,
	  10.0f, 1.5f, 1.5f, SkillSystem::FX_PROJECTILE, 320.0f, Color(0.85f, 0.9f, 1.0f), 0.0f },
	// 照妖葫：祭出重击（土元素，近战一挥大威力）
	{ "zhao_yao_hu", "照妖葫", ArtifactSystem::KIND_ATTACK, DMG_ELEMENTAL, ELEM_TU,
	  20.0f, 6.0f, 3.0f, SkillSystem::FX_MELEE_SWING, 0.0f, Color(), 0.0f },
	// 玄铁塔：辅助型，常驻防御 +10%×系数
	{ "xuan_tie_ta", "玄铁塔", ArtifactSystem::KIND_SUPPORT, DMG_PHYSICAL, ELEM_NONE,
	  0.0f, 0.0f, 0.0f, SkillSystem::FX_MELEE_SWING, 0.0f, Color(), 0.10f },
};

void ArtifactSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("acquire", "id"), &ArtifactSystem::acquire);
	ClassDB::bind_method(D_METHOD("is_owned", "id"), &ArtifactSystem::is_owned);
	ClassDB::bind_method(D_METHOD("equip", "slot", "id"), &ArtifactSystem::equip);
	ClassDB::bind_method(D_METHOD("activate_slot", "slot"), &ArtifactSystem::activate_slot);
	ClassDB::bind_method(D_METHOD("get_slot_coeff", "slot"), &ArtifactSystem::get_slot_coeff);
	ClassDB::bind_method(D_METHOD("get_passive_def_bonus"), &ArtifactSystem::get_passive_def_bonus);
	ClassDB::bind_method(D_METHOD("get_slot_limit"), &ArtifactSystem::get_slot_limit);
	ClassDB::bind_method(D_METHOD("get_slot_info", "slot"), &ArtifactSystem::get_slot_info);
	ClassDB::bind_method(D_METHOD("get_owned_list"), &ArtifactSystem::get_owned_list);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &ArtifactSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &ArtifactSystem::load_from_dict);

	ADD_SIGNAL(MethodInfo("artifact_activated", PropertyInfo(Variant::STRING_NAME, "id")));
	ADD_SIGNAL(MethodInfo("artifacts_changed"));
}

const ArtifactSystem::Def *ArtifactSystem::find_def(const StringName &p_id) {
	for (const Def &d : ARTIFACT_DEFS) {
		if (StringName(d.id) == p_id) return &d;
	}
	return nullptr;
}

String ArtifactSystem::kind_name(Kind p_k) {
	return p_k == KIND_ATTACK ? TXT("攻击") : TXT("辅助");
}

double ArtifactSystem::_now() const {
	return _player ? _player->get_time() : 0.0;
}

int ArtifactSystem::get_slot_limit() const {
	return _player ? _player->get_artifact_slot_limit() : 3;
}

bool ArtifactSystem::acquire(const StringName &p_id) {
	const Def *def = find_def(p_id);
	if (!def) return false;
	if (_owned.has(p_id)) return true;
	_owned.insert(p_id);
	emit_signal("artifacts_changed");
	return true;
}

bool ArtifactSystem::is_owned(const StringName &p_id) const {
	return _owned.has(p_id);
}

bool ArtifactSystem::equip(int p_slot, const StringName &p_id) {
	if (p_slot < 0 || p_slot >= get_slot_limit()) return false;
	if (!_owned.has(p_id) || !find_def(p_id)) return false;
	if (p_slot == 0) {
		// 本命槽：与 Player 本命法宝同步（飞升后锁定由 Player 约束）
		_slots[0] = p_id;
		if (_player) {
			_player->set_benming_artifact(p_id);
			_slots[0] = _player->get_benming_artifact(); // 被锁定拒绝时回读实际值
		}
	} else {
		_slots[p_slot] = p_id;
	}
	emit_signal("artifacts_changed");
	return true;
}

StringName ArtifactSystem::get_slot_artifact(int p_slot) const {
	if (p_slot < 0 || p_slot >= MAX_SLOTS) return StringName();
	if (p_slot == 0 && _player) {
		return _player->get_benming_artifact(); // 本命以 Player 为准
	}
	return _slots[p_slot];
}

float ArtifactSystem::get_slot_coeff(int p_slot) const {
	StringName id = get_slot_artifact(p_slot);
	if (id == StringName()) return 0.0f;
	if (p_slot == 0) {
		return _player ? _player->get_benming_coeff() : 1.0f; // 1.2~2.0
	}
	auto n = _nurture.find(id);
	float nurture = n ? n->value : 0.0f;
	if (nurture >= NURTURE_STAGE2) return 1.5f;
	if (nurture >= NURTURE_STAGE1) return 1.2f;
	return 1.0f;
}

float ArtifactSystem::get_passive_def_bonus() const {
	float sum = 0.0f;
	for (int i = 0; i < get_slot_limit(); i++) {
		StringName id = get_slot_artifact(i);
		if (id == StringName()) continue;
		const Def *def = find_def(id);
		if (!def || def->kind != KIND_SUPPORT) continue;
		sum += def->passive_def * get_slot_coeff(i);
	}
	return sum;
}

bool ArtifactSystem::activate_slot(int p_slot) {
	if (p_slot < 0 || p_slot >= get_slot_limit() || !_player) return false;
	StringName id = get_slot_artifact(p_slot);
	if (id == StringName()) return false;
	const Def *def = find_def(id);
	if (!def || def->kind != KIND_ATTACK) return false;

	double now = _now();
	auto cd = _cooldown_until.find(id);
	if (cd && cd->value > now) return false;

	CultivationSystem *cult = _player->get_cultivation();
	if (def->mana_cost > 0.0f) {
		if (!cult || !cult->consume_mana(def->mana_cost)) return false;
	}

	_cooldown_until[id] = now + double(def->cooldown);

	// 祭出：效果 × 法宝系数（复用 Skill 效果执行管线）
	float coeff = get_slot_coeff(p_slot);
	float power = def->power * coeff;
	switch (def->effect) {
		case SkillSystem::FX_MELEE_SWING:
			_player->exec_skill_melee(power, def->category, def->element);
			break;
		case SkillSystem::FX_LUNGE:
			_player->exec_skill_lunge(power, def->category, def->element);
			break;
		case SkillSystem::FX_PROJECTILE:
			_player->exec_skill_projectile(power, def->category, def->element,
			                               def->proj_speed, def->proj_color);
			break;
	}

	// 耗灵行为养练气 + 祭出推进温养
	if (GongfaSystem *gf = _player->get_gongfa()) {
		if (def->mana_cost > 0.0f) {
			gf->feed(GongfaSystem::SCHOOL_QI, def->mana_cost);
		}
	}
	if (p_slot == 0) {
		_player->nurture_benming(10.0f);
	} else {
		_nurture[id] = (_nurture.has(id) ? _nurture[id] : 0.0f) + 10.0f;
	}

	emit_signal("artifact_activated", id);
	return true;
}

void ArtifactSystem::nurture_equipped(float p_amount) {
	for (int i = 0; i < get_slot_limit(); i++) {
		StringName id = get_slot_artifact(i);
		if (id == StringName()) continue;
		if (i == 0) {
			if (_player) _player->nurture_benming(p_amount);
		} else {
			_nurture[id] = (_nurture.has(id) ? _nurture[id] : 0.0f) + p_amount;
		}
	}
}

Dictionary ArtifactSystem::get_slot_info(int p_slot) const {
	Dictionary d;
	if (p_slot < 0 || p_slot >= MAX_SLOTS) return d;
	if (p_slot >= get_slot_limit()) {
		d["locked"] = true;
		return d;
	}
	StringName id = get_slot_artifact(p_slot);
	if (id == StringName()) return d;
	const Def *def = find_def(id);
	if (!def) return d;
	d["id"] = String(id);
	d["name"] = TXT(def->name);
	d["kind"] = int(def->kind);
	d["kind_name"] = kind_name(def->kind);
	d["coeff"] = get_slot_coeff(p_slot);
	d["cooldown"] = def->cooldown;
	double rem = 0.0;
	auto cd = _cooldown_until.find(id);
	if (cd) rem = cd->value - _now();
	d["cd_remaining"] = rem > 0.0 ? rem : 0.0;
	d["mana_cost"] = def->mana_cost;
	d["power"] = def->power;
	d["passive_def"] = def->passive_def;
	d["nurture"] = p_slot == 0 ? (_player ? _player->get_benming_nurture() : 0.0f)
	                           : (_nurture.has(id) ? _nurture[id] : 0.0f);
	d["benming"] = p_slot == 0;
	return d;
}

Array ArtifactSystem::get_owned_list() const {
	Array out;
	for (const Def &def : ARTIFACT_DEFS) {
		if (_owned.has(StringName(def.id))) {
			Dictionary d;
			d["id"] = String(def.id);
			d["name"] = TXT(def.name);
			d["kind"] = int(def.kind);
			d["kind_name"] = kind_name(def.kind);
			out.append(d);
		}
	}
	return out;
}

Dictionary ArtifactSystem::save_to_dict() const {
	Dictionary d;
	Array owned;
	for (const StringName &id : _owned) {
		owned.append(String(id));
	}
	d["owned"] = owned;
	Array slots;
	for (int i = 1; i < MAX_SLOTS; i++) { // 槽0=本命存于 Player/GameManager 的 benming 段
		slots.append(String(_slots[i]));
	}
	d["slots"] = slots;
	Dictionary nurture;
	for (const auto &kv : _nurture) {
		nurture[String(kv.key)] = kv.value;
	}
	d["nurture"] = nurture;
	return d;
}

void ArtifactSystem::load_from_dict(const Dictionary &p_data) {
	_owned.clear();
	for (int i = 1; i < MAX_SLOTS; i++) _slots[i] = StringName();
	_nurture.clear();

	if (p_data.has("owned")) {
		Array owned = p_data["owned"];
		for (int i = 0; i < owned.size(); i++) {
			StringName id = StringName(String(owned[i]));
			if (find_def(id)) _owned.insert(id);
		}
	}
	if (p_data.has("slots")) {
		Array slots = p_data["slots"];
		for (int i = 0; i < slots.size() && i + 1 < MAX_SLOTS; i++) {
			StringName id = StringName(String(slots[i]));
			if (id != StringName() && _owned.has(id)) {
				_slots[i + 1] = id;
			}
		}
	}
	if (p_data.has("nurture")) {
		Dictionary nurture = p_data["nurture"];
		Array keys = nurture.keys();
		for (int i = 0; i < keys.size(); i++) {
			String k = keys[i];
			_nurture[StringName(k)] = float(nurture[k]);
		}
	}
	// 本命槽镜像 Player（benming 段由 GameManager 恢复后此处只回读）
	_slots[0] = _player ? _player->get_benming_artifact() : StringName();
	emit_signal("artifacts_changed");
}

} // namespace godot
