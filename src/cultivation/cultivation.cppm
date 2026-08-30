// mcpp-kaki cultivation module — all cultivation systems.
// SkillSystem stays a header (handled with nodes block); its EffectKind is
// referenced by ArtifactSystem and provided via #include in the global fragment.
module;

#include <vector>

#include <godot-cpp-m/macros.h>
#include <godot_cpp/templates/hash_map.hpp> // HashMap/HashSet 不被模块重导出，保持文本包含
#include <godot_cpp/templates/hash_set.hpp>


#include "../utils/text.h"          // LOC() / TXT()
import mcpp_kaki.combat;

namespace godot {
class Player; // external (nodes header) — global fragment, no module linkage
class Enemy;  // external (nodes header)
}

export module mcpp_kaki.cultivation;

import godot_cpp;

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
	static const char *ABILITY_DONGTIAN;
	static const char *ABILITY_UNITY_FORM;
	static const char *ABILITY_MERIT_HALO;
	static const char *ABILITY_CLOUD_FLIGHT;
	static const char *ABILITY_TRIBULATION_IMMUNITY;
	static const char *ABILITY_GIANT_FORM;
	static const char *ABILITY_GOLDEN_BODY;
	static const char *ABILITY_DAO_DOMAIN;
	static const char *ABILITY_MYRIAD_AVATARS;

	// 能力解锁表（data/abilities.json，design/data-externalization.md 十二节）：
	// ensure_defs_loaded 惰性装载，DataLoader 可用走 JSON（数组序=能力页显示序），
	// 否则回退硬编码兜底表（值与 JSON 同步）。
	struct AbilityDef {
		const char *id;
		const char *name;
		bool active;      // true=主动 false=被动（能力页分区）
		bool innate;      // 初始即会（不入境界解锁流程，构造时已解锁）
		int unlock_realm; // 境界门控（-1=非境界：innate/混元）
		bool hunyuan;     // 混元一气解锁（unlock_realm=-1 时看此位）
		const char *cond; // 解锁条件显示文本（能力页）
		const char *desc; // 说明
	};

	static void ensure_defs_loaded();
	static int get_ability_count();
	static const AbilityDef *get_ability(int p_idx);
	static const AbilityDef *find_def(const StringName &p_id);

	AbilityManager();

	void set_cultivation(CultivationSystem *p_cultivation) { _cultivation = p_cultivation; }
	void unlock_ability(const StringName &p_ability_id);
	bool has_ability(const StringName &p_ability_id) const;
	void check_realm_unlocks();
	String get_unlocked_list() const;

	// 全量定义表（能力页/测试用；元素 {id,name,type,innate,unlock_realm,hunyuan,cond,desc}）
	Array get_ability_list() const;

protected:
	static void _bind_methods();

