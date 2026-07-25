#pragma once

#include <vector>

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

// 宗门系统（design/sect-pressure.md，已定稿）：凡间拜师修炼体系。
// 与 CultivationSystem 的「门派轴」（大罗/太乙/散仙，真仙后称号，无数值）是两个概念。
//   - 四宗一散：蜀山(攻)/昆仑(灵力+回灵)/蓬莱(生命+防)/魔罗(击杀修为+攻)
//   - 职位：外门(0)/内门(贡献100)/真传(贡献300)，加成档位随职位升
//   - 贡献：玩家击杀 +1，Boss +10（Player::_on_enemy_killed 喂入）
//   - 拜师门槛：炼气；叛门自由、贡献清零、已学专属技保留
//   - 乘区钩子（Player 各结算点消费）：攻/灵力上限/回灵/生命/防御/击杀修为
class SectSystem : public Object {
	GDCLASS(SectSystem, Object)

public:
	enum Rank { RANK_OUTER = 0, RANK_INNER = 1, RANK_TRUE = 2, RANK_COUNT };

	struct Def {
		const char *id;
		const char *name;
		const char *desc;         // 宗旨
		const char *skill_id;     // 入门专属技（SkillSystem def id）
		float atk[RANK_COUNT];    // 攻击加值（0.06 = +6%）
		float mana[RANK_COUNT];   // 灵力上限加值
		float regen[RANK_COUNT];  // 回灵加值
		float hp[RANK_COUNT];     // 生命加值
		float def[RANK_COUNT];    // 防御加值
		float kill_xp[RANK_COUNT];// 击杀修为加值
	};

	static const Def *find_def(const StringName &p_id);
	static void ensure_defs_loaded();
	static String rank_name(int p_rank);

	bool in_sect() const { return _sect_id != StringName(); }
	StringName get_sect_id() const { return _sect_id; }
	int get_contribution() const { return _contribution; }
	int get_rank() const;              // 未入门返回 -1
	String get_rank_name() const;

	bool can_join(const StringName &p_id, int p_realm) const; // 炼气起 + 未入门 + 宗门存在
	bool join(const StringName &p_id, int p_realm);
	void leave(); // 叛门：贡献清零（已学技能保留——逐出师门不夺修为）
	void on_kill(bool p_boss);        // 贡献 +1/+10

	// 乘区钩子（未入门一律 1.0）
	float get_atk_mult() const;
	float get_mana_mult() const;
	float get_regen_mult() const;
	float get_hp_mult() const;
	float get_def_mult() const;
	float get_kill_xp_mult() const;

	Array get_sect_list() const;      // 云游/菜单数据源：[{id,name,desc,skill_id,joined}]
	Dictionary get_sect_info() const; // 当前宗门 {id,name,desc,rank,rank_name,contribution,next_rank_cost}
	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	StringName _sect_id;   // 空 = 散修
	static std::vector<Def> s_defs;
	static bool s_defs_loaded;
	int _contribution = 0;
};

} // namespace godot
