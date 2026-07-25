#pragma once

#include <vector>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

// 功法系统（design/gongfa-skills.md 第二节，已定稿）：
//   - 两系：炼体（生命/防御/物攻）/ 练气（灵力/回灵/法术强度/速度）
//   - 最多同修 1 炼体 + 1 练气；切换保留旧功法熟练
//   - 熟练度喂养：行为两系都加，主系 100% / 副系 20%（不偏科）
//   - 加成乘区：(1+功法) 括号内按层加法，与其他乘区相乘
class GongfaSystem : public Object {
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
		// 每层加成（比例，炼体系用前三个，练气系用后四个）
		float hp, def, atk;             // 炼体：生命/防御/物攻
		float mana, regen, spell, spd;  // 练气：灵力/回灵/法强/速度
	};

	struct SlotState {
		StringName id;
		int layer = 1;
		float prof = 0.0f; // 当前层熟练进度
		bool empty() const { return id == StringName(); }
	};

	// 信号：gongfa_changed（层数/装配变化，Player 刷新面板用）

	static const Def *find_def(const StringName &p_id);
	static void ensure_defs_loaded();
	static String grade_name(Grade p_g);

	// 装配（自动按系入槽；旧功法熟练保留在 _known）
	bool equip_gongfa(const StringName &p_id);
	// 熟练喂养：school 系行为产生 base 量；主系槽 +100%，副系槽 +20%
	void feed(School p_school, float p_base);
	// 直接习得并装配（机缘/初始奖励）
	void grant(const StringName &p_id) { equip_gongfa(p_id); }

	// 乘区（1 + Σ 每层×层数）
	float get_hp_mult() const;
	float get_def_mult() const;
	float get_atk_mult() const;    // 物理攻击
	float get_mana_mult() const;
	float get_regen_mult() const;
	float get_spell_mult() const;  // 法术强度（技能系统消费）
	float get_speed_mult() const;

	const SlotState &get_slot(School p_s) const { return _slots[p_s]; }
	// 绑定给脚本/UI: {id, name, grade_name, layer, max_layer, prof, threshold}
	Dictionary get_slot_info(int p_school) const;
	float prof_threshold(int p_layer) const { return 100.0f * p_layer; } // 每层所需熟练

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	SlotState _slots[SCHOOL_COUNT];
	// 换下的功法熟练保留：id → (layer, prof)
	HashMap<StringName, std::pair<int, float>> _known;
	static std::vector<Def> s_defs;
	static bool s_defs_loaded;

	float _slot_mult(School p_s, float Def::*p_field) const;
	void _feed_slot(School p_s, float p_amount);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::GongfaSystem::School);
VARIANT_ENUM_CAST(godot::GongfaSystem::Grade);
