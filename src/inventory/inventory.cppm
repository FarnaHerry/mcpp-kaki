// mcpp-kaki inventory module — Item / Inventory / ItemDatabase.
module;

#include <vector>

#include <godot-cpp-m/macros.h>
#include <godot_cpp/templates/hash_map.hpp> // HashMap 不被模块重导出，保持文本包含
#include <godot_cpp/templates/vector.hpp>

export module mcpp_kaki.inventory;

import godot_cpp;

namespace godot {

// Data-only item definition. Items are registered in ItemDatabase.
// This is a plain struct, not a Godot class — no _bind_methods needed.
export struct Item {
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
	float fullness_amount = 0.0f; // 回饱食度（食物类，随境界倍率：凡人1.0/炼气1.2；辟谷后转纯 buff）
	StringName buff_id;         // 增益 buff（BuffSystem def id）
	StringName learn_skill;     // 使用后习得技能（SkillSystem def id，秘籍/残卷类）
	int grade = 0;              // 品级：0凡 1灵 2地 3天
	float breakthrough_bonus = 0.0f; // 机缘突破事件的加成（事件系统实现后生效）
	bool plantable = false;     // 可种植（草药类，洞天内灵田播种）
	int grow_seconds = 0;       // 成熟所需现实秒数（plantable 时有效）

	// ---- Equipment bonuses (only relevant for EQUIPMENT type) ----
	float attack_bonus = 0.0f;   // flat damage added
	float defense_bonus = 0.0f;  // flat damage reduction
	float speed_bonus = 0.0f;    // multiplier on move speed (0.05 = +5%)
};

// Slot-based inventory container. Owned by Player.
export class Inventory : public Object {
	GDCLASS(Inventory, Object);

public:
	// 凡人背包有限（24格）；炼气事件解锁纳戒/储物袋后容量无限（UNLIMITED_SLOTS）
	static constexpr int DEFAULT_SLOTS = 24;
	static constexpr int UNLIMITED_SLOTS = 999;

	Inventory();
	~Inventory();

	// 纳戒解锁：容量 24 → 无限
	void unlock_unlimited() { _capacity = UNLIMITED_SLOTS; }
	bool is_unlimited() const { return _capacity > DEFAULT_SLOTS; }
	int get_capacity() const { return _capacity; }

	bool add_item(const StringName &p_id, int p_qty = 1);
	bool remove_item(const StringName &p_id, int p_qty = 1);
	int get_item_count(const StringName &p_id) const;
	bool has_item(const StringName &p_id) const;
	bool use_item(int p_slot_index);
	int get_free_slot_count() const;
	Dictionary get_slot(int p_idx) const;
	void clear();
	void set_slot(int p_idx, const StringName &p_id, int p_qty);

protected:
	static void _bind_methods();

private:
	struct Slot {
		StringName item_id;
		int quantity = 0;

		bool is_empty() const { return quantity <= 0 || item_id == StringName(); }
		void clear() {
			item_id = StringName();
			quantity = 0;
		}
	};

	Slot _slots[UNLIMITED_SLOTS];
	int _capacity = DEFAULT_SLOTS;

	int _find_stackable_slot(const StringName &p_id) const;
	int _find_empty_slot() const;
	void _emit_changed();
};

// Singleton registry of all item definitions in the game.
export class ItemDatabase : public Node {
	GDCLASS(ItemDatabase, Node);

public:
	static ItemDatabase *get_singleton() { return _singleton; }

	ItemDatabase();
	~ItemDatabase();

	const Item *get_item(const StringName &p_id) const;
	int get_item_count() const { return _items.size(); }
	bool has_item(const StringName &p_id) const { return _items.has(p_id); }
	Dictionary get_item_info(const StringName &p_id) const;

	// 所有可播种草药 id（品级升序：凡→灵→地）——洞天灵田播种选种用
	std::vector<StringName> get_plantable_ids() const;

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static ItemDatabase *_singleton;

	HashMap<StringName, Item> _items;

	void _register_items();
};

} // namespace godot
