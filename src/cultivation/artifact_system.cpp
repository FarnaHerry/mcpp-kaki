module;
#include "../nodes/player.h"
#include "../nodes/enemy.h"

#include "../utils/text.h"

module mcpp_kaki.cultivation;
import mcpp_kaki.utils;
namespace godot {

// 法宝定义表（v1 静态表；攻击型 = 祭出复用 Skill 管线，辅助型 = 常驻被动乘区）
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
	// 芭蕉扇：风刃扇形（火焰山，扇灭环境火开道）
	{ "ba_jiao_shan", "芭蕉扇", ArtifactSystem::KIND_ATTACK, DMG_ELEMENTAL, ELEM_NONE,
	  25.0f, 4.0f, 3.0f, SkillSystem::FX_PROJ_FAN, 300.0f, Color(0.6f, 0.9f, 0.6f), 0.0f },
	// 八卦炉：辅助型，常驻攻击 +15%×系数（化神残篇习得）
	{ "ba_gua_lu", "八卦炉", ArtifactSystem::KIND_SUPPORT, DMG_PHYSICAL, ELEM_NONE,
	  0.0f, 0.0f, 0.0f, SkillSystem::FX_MELEE_SWING, 0.0f, Color(), 0.0f,
	  0.15f, ELEM_NONE, 0.0f },
	// 捆仙绳：祭出瞬身锁敌——blink 至最近敌人身侧 + 束缚(慑服) + 一击（炼虚残篇习得）
	{ "kun_xian_sheng", "捆仙绳", ArtifactSystem::KIND_ATTACK, DMG_PHYSICAL, ELEM_NONE,
	  25.0f, 8.0f, 2.5f, SkillSystem::FX_BLINK, 0.0f, Color(), 0.0f },
	// 定风珠：辅助型，常驻风抗 +30%×系数（合体残篇习得）
	{ "ding_feng_zhu", "定风珠", ArtifactSystem::KIND_SUPPORT, DMG_PHYSICAL, ELEM_NONE,
	  0.0f, 0.0f, 0.0f, SkillSystem::FX_MELEE_SWING, 0.0f, Color(), 0.0f,
	  0.0f, ELEM_FENG, 0.30f },
};

void ArtifactSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("acquire", "id"), &ArtifactSystem::acquire);
	ClassDB::bind_method(D_METHOD("is_owned", "id"), &ArtifactSystem::is_owned);
	ClassDB::bind_method(D_METHOD("equip", "slot", "id"), &ArtifactSystem::equip);
	ClassDB::bind_method(D_METHOD("activate_slot", "slot"), &ArtifactSystem::activate_slot);
	ClassDB::bind_method(D_METHOD("get_slot_coeff", "slot"), &ArtifactSystem::get_slot_coeff);
	ClassDB::bind_method(D_METHOD("nurture_equipped", "amount"), &ArtifactSystem::nurture_equipped);
	ClassDB::bind_method(D_METHOD("get_passive_def_bonus"), &ArtifactSystem::get_passive_def_bonus);
	ClassDB::bind_method(D_METHOD("get_passive_atk_bonus"), &ArtifactSystem::get_passive_atk_bonus);
	ClassDB::bind_method(D_METHOD("get_passive_elem_resist", "elem"), &ArtifactSystem::get_passive_elem_resist);
	ClassDB::bind_method(D_METHOD("unlock_secondary_slots"), &ArtifactSystem::unlock_secondary_slots);
	ClassDB::bind_method(D_METHOD("is_secondary_unlocked"), &ArtifactSystem::is_secondary_unlocked);
	ClassDB::bind_method(D_METHOD("set_tribulation_mode", "on"), &ArtifactSystem::set_tribulation_mode);
	ClassDB::bind_method(D_METHOD("is_tribulation_mode"), &ArtifactSystem::is_tribulation_mode);
	ClassDB::bind_method(D_METHOD("get_slot_limit"), &ArtifactSystem::get_slot_limit);
	ClassDB::bind_method(D_METHOD("get_slot_info", "slot"), &ArtifactSystem::get_slot_info);
	ClassDB::bind_method(D_METHOD("get_owned_list"), &ArtifactSystem::get_owned_list);
	ClassDB::bind_method(D_METHOD("get_nurture_progress", "id"), &ArtifactSystem::get_nurture_progress);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &ArtifactSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &ArtifactSystem::load_from_dict);
	// 温养来源信号回调（Callable(this,"_on_*") 必须绑定，否则静默失效——CLAUDE.md 潜伏 bug 教训）
	ClassDB::bind_method(D_METHOD("_on_elite_killed", "pos", "tier", "realm"),
	                     &ArtifactSystem::_on_elite_killed);
	ClassDB::bind_method(D_METHOD("_on_boss_died"), &ArtifactSystem::_on_boss_died);
	ClassDB::bind_method(D_METHOD("_on_item_used", "item_id", "quantity"),
	                     &ArtifactSystem::_on_item_used);
	ClassDB::bind_method(D_METHOD("_on_energy_changed", "current", "max", "progress"),
	                     &ArtifactSystem::_on_energy_changed);

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
	return p_k == KIND_ATTACK ? LOC("攻击") : LOC("辅助");
}

