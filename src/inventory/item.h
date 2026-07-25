#ifndef CPP_KAKI_ITEM_H
#define CPP_KAKI_ITEM_H

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

// Data-only item definition. Items are registered in ItemDatabase.
// This is a plain struct, not a Godot class — no _bind_methods needed.
struct Item {
	enum Type {
		CONSUMABLE,  // Usable from inventory (pills, elixirs)
		MATERIAL,    // Crafting/currency (spirit stones, ore)
		KEY_ITEM,    // Quest/progression items
		EQUIPMENT,   // Equippable: weapons, armor, accessories
	};

	enum EquipSlot {
		SLOT_NONE = -1,
		SLOT_WEAPON = 0,
		SLOT_ARMOR = 1,
		SLOT_ACCESSORY = 2,
	};

	StringName id;
	String name;
	String description;
	Type type = CONSUMABLE;
	int max_stack = 99;
	EquipSlot equip_slot = SLOT_NONE;

	// ---- Consumable effects (only relevant for CONSUMABLE type) ----
	float heal_amount = 0.0f;
	float heal_pct = 0.0f;      // 按比例回血（0.5 = 50%）
	float mana_amount = 0.0f;   // 回灵力（法力池）
	float energy_amount = 0.0f; // 修为经验（accumulate_energy，到顶卡境界）
	StringName buff_id;         // 增益 buff（BuffSystem def id）
	int grade = 0;              // 品级：0凡 1灵 2地 3天
	float breakthrough_bonus = 0.0f; // 机缘突破事件的加成（事件系统实现后生效）

	// ---- Equipment bonuses (only relevant for EQUIPMENT type) ----
	float attack_bonus = 0.0f;   // flat damage added
	float defense_bonus = 0.0f;  // flat damage reduction
	float speed_bonus = 0.0f;    // multiplier on move speed (0.05 = +5%)
};

} // namespace godot

#endif // CPP_KAKI_ITEM_H
