#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "damage_types.h"

namespace godot {

class Player;

// 技能系统（design/gongfa-skills.md 第三节，已定稿）：
//   - 武技/法术统一为 Skill（id/类型/伤害类别/元素/消耗/冷却/效果），
//     神通/仙法后续复用同一套数据（第3、4阶只是解锁条件和消耗源不同）
//   - 武技：物理伤害，冷却驱动不耗灵力，凡人可用
//   - 法术：法术/元素伤害，耗灵力+冷却，炼气解锁
//   - 槽位 L1 单技能槽（v1）：A/S=武技槽 D/F=法术槽 G/H=法宝槽（步骤4）
class SkillSystem : public Object {
	GDCLASS(SkillSystem, Object)

public:
	enum SkillType {
		TYPE_MARTIAL = 0, // 武技（物理）
		TYPE_SPELL,       // 法术（灵力驱动）
		TYPE_SHENTONG,    // 神通（法则之力，步骤5）
		TYPE_XIANFA,      // 仙法（仙元，步骤5）
	};

	enum EffectKind {
		FX_MELEE_SWING = 0, // 强化一挥（借 Player HitBox）
		FX_LUNGE,           // 突进 + 一挥
		FX_PROJECTILE,      // 投射物
	};

	struct Def {
		const char *id;
		const char *name;
		SkillType type;
		DamageCategory category;
		Element element;
		float mana_cost;
		float cooldown;
		float power;      // × 攻击面板（法术另乘功法法强）
		EffectKind effect;
		int min_realm;    // 解锁最低境界（CultivationSystem::Realm）
		float proj_speed; // FX_PROJECTILE
		Color proj_color; // FX_PROJECTILE
	};

	static const int SLOT_COUNT = 6; // A/S=武技 D/F=法术 G/H=法宝(预留)
	// 槽位许可的技能类型（G/H 由法宝系统步骤4接管，此处记 TYPE_SHENTONG 占位不可装配）
	static SkillType slot_type(int p_slot) {
		switch (p_slot) {
			case 0: case 1: return TYPE_MARTIAL;
			case 2: case 3: return TYPE_SPELL;
			default: return TYPE_SHENTONG;
		}
	}

	static const Def *find_def(const StringName &p_id);
	static String type_name(SkillType p_t);

	void set_player(Player *p) { _player = p; }

	// 学习 / 装配（装配校验槽位类型；重复学习幂等）
	bool learn(const StringName &p_id);
	bool is_known(const StringName &p_id) const;
	bool assign(int p_slot, const StringName &p_id);

	// 施放：检查 已学/境界/冷却/灵力 → 执行效果（经 Player）→ 喂养功法
	bool cast_slot(int p_slot);

	double get_cooldown_remaining(const StringName &p_id) const;
	StringName get_slot_skill(int p_slot) const;
	// 绑定给 HUD/菜单: {id,name,type,type_name,cd_remaining,cooldown,mana_cost,castable}
	Dictionary get_slot_info(int p_slot) const;
	// 已学技能 id 列表（菜单用）
	Array get_known_list() const;

	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

	// 信号: skill_cast(id) / skills_changed（学习或装配变化）
protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	HashSet<StringName> _known;
	StringName _slots[SLOT_COUNT];
	HashMap<StringName, double> _cooldown_until; // 冷却截止（Player._time 时基，随暂停冻结）

	double _now() const;
	void _execute(const Def *p_def);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::SkillSystem::SkillType);