double ArtifactSystem::_now() const {
	return _player ? _player->get_time() : 0.0;
}

ArtifactSystem::ArtifactSystem() {
	// SignalBus 由场景装配（world_common）先于 Player 创建，此处通常已可连；
	// 连不上也不碍——acquire/equip/nurture/load 里 _ensure_bus_connected 惰性补连
	_ensure_bus_connected();
}

void ArtifactSystem::_ensure_bus_connected() {
	if (_bus_connected) return;
	SignalBus *bus = SignalBus::get_singleton();
	if (!bus) return;
	// 温养来源扩充：精英/Boss 击杀、服丹、打坐修为，全部走 SignalBus 解耦
	Callable cb_elite(this, "_on_elite_killed");
	if (!bus->is_connected("elite_killed", cb_elite)) {
		bus->connect("elite_killed", cb_elite);
	}
	Callable cb_boss(this, "_on_boss_died");
	if (!bus->is_connected("boss_died", cb_boss)) {
		bus->connect("boss_died", cb_boss);
	}
	Callable cb_item(this, "_on_item_used");
	if (!bus->is_connected("item_used", cb_item)) {
		bus->connect("item_used", cb_item);
	}
	Callable cb_energy(this, "_on_energy_changed");
	if (!bus->is_connected("spiritual_energy_changed", cb_energy)) {
		bus->connect("spiritual_energy_changed", cb_energy);
	}
	_bus_connected = true;
}

// 精英击杀：词缀精英的修为更浑厚，全部已装备法宝温养 +tier×2
void ArtifactSystem::_on_elite_killed(const Vector2 &p_pos, int p_tier, int p_realm) {
	(void)p_pos; (void)p_realm;
	if (p_tier <= 0) return;
	nurture_equipped(NURTURE_PER_KILL_ELITE_TIER * float(p_tier));
}

// Boss 击杀：大额温养，全部已装备法宝 +15（SignalBus boss_died 无参数版，含渡劫天罚使）
void ArtifactSystem::_on_boss_died() {
	nurture_equipped(NURTURE_PER_BOSS);
}

// 服丹：丹药入腹药力淬炼本命法宝 +1/颗
// 丹药判定 = 炼丹产物（AlchemySystem 配方 id 即产出物品 id，recipes.json 数据驱动）——
// 仙桃/人参果/千年珍珠等天材地宝不算丹药
void ArtifactSystem::_on_item_used(const String &p_item_id, int p_quantity) {
	if (!_player || p_quantity <= 0) return;
	if (!AlchemySystem::find_recipe(StringName(p_item_id))) return;
	_player->nurture_benming(NURTURE_PER_PILL * float(p_quantity));
}

// 打坐修为：入定吐纳时灵力流转温养本命 +0.1/次（非打坐状态的修为获取不触发）
void ArtifactSystem::_on_energy_changed(int64_t p_current, int64_t p_max, double p_progress) {
	(void)p_current; (void)p_max; (void)p_progress;
	if (!_player || !_player->is_meditating()) return;
	_player->nurture_benming(NURTURE_PER_MEDITATE_TICK);
}

int ArtifactSystem::get_slot_limit() const {
	// 飞升解锁标记 或 玩家境界已达真仙（旧档兼容：realm 推导兜底）→ 6 槽
	if (_secondary_unlocked) return MAX_SLOTS;
	return _player ? _player->get_artifact_slot_limit() : 3;
}

void ArtifactSystem::unlock_secondary_slots() {
	if (_secondary_unlocked) return;
	_secondary_unlocked = true;
	emit_signal("artifacts_changed");
}

