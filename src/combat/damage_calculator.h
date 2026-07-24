#pragma once

#include "damage_types.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

// 一次伤害的完整描述（攻击方）
struct DamageInfo {
	float base_amount = 0.0f;                  // 攻击方面板 × 技能倍率后的量
	DamageCategory category = DMG_PHYSICAL;
	Element element = ELEM_NONE;               // DMG_ELEMENTAL 时有效
};

// 防御方抗性剖面
struct DefenseProfile {
	float defense = 0.0f;                          // 物理防御（平减）
	float spell_resist = 0.0f;                     // 法术抗性（比例 0~0.9）
	float elem_resist[ELEM_CAPACITY] = {};         // 元素抗性（比例 0~1）
	Element self_element = ELEM_NONE;              // 自身五行属性（被克制判定）
};

// 伤害结算唯一入口（design/gongfa-skills.md 第六节，已敲定）。
// 减伤三线、五行克制、后续暴击/穿透/法宝系数——都只往这个类里加。
class DamageCalculator {
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

} // namespace godot