private:
	static std::vector<AbilityDef> s_defs;
	static bool s_defs_loaded;

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
	static String realm_name_of(int p_realm); // 任意境界序号 → 境界名（Boss 血条等用；越界返空）
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
	// 秘境压制修为：Player 通过此字段通知 CultivationSystem 按压制境界算法力上限
	void set_suppressed_realm_for_mana(int v) { _suppressed_realm_for_mana = v; }
	void set_mana_regen_mult(double p_m) { _mana_regen_mult = p_m; }
	// 灵力回复总倍率（功法×被动×聚灵阵等外部乘区组合共用）
	double get_mana_regen_mult() const { return _mana_regen_mult; }
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

	// ---- 元婴分叉：肉身成圣 / 元神修炼（design/cultivation-realms.md 元婴分叉）----
	// 行为喂养双轨（近战系→肉身，法术系→元神），100 经验 1 级共 5 级；
	// 合体「形神合一」弱侧补 80% 差值汇合；focus 称号轴自动跟随高侧。
	static constexpr float PATH_EXP_PER_LEVEL = 100.0f;
	static constexpr int PATH_MAX_LEVEL = 5;
	void feed_path(int p_path, float p_amount); // 0=肉身 1=元神（元婴起）
	int get_path_body_level() const { return MIN(int(_path_body_exp / PATH_EXP_PER_LEVEL), PATH_MAX_LEVEL); }
	int get_path_spirit_level() const { return MIN(int(_path_spirit_exp / PATH_EXP_PER_LEVEL), PATH_MAX_LEVEL); }
	float get_path_body_exp() const { return _path_body_exp; }
	float get_path_spirit_exp() const { return _path_spirit_exp; }
	void set_path_exp(int p_path, float p_exp); // 读档
	bool is_path_merged() const { return _path_merged; }
	void set_path_merged(bool p_merged) { _path_merged = p_merged; }
	float get_path_atk_mult() const { return 1.0f + 0.03f * get_path_body_level(); }    // 肉身：物攻
	float get_path_spell_mult() const { return 1.0f + 0.03f * get_path_spirit_level(); } // 元神：法强
	float get_path_law_mult() const { return 1.0f + 0.05f * get_path_spirit_level(); }   // 元神：法则回复
	float get_path_tribulation_resist() const { return 0.08f * get_path_body_level(); }  // 三灾硬抗减伤（L5=40%）

	float get_damage_multiplier() const;
	float get_defense_multiplier() const;
	float get_speed_multiplier() const;
	double get_max_health() const { return 200.0 * get_defense_multiplier(); }

	// 秘境压制修为：按指定境界的 REALM_STATS 底数计乘区（当前实例 STAGE_FACTOR/分叉乘区照旧）
	float get_damage_multiplier_for_realm(int realm) const;
	float get_defense_multiplier_for_realm(int realm) const;
	float get_speed_multiplier_for_realm(int realm) const;
	double get_max_health_for_realm(int realm) const; // 100 × get_defense_multiplier_for_realm
	double get_max_mana_for_realm(int realm) const;   // 指定境界法力底数 × 现有乘区

	int64_t energy_to_next_realm() const;
	bool is_max_realm() const { return _current_realm >= GOLDEN_IMMORTAL; }

	static int64_t get_realm_cap(Realm p_realm);

	// 境界表外抽（data/realms.json，design/data-externalization.md）：惰性装载，
	// DataLoader 可用走 JSON，否则回退硬编码兜底表（值与 JSON 同步）
	static void ensure_defs_loaded();

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
	// 元婴分叉（肉身/元神双轨经验 + 合体已汇合标记）
	float _path_body_exp = 0.0f;
	float _path_spirit_exp = 0.0f;
	bool _path_merged = false;
	bool _hunyuan = false;
	bool _free_breakthrough = true;
	int _suppressed_realm_for_mana = -1; // 秘境压制修为：get_max_mana 按此境界计底数（-1=不压制）

	void _set_realm_internal(Realm p_realm);
	void _update_focus_from_paths(); // 分叉等级 → focus 称号轴自动跟随
	void _emit_energy_changed();
	void _emit_mana_changed();
	void _emit_law_changed();
	void _notify_name_changed();
};

export class GongfaSystem : public Object {
	GDCLASS(GongfaSystem, Object)

public:
	enum School { SCHOOL_BODY = 0, SCHOOL_QI = 1, SCHOOL_COUNT };
	// 品级四阶：黄/玄/地/天 + 仙（GRADE_XIAN=先天仙品，天生仙品无需飞升晋升，
	// 如孙悟空《大品天仙诀》；后天仙化 = 飞升真仙后 _xian_promoted 全体晋升）
	enum Grade { GRADE_HUANG = 0, GRADE_XUAN, GRADE_DI, GRADE_TIAN, GRADE_XIAN };

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