void ArtifactSystem::set_tribulation_mode(bool p_on) {
	if (_tribulation_mode == p_on) return;
	_tribulation_mode = p_on;
	emit_signal("artifacts_changed");
}

bool ArtifactSystem::acquire(const StringName &p_id) {
	_ensure_bus_connected();
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
	_ensure_bus_connected();
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
	// 渡劫「只带本命法宝」：三灾中仅本命槽（0）被动生效
	int n = _tribulation_mode ? 1 : get_slot_limit();
	for (int i = 0; i < n; i++) {
		StringName id = get_slot_artifact(i);
		if (id == StringName()) continue;
		const Def *def = find_def(id);
		if (!def || def->kind != KIND_SUPPORT) continue;
		sum += def->passive_def * get_slot_coeff(i);
	}
	return sum;
}

float ArtifactSystem::get_passive_atk_bonus() const {
	float sum = 0.0f;
	int n = _tribulation_mode ? 1 : get_slot_limit();
	for (int i = 0; i < n; i++) {
		StringName id = get_slot_artifact(i);
		if (id == StringName()) continue;
		const Def *def = find_def(id);
		if (!def || def->kind != KIND_SUPPORT) continue;
		sum += def->passive_atk * get_slot_coeff(i);
	}
	return sum;
}

float ArtifactSystem::get_passive_elem_resist(int p_elem) const {
	float sum = 0.0f;
	int n = _tribulation_mode ? 1 : get_slot_limit();
	for (int i = 0; i < n; i++) {
		StringName id = get_slot_artifact(i);
		if (id == StringName()) continue;
		const Def *def = find_def(id);
		if (!def || def->kind != KIND_SUPPORT) continue;
		if (int(def->resist_elem) != p_elem || def->resist_elem_pct <= 0.0f) continue;
		sum += def->resist_elem_pct * get_slot_coeff(i);
	}
	return sum;
}

bool ArtifactSystem::activate_slot(int p_slot) {
	if (p_slot < 0 || p_slot >= get_slot_limit() || !_player) return false;
	// 渡劫「只带本命法宝」：次要法宝不可祭出
	if (_tribulation_mode && p_slot != 0) return false;
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
		case SkillSystem::FX_PROJ_FAN:
			_player->exec_skill_proj_fan(power, def->category, def->element,
			                             def->proj_speed, def->proj_color);
			break;
		case SkillSystem::FX_BLINK: {
			// 捆仙绳：瞬身锁敌——blink 至最近敌人身侧，束缚（慑服 2.5s）后一击
			Enemy *target = nullptr;
			float best = 300.0f; // 锁敌半径
			if (_player->get_tree()) {
				TypedArray<Node> foes = _player->get_tree()->get_nodes_in_group("enemies");
				Vector2 pp = _player->get_global_position();
				for (int i = 0; i < foes.size(); i++) {
					Enemy *e = Object::cast_to<Enemy>(foes[i]);
					if (!e) continue;
					float d = pp.distance_to(e->get_global_position());
					if (d < best) { best = d; target = e; }
				}
			}
			if (target) {
				Vector2 to = target->get_global_position() - _player->get_global_position();
				_player->facing_direction = to.x >= 0.0f ? 1 : -1;
				float dist = Math::max(0.0f, Math::abs(to.x) - 20.0f); // 停在敌身侧
				if (dist > 1.0f)
					_player->exec_skill_blink(dist);
				target->suppress(2.5); // 束缚：定身+灰显
			}
			_player->exec_skill_melee(power, def->category, def->element);
			break;
		}
	}

	// 芭蕉扇：扇灭火焰山环境火（design/world-map.md 火焰山「芭蕉扇开路」）
	if (id == StringName("ba_jiao_shan") && _player && _player->get_tree()) {
		TypedArray<Node> zones = _player->get_tree()->get_nodes_in_group("fire_zones");
		for (int i = 0; i < zones.size(); i++) {
			Node *zone = Object::cast_to<Node>(zones[i]);
			if (zone && zone->has_method("extinguish"))
				zone->call("extinguish");
		}
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
	_ensure_bus_connected();
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
	d["name"] = LOC(def->name);
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
	d["passive_atk"] = def->passive_atk;
	d["resist_elem"] = int(def->resist_elem);
	d["resist_elem_pct"] = def->resist_elem_pct;
	if (_tribulation_mode && p_slot != 0)
		d["tribulation_off"] = true; // 渡劫中次要法宝禁用（UI 灰显）
	d["nurture"] = p_slot == 0 ? (_player ? _player->get_benming_nurture() : 0.0f)
	                           : (_nurture.has(id) ? _nurture[id] : 0.0f);
	d["benming"] = p_slot == 0;
	// 温养进度可视化（法宝页/测试）；本命与次要共用阶段语义
	Dictionary prog = get_nurture_progress(id);
	for (const Variant &k : prog.keys()) {
		if (k != Variant("nurture") && k != Variant("is_benming") && k != Variant("awakened"))
			d[k] = prog[k];
	}
	return d;
}

