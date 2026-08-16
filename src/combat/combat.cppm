// mcpp-kaki combat module — damage types / damage calculator / combo chain /
// hitbox / hurtbox / projectile. SkillSystem stays a header for now (depends on
// Player, handled with the nodes module).
module;

#include <vector>

#include <godot-cpp-m/macros.h>
#include <godot_cpp/templates/hash_map.hpp> // HashMap/HashSet 不被模块重导出，保持文本包含
#include <godot_cpp/templates/hash_set.hpp>

namespace godot {
class Player; // external (nodes header) — global fragment, no module linkage
}

export module mcpp_kaki.combat;

import godot_cpp;

namespace godot {

// 伤害类型体系（design/gongfa-skills.md 第六节，已定稿）：
//   三大类：物理 / 法术 / 元素（五行起步，预留拓展）
//   注意：三灾（Tribulation）走 DMG_ELEMENTAL 元素结算（雷=LEI/阴火=HUO/赑风=FENG），
//   比例抗性可减免、物理防御不能——渡劫不可堆防硬抗。
export enum DamageCategory {
	DMG_PHYSICAL = 0, // 物理：普攻、武技；被防御平减
	DMG_SPELL,        // 法术：无属性法术；被法术抗性按比例减免
	DMG_ELEMENTAL,    // 元素：五行属性伤害；被对应元素抗性减免 + 克制增伤
};

export enum Element {
	ELEM_NONE = 0,
	ELEM_JIN,   // 金
	ELEM_MU,    // 木
	ELEM_SHUI,  // 水
	ELEM_HUO,   // 火
	ELEM_TU,    // 土
	ELEM_LEI,   // 雷（拓展位启用；不入五行克制环）
	ELEM_FENG,  // 风（赑风天劫用；不入五行克制环）
	ELEM_COUNT, // 后续：冰/毒…（数组容量预留 8）
	ELEM_CAPACITY = 8,
};

// 一次伤害的完整描述（攻击方）
export struct DamageInfo {
	float base_amount = 0.0f;                  // 攻击方面板 × 技能倍率后的量
	DamageCategory category = DMG_PHYSICAL;
	Element element = ELEM_NONE;               // DMG_ELEMENTAL 时有效
};

// 防御方抗性剖面
export struct DefenseProfile {
	float defense = 0.0f;                          // 物理防御（平减）
	float spell_resist = 0.0f;                     // 法术抗性（比例 0~0.9）
	float elem_resist[ELEM_CAPACITY] = {};         // 元素抗性（比例 0~1）
	Element self_element = ELEM_NONE;              // 自身五行属性（被克制判定）
};

// 伤害结算唯一入口（design/gongfa-skills.md 第六节，已敲定）。
// 减伤三线、五行克制、后续暴击/穿透/法宝系数——都只往这个类里加。
export class DamageCalculator {
public:
	static constexpr float COUNTER_BONUS = 1.25f;  // 五行克制增伤（v1 只做增伤）
	static constexpr float SPELL_RESIST_CAP = 0.9f;

	// 五行相克：金克木、木克土、土克水、水克火、火克金
	static bool is_counter(Element p_atk, Element p_target) {
		return (p_atk == ELEM_JIN && p_target == ELEM_MU) ||
		       (p_atk == ELEM_MU && p_target == ELEM_TU) ||
		       (p_atk == ELEM_TU && p_target == ELEM_SHUI) ||
		       (p_atk == ELEM_SHUI && p_target == ELEM_HUO) ||
		       (p_atk == ELEM_HUO && p_target == ELEM_JIN);
	}

	static float compute(const DamageInfo &p_info, const DefenseProfile &p_def) {
		float dmg = p_info.base_amount;
		switch (p_info.category) {
			case DMG_PHYSICAL:
				dmg -= p_def.defense; // 平减（沿用现有模型）
				break;
			case DMG_SPELL:
				dmg *= 1.0f - CLAMP(p_def.spell_resist, 0.0f, SPELL_RESIST_CAP);
				break;
			case DMG_ELEMENTAL: {
				float r = 0.0f;
				if (p_info.element > ELEM_NONE && p_info.element < ELEM_CAPACITY) {
					r = p_def.elem_resist[p_info.element];
				}
				dmg *= 1.0f - CLAMP(r, 0.0f, 1.0f);
				if (is_counter(p_info.element, p_def.self_element)) {
					dmg *= COUNTER_BONUS;
				}
				break;
			}
		}
		return MAX(dmg, 1.0f); // 保底 1 点
	}
};

// Manages a multi-hit combo chain.
export struct ComboHit {
	float damage_mult = 1.0f;
	float knockback_mult = 1.0f;
	float startup_time = 0.05f;   // seconds before hitbox activates
	float active_time = 0.1f;     // seconds hitbox stays active
	float recovery_time = 0.15f;  // seconds after active before next action
	int hit_count_required = 0;   // how many landed hits needed to unlock this step
};

export class ComboChain {
public:
	static constexpr int MAX_COMBO = 3;
	static constexpr float CHAIN_WINDOW = 0.5f;   // time after previous attack to chain
	static constexpr float COMBO_TIMEOUT = 1.2f;   // time since last hit to reset combo

