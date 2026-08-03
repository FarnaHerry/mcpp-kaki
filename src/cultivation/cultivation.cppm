// mcpp-kaki cultivation module — all cultivation systems.
// SkillSystem stays a header (handled with nodes block); its EffectKind is
// referenced by ArtifactSystem and provided via #include in the global fragment.
module;

#include <vector>

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>


#include "../utils/text.h"          // LOC() / TXT()
import mcpp_kaki.combat;

namespace godot {
class Player; // external (nodes header) — global fragment, no module linkage
}

export module mcpp_kaki.cultivation;

namespace godot {

// Forward declarations (defined later in this module).
export class CultivationSystem;
export class TribulationController;

// 三灾——渡劫劫难专用独立枚举。
export enum Tribulation {
	TRIBULATION_LIGHTNING = 0,  // 雷灾（天雷）
	TRIBULATION_YIN_FIRE = 1,   // 阴火（自涌泉穴烧起，非凡火）
	TRIBULATION_BI_WIND = 2,    // 赑风（自囟门吹入，非常风）
	TRIBULATION_COUNT = 3
};

export class AbilityManager : public Object {
	GDCLASS(AbilityManager, Object);

public:
	static const char *ABILITY_DOUBLE_JUMP;
	static const char *ABILITY_WALL_CLING;
	static const char *ABILITY_DASH;
	static const char *ABILITY_AIR_DASH;
	static const char *ABILITY_SPIRIT_VISION;
	static const char *ABILITY_GLIDE;
	static const char *ABILITY_STORAGE_RING;
	static const char *ABILITY_SHORT_FLIGHT;
	static const char *ABILITY_FREE_FLIGHT;
	static const char *ABILITY_SOUL_EXIT;
	static const char *ABILITY_DOMAIN;
	static const char *ABILITY_SPIRIT_TRAVEL;
	static const char *ABILITY_SPIRIT_SENSE;
	static const char *ABILITY_VOID_SHIFT;
	static const char *ABILITY_UNITY_FORM;
	static const char *ABILITY_MERIT_HALO;
	static const char *ABILITY_CLOUD_FLIGHT;
	static const char *ABILITY_TRIBULATION_IMMUNITY;
	static const char *ABILITY_GIANT_FORM;
	static const char *ABILITY_GOLDEN_BODY;
	static const char *ABILITY_DAO_DOMAIN;
	static const char *ABILITY_MYRIAD_AVATARS;

	AbilityManager();

	void set_cultivation(CultivationSystem *p_cultivation) { _cultivation = p_cultivation; }
	void unlock_ability(const StringName &p_ability_id);
	bool has_ability(const StringName &p_ability_id) const;
	void check_realm_unlocks();
	String get_unlocked_list() const;

protected:
	static void _bind_methods();

private:
	HashSet<StringName> _unlocked;
	CultivationSystem *_cultivation = nullptr;
};

export class CultivationSystem : public Object {
	GDCLASS(CultivationSystem, Object);

public:
	enum Realm {
		MORTAL = 0, QI_REFINING = 1, FOUNDATION = 2, GOLDEN_CORE = 3,
		NASCENT_SOUL = 4, SPIRIT_SEVERING = 5, LIAN_XU = 6, HE_TI = 7,
		DA_CHENG = 8, DU_JIE = 9, TRUE_IMMORTAL = 10, GOLDEN_IMMORTAL = 11,
		TIAN_ZUN = 12
	};
	static const int REALM_COUNT = 13;

	enum RealmStage { STAGE_EARLY = 0, STAGE_MID = 1, STAGE_LATE = 2, STAGE_DA_YUANMAN = 3 };
	enum ImmortalType { TYPE_NONE = 0, TYPE_GHOST = 1, TYPE_HUMAN = 2, TYPE_EARTH = 3, TYPE_SPIRIT = 4, TYPE_CELE = 5 };
	enum Sect { SECT_NONE = 0, SECT_DA_LUO = 1, SECT_TAI_YI = 2, SECT_SANXIAN = 3 };
	enum Origin { ORIGIN_MORTAL = 0, ORIGIN_INNATE = 1 };
	enum BuddhistRank { RANK_NONE = 0, RANK_LUO_HAN = 1, RANK_PU_SA = 2, RANK_FO = 3 };
	enum CultivationFocus { FOCUS_NONE = 0, FOCUS_BODY = 1, FOCUS_SPIRIT = 2 };