	// 飞升仙化：玩家首达真仙（realm 10）时 _known/在修功法全体晋升仙品——
	// 品级显示「仙」、名前缀「仙·」、每层效果 ×XIAN_LAYER_MULT（不加层数上限）。
	// 先天仙品（GRADE_XIAN）数值本身即仙品档位，不再吃此晋升加成（防双重叠加）。
	static constexpr float XIAN_LAYER_MULT = 1.5f;
	// 天品功法境界自动领悟门槛：大乘（realm 8）
	static constexpr int TIAN_GRANT_REALM = 8;

	GongfaSystem();

	bool equip_gongfa(const StringName &p_id);
	bool grant_gongfa(const StringName &p_id); // 先天仙品专用通道（斜月三星洞菩提祖师传法）
	void feed(School p_school, float p_base);
	void grant(const StringName &p_id) { equip_gongfa(p_id); }

	bool is_xian_promoted() const { return _xian_promoted; }

	float get_hp_mult() const;
	float get_def_mult() const;
	float get_atk_mult() const;
	float get_mana_mult() const;
	float get_regen_mult() const;
	float get_spell_mult() const;
	float get_speed_mult() const;

	const SlotState &get_slot(School p_s) const { return _slots[p_s]; }
	Dictionary get_slot_info(int p_school) const;
	Dictionary get_def_info(const StringName &p_id) const; // 定义表查询（测试/UI 用）
	float prof_threshold(int p_layer) const { return 100.0f * p_layer; }

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	SlotState _slots[SCHOOL_COUNT];
	HashMap<StringName, std::pair<int, float>> _known;
	bool _xian_promoted = false; // 后天仙化标记（存档 pd["gongfa"].xian_promoted）
	bool _bus_connected = false;
	static std::vector<Def> s_defs;
	static bool s_defs_loaded;

