#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <vector>

#include "../combat/damage_types.h"

namespace godot {

// Buff 系统（design/alchemy.md 第三节，已定稿）：
//   - 丹药/食物/状态统一为 Buff：计时到期自动消失，同名刷新时间不叠加
//   - 属性走乘区钩子：攻击/防御乘区 + 元素抗性加值（DamageCalculator 口径）
//   - 丹毒预留：v1 不做，字段留位
class BuffSystem : public Object {
	GDCLASS(BuffSystem, Object)

public:
	struct Def {
		const char *id;
		const char *name;
		float duration;      // 秒
		float atk_mult;      // 攻击乘区加值（0.15 = +15%）
		float def_mult;      // 防御乘区加值
		Element elem;        // 元素抗性目标（ELEM_NONE = 无）
		float elem_resist;   // 元素抗性加值（0.15 = +15%）
	};

	struct Active {
		StringName id;
		float remaining = 0.0f;
	};

	static const Def *find_def(const StringName &p_id);

	// 施加/刷新：同名只刷新时间，不叠加
	bool apply(const StringName &p_id);
	void remove(const StringName &p_id);
	void clear();
	void tick(double p_delta);

	bool has(const StringName &p_id) const;
	// 乘区钩子（Player 攻/防结算处消费）
	float get_atk_mult() const { return 1.0f + _sum_atk; }
	float get_def_mult() const { return 1.0f + _sum_def; }
	float get_elem_resist_bonus(int p_elem) const;

	// HUD/存档
	Array get_active_list() const; // [{id,name,remaining}]
	Dictionary save_to_dict() const;
	void load_from_dict(const Dictionary &p_data);

protected:
	static void _bind_methods();

private:
	std::vector<Active> _active;
	float _sum_atk = 0.0f;
	float _sum_def = 0.0f;
	float _sum_elem[ELEM_CAPACITY] = {};

	void _recalc();
	void _emit_changed();
};

} // namespace godot