	CultivationSystem();

	Realm get_current_realm() const { return _current_realm; }
	String get_realm_name() const;
	String get_full_title() const;
	int get_realm_index() const { return (int)_current_realm; }
	bool is_immortal() const { return _current_realm >= TRUE_IMMORTAL; }

	RealmStage get_stage() const;
	String get_stage_name() const;

	int64_t get_spiritual_energy() const { return _lingqi; }
	int64_t get_xianyuan() const { return _xianyuan; }
	int64_t get_current_energy() const { return is_immortal() ? _xianyuan : _lingqi; }
	int64_t get_max_energy() const;
	float get_realm_progress() const;

	void accumulate_energy(double p_amount);

	double get_mana() const { return _mana; }
	double get_max_mana() const;
	String get_mana_name() const { return is_immortal() ? LOC("仙元") : LOC("灵力"); }
	bool consume_mana(double p_cost);
	void restore_mana(double p_amount);
	void set_mana(double p_amount);
	void tick_mana_regen(double p_delta);

	void set_mana_max_mult(double p_m) { _mana_max_mult = p_m; _emit_mana_changed(); }
	void set_mana_regen_mult(double p_m) { _mana_regen_mult = p_m; }
	void set_law_regen_mult(double p_m) { _law_regen_mult = p_m; }

	double get_law_power() const { return _law_power; }
	double get_law_power_max() const { return _current_realm >= SPIRIT_SEVERING ? LAW_POWER_MAX : 0.0; }
	bool consume_law_power(double p_cost);
	void restore_law_power(double p_amount);
	void set_law_power(double p_amount);
	void tick_law_regen(double p_delta);

	void set_spiritual_energy(int64_t p_amount);
	void set_xianyuan(int64_t p_amount);
	void set_realm(int p_realm);

	bool attempt_breakthrough();

	void set_free_breakthrough(bool p_enabled) { _free_breakthrough = p_enabled; }
	bool is_free_breakthrough() const { return _free_breakthrough; }

	bool is_hunyuan() const { return _hunyuan; }
	bool attain_hunyuan();
	void set_hunyuan(bool p_hunyuan);
	bool attain_tianzun();

	ImmortalType get_immortal_type() const { return _immortal_type; }
	String get_immortal_type_name() const;
	bool choose_immortal_type(int p_type);
	void set_immortal_type(int p_type);

	Sect get_sect() const { return _sect; }
	String get_sect_name() const;
	bool choose_sect(int p_sect);
	void set_sect(int p_sect);

	Origin get_origin() const { return _origin; }
	String get_origin_name() const;
	void set_origin(int p_origin);

	BuddhistRank get_buddhist_rank() const { return _buddhist_rank; }
	String get_buddhist_rank_name() const;
	void set_buddhist_rank(int p_rank);

	CultivationFocus get_focus() const { return _focus; }
	String get_focus_name() const;
	bool choose_focus(int p_focus);
	void set_focus(int p_focus);

	float get_damage_multiplier() const;
	float get_defense_multiplier() const;
	float get_speed_multiplier() const;
	double get_max_health() const { return 100.0 * get_defense_multiplier(); }

	int64_t energy_to_next_realm() const;
	bool is_max_realm() const { return _current_realm >= GOLDEN_IMMORTAL; }

	static int64_t get_realm_cap(Realm p_realm);

protected:
	static void _bind_methods();

private:
	Realm _current_realm = MORTAL;
	int64_t _lingqi = 0;
	int64_t _xianyuan = 0;
	double _mana = 0.0;
	double _law_power = 0.0;
	static constexpr double LAW_POWER_MAX = 100.0;
	static constexpr double LAW_REGEN_PER_SEC = 3.0;
	double _mana_max_mult = 1.0;
	double _mana_regen_mult = 1.0;
	double _law_regen_mult = 1.0;
	ImmortalType _immortal_type = TYPE_HUMAN;
	Sect _sect = SECT_NONE;
	Origin _origin = ORIGIN_MORTAL;
	BuddhistRank _buddhist_rank = RANK_NONE;
	CultivationFocus _focus = FOCUS_NONE;
	bool _hunyuan = false;
	bool _free_breakthrough = true;