	float _slot_mult(School p_s, float Def::*p_field) const;
	void _feed_slot(School p_s, float p_amount);
	void _ensure_bus_connected(); // 惰性连接 SignalBus realm_changed（GDCLASS Object 无 _ready）
	void _on_realm_changed(int p_old, int p_new, const String &p_name);
	void _maybe_grant_tian(int p_realm); // 大乘起自动领悟天品（入 _known 并换装）
	void _promote_to_xian();             // 幂等
	void _check_current_realm();         // 兜底：读档/老档 realm≥门槛但标记缺失时补触发
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
		float passive_atk = 0.0f;       // 辅助型常驻攻击加成（比例）
		Element resist_elem = ELEM_NONE; // 辅助型常驻元素抗性（ELEM_NONE=无）
		float resist_elem_pct = 0.0f;   // 辅助型常驻元素抗性比例
	};

	static const int MAX_SLOTS = 6;

	// 温养来源常量（SignalBus 解耦，见 _on_* 回调）
	static constexpr float NURTURE_PER_KILL_ELITE_TIER = 2.0f; // 精英击杀：全部已装备 +tier×2
	static constexpr float NURTURE_PER_BOSS = 15.0f;           // Boss 击杀：全部已装备 +15
	static constexpr float NURTURE_PER_PILL = 1.0f;            // 服丹（炼丹产物）：本命 +1/颗
	static constexpr float NURTURE_PER_MEDITATE_TICK = 0.1f;   // 打坐修为：本命 +0.1/次

	static const Def *find_def(const StringName &p_id);
	static String kind_name(Kind p_k);

	// 温养阶段阈值（次要法宝：100%→120%（STAGE1）→150%（STAGE2）→圆满；本命同档位语义）
	static constexpr float NURTURE_STAGE1 = 300.0f;
	static constexpr float NURTURE_STAGE2 = 600.0f;

	ArtifactSystem();

	void set_player(Player *p) { _player = p; }

	bool acquire(const StringName &p_id);
	bool is_owned(const StringName &p_id) const;
	bool equip(int p_slot, const StringName &p_id);

	bool activate_slot(int p_slot);

	void nurture_equipped(float p_amount);
	float get_slot_coeff(int p_slot) const;
	float get_passive_def_bonus() const;
	float get_passive_atk_bonus() const;
	float get_passive_elem_resist(int p_elem) const;

	// 飞升（真仙）：次要法宝槽 2→5（共 6 槽），随档持久化
	void unlock_secondary_slots();
	bool is_secondary_unlocked() const { return _secondary_unlocked; }

	// 渡劫「只带本命法宝」：三灾 arena 内禁用次要法宝祭出与被动（装备加成由 Player 侧置空）
	void set_tribulation_mode(bool p_on);
	bool is_tribulation_mode() const { return _tribulation_mode; }

	int get_slot_limit() const;
	StringName get_slot_artifact(int p_slot) const;
	Dictionary get_slot_info(int p_slot) const;
	Array get_owned_list() const;

	// 温养进度可视化（法宝页/测试）：
	// 返回 { nurture 当前温养值, stage 档位(0/1/2), progress 档内进度 0~1,
	//        next_need 距下一档所需值(满档=0), is_benming, awakened(仅本命) }
	// 本命：stage 0 温养中(120%→150%) / 1 温养圆满待渡劫觉醒 / 2 已觉醒(×2.0 锁定)；
	// 次要：stage 0 →×1.2 / 1 →×1.5 / 2 已圆满。进度按当前档→下一档区间归一。
	Dictionary get_nurture_progress(const StringName &p_id) const;

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
	bool _secondary_unlocked = false; // 飞升解锁次要槽 +3
	bool _tribulation_mode = false;   // 渡劫中：只带本命法宝

	bool _bus_connected = false;
	void _ensure_bus_connected(); // 惰性连接 SignalBus（GDCLASS Object 无 _ready，参考 GongfaSystem）
	// 温养来源回调（全部走 SignalBus 解耦）
	void _on_elite_killed(const Vector2 &p_pos, int p_tier, int p_realm); // 精英：全装备 +tier×2
	void _on_boss_died();                                                 // Boss：全装备 +15
	void _on_item_used(const String &p_item_id, int p_quantity);          // 服丹：本命 +1/颗
	void _on_energy_changed(int64_t p_current, int64_t p_max, double p_progress); // 打坐中：本命 +0.1/次

	double _now() const;
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

	// 机缘事件/秘境进行中（洞天等系统据此互斥）
	bool is_active() const { return _active; }

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

	// ---- 心魔镜像「模拟施法」驱动（session W4-5）：镜像战斗中使用玩家已装配主动技能 ——
	// 所有字段由 BreakthroughManager 经各处 memnew 显式装配，绝不依赖代码内默认值一致。
	struct MirrorCastState {
		double last_cast_at = -1.0e9; // 上次实际施放时刻（可能为 null 的「无施放」状态）
		int last_skill_idx = -1;      // 上次选中的技能在装配列表中的下标（-1=无）
		double cast_ready_at = 0.0;   // 下次施放可用时刻（Player::get_time 时基）
		String last_cast_id;          // 上次实际施放的技能 id（测试断言「尊重玩家冷却」用）
		double last_cast_gap = 0.0;   // 上次实际施放之间的间隔（秒；测试断言 不连发）
		double last_cast_against_time = -1.0e9; // 上次非自伤投射物实际命中玩家的时刻
		bool casting = false;         // 施法前摇进行中（面板计时）
		double cast_until = 0.0;      // 前摇结束时刻
		String casting_id;            // 前摇中的技能 id
		bool melee_cast = false;      // 前摇结束时改走近战 HitBox 路径（武技/无投射物）
		float facing = -1.0f;         // 前摇锁定朝向（施放方向）
		bool manual_cd = false;       // 人工冷却倒计时激活（无投射物技能防连发兜底）
		double manual_cd_until = 0.0; // 人工冷却结束时刻
	};

	// ---- 测试探针：镜像模拟施法诊断（scripts/test_heart_demon_clone.gd 读取）----
	Dictionary mirror_cast_diagnostics() const;
	// 镜像已装配主动技能 id 列表（稳定顺序；测试断言尊重玩家装配）
	Array mirror_equipped_skill_ids() const;

	bool _active = false;
	int _event_id = -1;
	Phase _phase = Phase::IDLE;
	EventDef _def;

	bool _wave_check_pending = false;
	bool _fail_pending = false;

	// 心魔镜像模拟施法（仅 COMBAT 秘境使用；Entry 无内建数据，各字段全在本文件 memnew 装配）
	MirrorCastState *_mirror = nullptr;

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
	void _tick_mirror_casting(double p_delta);
	void _start_mirror_cast(SkillSystem::Def const *p_def, int p_skill_idx, float p_facing);
	void _deliver_mirror_cast();
	void _deliver_player_projectile(SkillSystem::Def const *p_def, const Vector2 &p_dir, float p_damage);
	void _deliver_mirror_melee(DamageCategory p_cat, Element p_elem, float p_raw_damage);
	void _mirror_proj_hit_player(Node *p_proj, Node *p_player);
	void _clear_mirror();
	Enemy *_mirror_enemy() const;
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

	// 渡劫 v2（session 019）：三灾齐至——雷/火/风全程并发，不再分阶段连考；
	// 天罚使（劫云化身 Boss）代天行罚，斩之即渡劫成（boss_died → tribulation_finished）。
	// 双过法（元婴分叉联动）：肉身等级→三灾伤害减免（硬抗道）；
	// 元神等级→雷预警延长 + 风反转概率减免 + 阴火减免（躲避道）。