Array ArtifactSystem::get_owned_list() const {
	Array out;
	for (const Def &def : ARTIFACT_DEFS) {
		if (_owned.has(StringName(def.id))) {
			Dictionary d;
			d["id"] = String(def.id);
			d["name"] = LOC(def.name);
			d["kind"] = int(def.kind);
			d["kind_name"] = kind_name(def.kind);
			out.append(d);
		}
	}
	return out;
}

Dictionary ArtifactSystem::get_nurture_progress(const StringName &p_id) const {
	Dictionary d;
	bool is_benming = _player && _player->get_benming_artifact() == p_id;
	float nurture = 0.0f;
	if (is_benming) {
		nurture = _player->get_benming_nurture();
	} else if (_nurture.has(p_id)) {
		nurture = _nurture[p_id];
	}
	d["nurture"] = nurture;
	d["is_benming"] = is_benming;
	d["awakened"] = is_benming && _player && _player->is_benming_awakened();

	int stage = 0;
	if (is_benming) {
		// 本命：0 温养中(1.2→1.5) / 1 温养圆满待觉醒 / 2 已觉醒(×2.0 锁定)
		if (_player && _player->is_benming_awakened()) {
			stage = 2;
		} else if (nurture >= NURTURE_STAGE2) {
			stage = 1;
		}
	} else {
		// 次要：0 →×1.2 / 1 →×1.5 / 2 已圆满
		if (nurture >= NURTURE_STAGE2) stage = 2;
		else if (nurture >= NURTURE_STAGE1) stage = 1;
	}
	d["stage"] = stage;

	if (is_benming) {
		d["coeff"] = _player ? _player->get_benming_coeff() : 1.0f;
	} else {
		// 次要系数按所在槽读（未装备/不存在 = 1.0）
		d["coeff"] = 1.0f;
		for (int i = 1; i < MAX_SLOTS; i++) {
			if (get_slot_artifact(i) == p_id) {
				d["coeff"] = get_slot_coeff(i);
				break;
			}
		}
	}

	// ---- 距下一档进度/所需值（阶段语义见上） ----
	switch (stage) {
		case 2: // 已圆满/已觉醒：无下一档
			d["progress"] = 1.0;
			d["next_need"] = 0.0f;
			break;
		case 1: { // 次要 ×1.5 → 圆满（还需 STAGE2-nurture）；本命 150% → 渡劫觉醒（需觉醒，非温养）
			d["progress"] = 1.0;
			d["next_need"] = Math::max(0.0f, is_benming ? 0.0f : (NURTURE_STAGE2 - nurture));
			break;
		}
		default: // stage 0：下一档 = STAGE1
			d["progress"] = CLAMP(nurture / NURTURE_STAGE1, 0.0f, 1.0f);
			d["next_need"] = Math::max(0.0f, NURTURE_STAGE1 - nurture);
			break;
	}
	return d;
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
	d["secondary_unlocked"] = _secondary_unlocked; // 飞升解锁次要槽 +3
	return d;
}

void ArtifactSystem::load_from_dict(const Dictionary &p_data) {
	_ensure_bus_connected();
	_owned.clear();
	for (int i = 1; i < MAX_SLOTS; i++) _slots[i] = StringName();
	_nurture.clear();
	_secondary_unlocked = false;

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
	if (p_data.has("secondary_unlocked")) {
		_secondary_unlocked = bool(p_data["secondary_unlocked"]);
	}
	// 本命槽镜像 Player（benming 段由 GameManager 恢复后此处只回读）
	_slots[0] = _player ? _player->get_benming_artifact() : StringName();
	emit_signal("artifacts_changed");
}

} // namespace godot