	void _set_realm_internal(Realm p_realm);
	void _emit_energy_changed();
	void _emit_mana_changed();
	void _emit_law_changed();
	void _notify_name_changed();
};

export class GongfaSystem : public Object {
	GDCLASS(GongfaSystem, Object)

public:
	enum School { SCHOOL_BODY = 0, SCHOOL_QI = 1, SCHOOL_COUNT };
	enum Grade { GRADE_HUANG = 0, GRADE_XUAN, GRADE_DI, GRADE_TIAN };

	struct Def {
		const char *id;
		const char *name;
		School school;
		Grade grade;
		int max_layer;
		float hp, def, atk;
		float mana, regen, spell, spd;
	};

	struct SlotState {
		StringName id;
		int layer = 1;
		float prof = 0.0f;
		bool empty() const { return id == StringName(); }
	};

	static const Def *find_def(const StringName &p_id);
	static void ensure_defs_loaded();
	static String grade_name(Grade p_g);

	bool equip_gongfa(const StringName &p_id);
	void feed(School p_school, float p_base);
	void grant(const StringName &p_id) { equip_gongfa(p_id); }

	float get_hp_mult() const;
	float get_def_mult() const;
	float get_atk_mult() const;
	float get_mana_mult() const;
	float get_regen_mult() const;
	float get_spell_mult() const;
	float get_speed_mult() const;

	const SlotState &get_slot(School p_s) const { return _slots[p_s]; }
	Dictionary get_slot_info(int p_school) const;
	float prof_threshold(int p_layer) const { return 100.0f * p_layer; }

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	SlotState _slots[SCHOOL_COUNT];
	HashMap<StringName, std::pair<int, float>> _known;
	static std::vector<Def> s_defs;
	static bool s_defs_loaded;

	float _slot_mult(School p_s, float Def::*p_field) const;
	void _feed_slot(School p_s, float p_amount);
};

export class BuffSystem : public Object {
	GDCLASS(BuffSystem, Object)

public:
	struct Def {
		const char *id;
		const char *name;
		float duration;
		float atk_mult;
		float def_mult;
		Element elem;
		float elem_resist;
	};

	struct Active {
		StringName id;
		float remaining = 0.0f;
	};

	static const Def *find_def(const StringName &p_id);
	static void ensure_defs_loaded();

	bool apply(const StringName &p_id);
	void remove(const StringName &p_id);
	void clear();
	void tick(double p_delta);

	bool has(const StringName &p_id) const;
	float get_atk_mult() const { return 1.0f + _sum_atk; }
	float get_def_mult() const { return 1.0f + _sum_def; }
	float get_elem_resist_bonus(int p_elem) const;

	Array get_active_list() const;
	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	static std::vector<Def> s_defs;
	static bool s_defs_loaded;
	std::vector<Active> _active;
	float _sum_atk = 0.0f;
	float _sum_def = 0.0f;
	float _sum_elem[ELEM_CAPACITY] = {};

	void _recalc();
	void _emit_changed();
};

export class SectSystem : public Object {
	GDCLASS(SectSystem, Object)

public:
	enum Rank { RANK_OUTER = 0, RANK_INNER = 1, RANK_TRUE = 2, RANK_COUNT };

	struct Def {
		const char *id;
		const char *name;
		const char *desc;
		const char *skill_id;
		float atk[RANK_COUNT];
		float mana[RANK_COUNT];
		float regen[RANK_COUNT];
		float hp[RANK_COUNT];
		float def[RANK_COUNT];
		float kill_xp[RANK_COUNT];
	};

	static const Def *find_def(const StringName &p_id);
	static void ensure_defs_loaded();
	static String rank_name(int p_rank);

	bool in_sect() const { return _sect_id != StringName(); }
	StringName get_sect_id() const { return _sect_id; }
	int get_contribution() const { return _contribution; }
	int get_rank() const;
	String get_rank_name() const;