public:
	TribulationController();

	void start_tribulation(Player *p_player, const Rect2 &p_arena, Node *p_arena_node);
	void abort();

	void _process(double p_delta) override;

	// 测试探针
	bool is_boss_alive() const { return _boss != nullptr && !_aborted; }

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

	static constexpr double THUNDER_INTERVAL = 2.2;          // 落雷间隔
	static constexpr double THUNDER_INTERVAL_ENRAGED = 1.4;  // 天罚使半血后
	static constexpr double THUNDER_WARN = 1.0;              // 预警（元神每级 +15%）
	static constexpr float THUNDER_HIT_HALF_W = 14.0f;
	static constexpr float THUNDER_DMG_FRAC = 0.12f;         // 最大生命比例（肉身可减免）
	static constexpr double FIRE_TICK = 1.0;
	static constexpr float FIRE_DMG_FRAC = 0.015f;
	static constexpr double GUST_INTERVAL = 2.5;
	static constexpr double GUST_INTERVAL_ENRAGED = 1.8;
	static constexpr float GUST_FORCE = 130.0f;
	static constexpr float WIND_ERODE_FRAC = 0.006f;
	static constexpr double WIND_ERODE_TICK = 0.5;
	static constexpr float BOSS_HP = 2500.0f;

	Player *_player = nullptr;
	Node *_arena_node = nullptr;
	Enemy *_boss = nullptr; // 天罚使
	Rect2 _arena;
	double _time = 0.0;
	bool _aborted = false;
	bool _enraged = false; // Boss 半血：三灾加剧

	double _next_strike_at = 1.0;
	std::vector<LightningBolt> _bolts;
	double _dot_accum = 0.0;
	double _gust_timer = 0.0;
	Vector2 _gust_dir = Vector2(1, 0);
	double _erode_accum = 0.0;

	CanvasLayer *_ui = nullptr;
	Label *_title_label = nullptr;

	void _spawn_boss();
	void _on_boss_died();
	void _spawn_bolt();
	void _update_bolts(double p_delta);
	void _update_fire(double p_delta);
	void _update_wind(double p_delta);
	float _body_resist() const;   // 肉身减伤（硬抗过法）
	double _thunder_warn() const; // 元神预警延长（躲避道）
	float _fire_resist() const;   // 阴火减免（肉身主、元神辅）
	void _update_title();
	void _create_ui();
	void _clear_bolts();
	void _restore_player_effects();
	void _finish();
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