	ComboChain();

	// Call when the player presses attack. Returns the combo step to execute (0, 1, 2).
	int start_attack(double p_time);
	void on_hit_landed(double p_time);
	void update(double p_time);

	int get_hit_count() const { return _hit_count; }
	int get_combo_step() const { return _combo_step; }
	bool is_in_combo() const { return _hit_count >= 2; }

	float get_damage_multiplier() const;
	float get_knockback_multiplier() const;
	float get_startup_time() const;
	float get_active_time() const;
	float get_recovery_time() const;

	void reset();
	static const ComboHit &get_hit_definition(int p_step);

private:
	int _hit_count = 0;
	int _combo_step = 0;
	double _last_attack_time = 0.0;
	double _last_hit_time = 0.0;

	ComboHit _hits[MAX_COMBO];
	void _init_hits();
};

// HitBox = 攻击判定框（layer 5），激活时主动检测 HurtBox 并驱动伤害结算。
export class HitBox : public Area2D {
	GDCLASS(HitBox, Area2D);

public:
	float damage = 1.0f;
	float knockback_force = 200.0f;
	float knockback_angle = 0.0f;
	DamageCategory damage_category = DMG_PHYSICAL;
	Element element = ELEM_NONE;

	void set_active(bool p_active);
	bool is_active() const { return _active; }
	void set_knockback_from_facing(int p_facing_direction);

	void _ready() override;
	void _on_area_entered(Area2D *p_area);

protected:
	static void _bind_methods();

private:
	bool _active = false;
	void _update_monitoring();
};

// HurtBox = "可被击中"标记。它自身不检测任何东西（monitoring=关）；
// 伤害由 HitBox 侧检测到重叠后 emit hurtbox_hit 驱动（见 hitbox.cpp 注释）。
export class HurtBox : public Area2D {
	GDCLASS(HurtBox, Area2D);

public:
	void _ready() override;

protected:
	static void _bind_methods();
};

// 技能系统：武技/法术/神通/仙法/被动统一 Skill 管线。
export class SkillSystem : public Object {
	GDCLASS(SkillSystem, Object)

public:
	enum SkillType {
		TYPE_MARTIAL = 0, // 武技（物理）
		TYPE_SPELL,       // 法术（灵力驱动）
		TYPE_SHENTONG,    // 神通（法则之力，步骤5）
		TYPE_XIANFA,      // 仙法（仙元，步骤5）
		TYPE_PASSIVE,     // 被动（学会即常驻，不占槽；数值走乘区，添头级）
	};

	enum PassiveStat {
		PAS_NONE = 0,
		PAS_ATK,
		PAS_SPD,
		PAS_DEF,
		PAS_MANA_REGEN,
		PAS_FLY_SPEED,
		PAS_LAW_REGEN,
		PAS_ELEM_RESIST, // 全元素抗性（菩提心法；加成比例，非乘区）
	};

	enum EffectKind {
		FX_MELEE_SWING = 0,
		FX_LUNGE,
		FX_PROJECTILE,
		FX_BLINK,
		FX_AOE_SWING,
		FX_RISING,
		FX_SELF_BUFF,
		FX_PROJ_FAN,
		FX_INVULN,
	};

	struct Def {
		const char *id;
		const char *name;
		SkillType type;
		DamageCategory category;
		Element element;
		float mana_cost;
		float law_cost;
		float cooldown;
		float power;
		EffectKind effect;
		int min_realm;
		float proj_speed;
		Color proj_color;
		float effect_param;
		const char *buff_id;
		PassiveStat passive_stat;
		float passive_value;
	};