	bool can_join(const StringName &p_id, int p_realm) const;
	bool join(const StringName &p_id, int p_realm);
	void leave();
	void on_kill(bool p_boss);

	float get_atk_mult() const;
	float get_mana_mult() const;
	float get_regen_mult() const;
	float get_hp_mult() const;
	float get_def_mult() const;
	float get_kill_xp_mult() const;

	Array get_sect_list() const;
	Dictionary get_sect_info() const;
	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	StringName _sect_id;
	static std::vector<Def> s_defs;
	static bool s_defs_loaded;
	int _contribution = 0;
};

export class AlchemySystem : public Object {
	GDCLASS(AlchemySystem, Object)

public:
	static constexpr int MAX_MATS = 3;

	struct Recipe {
		const char *id;
		const char *name;
		const char *mat_id[MAX_MATS];
		int mat_qty[MAX_MATS];
		int mat_count;
		int grade;
		int min_realm;
		float success_rate;
		const char *effect_desc;
	};

	static const Recipe *find_recipe(const StringName &p_id);
	static void ensure_loaded();
	static int get_recipe_count();
	static const Recipe *get_recipe(int p_idx);

	void set_player(Player *p) { _player = p; }

	bool can_craft(const StringName &p_id) const;
	bool is_realm_locked(const Recipe *p_r) const;
	bool craft(const StringName &p_id);
	Array get_recipe_list() const;

	String get_last_message() const { return _last_message; }

protected:
	static void _bind_methods();

private:
	static std::vector<Recipe> s_recipes;
	static bool s_loaded;
	Player *_player = nullptr;
	String _last_message;
};

export class ArtifactSystem : public Object {
	GDCLASS(ArtifactSystem, Object)

public:
	enum Kind {
		KIND_ATTACK = 0,
		KIND_SUPPORT,
	};

	struct Def {
		const char *id;
		const char *name;
		Kind kind;
		DamageCategory category;
		Element element;
		float mana_cost;
		float cooldown;
		float power;
		SkillSystem::EffectKind effect;
		float proj_speed;
		Color proj_color;
		float passive_def;
	};

	static const int MAX_SLOTS = 6;

	static const Def *find_def(const StringName &p_id);
	static String kind_name(Kind p_k);

	void set_player(Player *p) { _player = p; }

	bool acquire(const StringName &p_id);
	bool is_owned(const StringName &p_id) const;
	bool equip(int p_slot, const StringName &p_id);

	bool activate_slot(int p_slot);

	void nurture_equipped(float p_amount);
	float get_slot_coeff(int p_slot) const;
	float get_passive_def_bonus() const;

	int get_slot_limit() const;
	StringName get_slot_artifact(int p_slot) const;
	Dictionary get_slot_info(int p_slot) const;
	Array get_owned_list() const;

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	HashSet<StringName> _owned;
	StringName _slots[MAX_SLOTS];
	HashMap<StringName, float> _nurture;
	HashMap<StringName, double> _cooldown_until;

	double _now() const;
	static constexpr float NURTURE_STAGE1 = 300.0f;
	static constexpr float NURTURE_STAGE2 = 600.0f;
};

export class TitleComposer {
public:
	static String compose(const CultivationSystem &p_cultivation);
};

export class BreakthroughManager : public Node {
	GDCLASS(BreakthroughManager, Node);

public:
	BreakthroughManager();

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	enum class EventKind { NONE, NARRATIVE, COMBAT, TRIBULATION };
	enum class Phase { IDLE, INTRO, ARENA, OUTRO };

	struct EventDef {
		EventKind kind = EventKind::NONE;
		String name;
		std::vector<String> intro_lines;
		std::vector<String> outro_lines;
		int waves = 0;
	};

	bool _active = false;
	int _event_id = -1;
	Phase _phase = Phase::IDLE;
	EventDef _def;

	bool _wave_check_pending = false;
	bool _fail_pending = false;

	CanvasLayer *_overlay = nullptr;
	Label *_title_label = nullptr;
	Label *_body_label = nullptr;
	Label *_hint_label = nullptr;
	std::vector<String> _lines;
	int _line_idx = 0;
	bool _hint_mode = false;
	double _hint_timer = 0.0;

