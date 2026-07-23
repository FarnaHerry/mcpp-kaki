#ifndef CPP_KAKI_INVENTORY_H
#define CPP_KAKI_INVENTORY_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

struct Item;

// Slot-based inventory container. Owned by Player.
// Each slot holds an item ID and a stack quantity.
class Inventory : public Object {
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

	// Add items to the inventory. Stacks with existing slots first,
	// then fills empty slots. Returns false if inventory is full.
	bool add_item(const StringName &p_id, int p_qty = 1);

	// Remove items from the inventory. Returns false if insufficient quantity.
	bool remove_item(const StringName &p_id, int p_qty = 1);

	// Count total quantity of an item across all slots.
	int get_item_count(const StringName &p_id) const;

	// Check if the inventory contains at least one of an item.
	bool has_item(const StringName &p_id) const;

	// Use a consumable from the given slot. Returns false if the slot
	// is empty, the item is not a consumable, or the effect can't apply.
	// The player is responsible for applying the effect.
	bool use_item(int p_slot_index);

	// Number of empty slots.
	int get_free_slot_count() const;

	// Get slot data as a Dictionary: {"id": StringName, "quantity": int}
	// Returns empty dict for empty slots.
	Dictionary get_slot(int p_idx) const;

	// Clear all slots.
	void clear();

	// Set slot directly (for save/load). Bounds-checked.
	void set_slot(int p_idx, const StringName &p_id, int p_qty);

	// ---- Signals (local, not on SignalBus) ----
	// inventory_changed() — emitted when slots change

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

} // namespace godot

#endif // CPP_KAKI_INVENTORY_H