	// 连招派生定义（数据驱动，data/skills.json 技能条目 combo_* 字段）：
	// 施放 after_id 后 window 秒内施放 skill_id → 该次施放伤害 ×mult（只乘 power 出口，
	// 武技=物理/法术=元素都走既有结算入口）。一个技能多个前置 = 多条记录。
	struct ComboDef {
		const char *skill_id; // 被强化的技能（本次施放）
		const char *after_id; // 前置技能
		float window;         // 连招窗口（秒，默认 3.0）
		float mult;           // 伤害倍率
		const char *text;     // 触发提示语（nullptr = 默认话术）
	};

	float get_passive_atk_mult() const { return 1.0f + _passive_sum(PAS_ATK); }
	float get_passive_spd_mult() const { return 1.0f + _passive_sum(PAS_SPD); }
	float get_passive_def_mult() const { return 1.0f + _passive_sum(PAS_DEF); }
	float get_passive_mana_regen_mult() const { return 1.0f + _passive_sum(PAS_MANA_REGEN); }
	float get_passive_fly_mult() const { return 1.0f + _passive_sum(PAS_FLY_SPEED); }
	float get_passive_law_regen_mult() const { return 1.0f + _passive_sum(PAS_LAW_REGEN); }
	float get_passive_elem_resist() const { return _passive_sum(PAS_ELEM_RESIST); } // 全元素抗性加成（0.1 = +10%）

	static const int SLOT_COUNT = 12;
	// 键位映射（QWERTY 上行 0..5 / ASDFGH 下行 6..11）：
	//   Q W E R T Y  = slot 0 1 2 3 4 5
	//   A S D F G H  = slot 6 7 8 9 10 11
	// 类型分组：Q/W/A/S 武技，E/R/D/F 法术，T/Y/G 神通，H 仙法
	static SkillType slot_type(int p_slot) {
		switch (p_slot) {
			case 0: case 1: case 6: case 7: return TYPE_MARTIAL;   // Q W A S
			case 2: case 3: case 8: case 9: return TYPE_SPELL;     // E R D F
			case 4: case 5: case 10: return TYPE_SHENTONG;         // T Y G
			case 11: return TYPE_XIANFA;                           // H
			default: return TYPE_MARTIAL;
		}
	}

	static const Def *find_def(const StringName &p_id);
	static String type_name(SkillType p_t);
	static void ensure_defs_loaded();

	void set_player(Player *p) { _player = p; }

	bool learn(const StringName &p_id);
	bool is_known(const StringName &p_id) const;
	bool assign(int p_slot, const StringName &p_id);
	bool cast_slot(int p_slot);

	double get_cooldown_remaining(const StringName &p_id) const;
	StringName get_slot_skill(int p_slot) const;
	Dictionary get_slot_info(int p_slot) const;
	Array get_known_list() const;

	// 连招派生查询（测试/调试/UI）
	Array get_combo_list() const;                                 // 全连招表（Dictionary 数组）
	float get_last_combo_mult() const { return _last_combo_mult; } // 上次施放实际应用的连招倍率（1.0=未触发）

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	float _passive_sum(PassiveStat p_stat) const;
	Player *_player = nullptr;
	HashSet<StringName> _known;
	StringName _slots[SLOT_COUNT];
	HashMap<StringName, double> _cooldown_until;

	// 连招派生运行时状态：上次成功施放的主动技 + 时间戳
	StringName _last_cast_id;
	double _last_cast_time = -1e9;
	float _last_combo_mult = 1.0f;

	double _now() const;
	void _execute(const Def *p_def, float p_power_mult);
	const ComboDef *_match_combo(const StringName &p_id, double p_now) const;
	void _show_combo_hint(const char *p_text);
	void _on_combo_hint_timeout();
	static std::vector<Def> s_defs;
	static std::vector<ComboDef> s_combos;
	static bool s_defs_loaded;
};

// Reusable projectile — flies in a direction, damages on body hit, auto-destroys.
export class Projectile : public Area2D {
	GDCLASS(Projectile, Area2D);

public:
	float speed = 250.0f;
	float damage = 1.0f;
	Vector2 direction = Vector2(1.0f, 0.0f);
	float lifetime = 3.0f;
	DamageCategory damage_category = DMG_PHYSICAL;
	Element element = ELEM_NONE;
	Color visual_color = Color(1.0f, 0.4f, 0.2f, 0.9f);

	void set_source(Node *p_source) { _source = p_source; }
	Node *get_source() const { return _source; }

	void _ready() override;
	void _physics_process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	Node *_source = nullptr;
	double _age = 0.0;

	void _create_visual();
};

} // namespace godot

VARIANT_ENUM_CAST(godot::SkillSystem::SkillType);