	Node *_arena = nullptr;
	Rect2 _arena_bounds;
	Vector2 _saved_world_pos;
	int _waves_left = 0;
	int _enemies_alive = 0;
	TribulationController *_tribulation = nullptr;

	void _on_breakthrough_requested();
	EventDef _event_for_realm(int p_realm) const;
	void _start_event(const EventDef &p_def, int p_realm);

	void _create_overlay();
	void _begin_lines(const std::vector<String> &p_lines, const String &p_title, bool p_pause);
	void _show_hint(const String &p_text);
	void _hide_overlay(bool p_unpause);
	void _on_intro_finished();
	bool _advance_pressed() const;

	void _load_arena(const String &p_scene_path, const Rect2 &p_bounds);
	void _restore_player_from_arena(bool p_restore_pos);
	void _spawn_wave(int p_wave_idx);
	void _on_event_enemy_died();
	void _wave_check();
	void _enter_tribulation();
	void _on_tribulation_finished(bool p_success);

	void _victory();
	void _on_player_died();
	void _fail_cleanup();
	void _finish(bool p_success);

	Player *_player() const;
	CultivationSystem *_cs() const;
};

export class TribulationController : public Node {
	GDCLASS(TribulationController, Node);

public:
	enum Phase {
		PHASE_THUNDER = 0,
		PHASE_FIRE = 1,
		PHASE_WIND = 2,
		PHASE_DONE = 3
	};

	TribulationController();

	void start_tribulation(Player *p_player, const Rect2 &p_arena);
	void abort();

	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	struct LightningBolt {
		Polygon2D *visual = nullptr;
		float x = 0.0f;
		double strike_at = 0.0;
		double remove_at = 0.0;
		bool struck = false;
	};

	static constexpr int THUNDER_COUNT = 10;
	static constexpr double THUNDER_INTERVAL = 1.3;
	static constexpr double THUNDER_WARN = 0.85;
	static constexpr float THUNDER_HIT_HALF_W = 14.0f;
	static constexpr float THUNDER_DMG_FRAC = 0.15f;
	static constexpr double FIRE_DURATION = 10.0;
	static constexpr double FIRE_TICK = 0.5;
	static constexpr float FIRE_DMG_FRAC = 0.02f;
	static constexpr double WIND_DURATION = 12.0;
	static constexpr double GUST_INTERVAL = 1.8;
	static constexpr float GUST_FORCE = 140.0f;
	static constexpr float WIND_ERODE_FRAC = 0.005f;

	Player *_player = nullptr;
	Rect2 _arena;
	Phase _phase = PHASE_THUNDER;
	double _time = 0.0;
	double _phase_elapsed = 0.0;
	bool _aborted = false;

	int _strikes_spawned = 0;
	double _next_strike_at = 0.0;
	std::vector<LightningBolt> _bolts;

	double _dot_accum = 0.0;

	double _gust_timer = 0.0;
	Vector2 _gust_dir;
	double _erode_accum = 0.0;

	CanvasLayer *_ui = nullptr;
	Label *_phase_label = nullptr;

	void _begin_phase(Phase p_phase);
	void _update_thunder(double p_delta);
	void _update_fire(double p_delta);
	void _update_wind(double p_delta);
	void _spawn_bolt();
	void _update_phase_label();
	void _create_ui();
	void _clear_bolts();
	void _restore_player_effects();
	void _finish();
	String _phase_title() const;
};

} // namespace godot

VARIANT_ENUM_CAST(godot::CultivationSystem::RealmStage);
VARIANT_ENUM_CAST(godot::CultivationSystem::ImmortalType);
VARIANT_ENUM_CAST(godot::CultivationSystem::Sect);
VARIANT_ENUM_CAST(godot::CultivationSystem::Origin);
VARIANT_ENUM_CAST(godot::CultivationSystem::BuddhistRank);
VARIANT_ENUM_CAST(godot::CultivationSystem::CultivationFocus);
VARIANT_ENUM_CAST(godot::GongfaSystem::School);
VARIANT_ENUM_CAST(godot::GongfaSystem::Grade);
VARIANT_ENUM_CAST(godot::ArtifactSystem::Kind);
