module;

#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.inventory;

namespace godot {

Inventory::Inventory() {
	for (int i = 0; i < _capacity; i++) {
		_slots[i].clear();
	}
}

Inventory::~Inventory() {}

void Inventory::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_item", "id", "qty"), &Inventory::add_item, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("remove_item", "id", "qty"), &Inventory::remove_item, DEFVAL(1));
	ClassDB::bind_method(D_METHOD("get_item_count", "id"), &Inventory::get_item_count);
	ClassDB::bind_method(D_METHOD("has_item", "id"), &Inventory::has_item);
	ClassDB::bind_method(D_METHOD("use_item", "slot_index"), &Inventory::use_item);
	ClassDB::bind_method(D_METHOD("get_free_slot_count"), &Inventory::get_free_slot_count);
	ClassDB::bind_method(D_METHOD("get_slot", "idx"), &Inventory::get_slot);
	ClassDB::bind_method(D_METHOD("clear"), &Inventory::clear);
	ClassDB::bind_method(D_METHOD("set_slot", "idx", "id", "qty"), &Inventory::set_slot);
	ClassDB::bind_method(D_METHOD("unlock_unlimited"), &Inventory::unlock_unlimited);
	ClassDB::bind_method(D_METHOD("is_unlimited"), &Inventory::is_unlimited);
	ClassDB::bind_method(D_METHOD("get_capacity"), &Inventory::get_capacity);

	ADD_SIGNAL(MethodInfo("inventory_changed"));
}

bool Inventory::add_item(const StringName &p_id, int p_qty) {
	if (p_qty <= 0) {
		return false;
	}

	// Check that the item exists in the database
	const Item *def = ItemDatabase::get_singleton()->get_item(p_id);
	if (!def) {
		return false;
	}

	int remaining = p_qty;
	int max_per_stack = def->max_stack;

	// First, try to stack into existing slots of the same item
	int stack_slot = _find_stackable_slot(p_id);
	while (stack_slot >= 0 && remaining > 0) {
		int can_add = max_per_stack - _slots[stack_slot].quantity;
		int to_add = (remaining < can_add) ? remaining : can_add;
		_slots[stack_slot].quantity += to_add;
		remaining -= to_add;
		if (remaining > 0) {
			stack_slot = _find_stackable_slot(p_id);
		}
	}

	// Then, fill empty slots
	while (remaining > 0) {
		int empty = _find_empty_slot();
		if (empty < 0) {
			break; // Inventory full
		}
		int to_add = (remaining < max_per_stack) ? remaining : max_per_stack;
		_slots[empty].item_id = p_id;
		_slots[empty].quantity = to_add;
		remaining -= to_add;
	}

	_emit_changed();
	return remaining == 0;
}

bool Inventory::remove_item(const StringName &p_id, int p_qty) {
	if (p_qty <= 0) {
		return false;
	}

	int total = get_item_count(p_id);
	if (total < p_qty) {
		return false;
	}

	int remaining = p_qty;
	for (int i = 0; i < _capacity && remaining > 0; i++) {
		if (_slots[i].item_id == p_id) {
			int to_remove = (remaining < _slots[i].quantity) ? remaining : _slots[i].quantity;
			_slots[i].quantity -= to_remove;
			remaining -= to_remove;
			if (_slots[i].quantity <= 0) {
				_slots[i].clear();
			}
		}
	}

	_emit_changed();
	return true;
}

int Inventory::get_item_count(const StringName &p_id) const {
	int total = 0;
	for (int i = 0; i < _capacity; i++) {
		if (_slots[i].item_id == p_id) {
			total += _slots[i].quantity;
		}
	}
	return total;
}

bool Inventory::has_item(const StringName &p_id) const {
	return get_item_count(p_id) > 0;
}

bool Inventory::use_item(int p_slot_index) {
	if (p_slot_index < 0 || p_slot_index >= _capacity) {
		return false;
	}

	Slot &slot = _slots[p_slot_index];
	if (slot.is_empty()) {
		return false;
	}

	const Item *def = ItemDatabase::get_singleton()->get_item(slot.item_id);
	if (!def || def->type != Item::CONSUMABLE) {
		return false;
	}

	// Remove one from the slot
	slot.quantity--;
	if (slot.quantity <= 0) {
		slot.clear();
	}

	_emit_changed();
	return true;
}

int Inventory::get_free_slot_count() const {
	int count = 0;
	for (int i = 0; i < _capacity; i++) {
		if (_slots[i].is_empty()) {
			count++;
		}
	}
	return count;
}

Dictionary Inventory::get_slot(int p_idx) const {
	Dictionary d;
	if (p_idx < 0 || p_idx >= _capacity) {
		return d;
	}

	if (!_slots[p_idx].is_empty()) {
		d["id"] = _slots[p_idx].item_id;
		d["quantity"] = _slots[p_idx].quantity;
	}
	return d;
}

void Inventory::clear() {
	for (int i = 0; i < _capacity; i++) {
		_slots[i].clear();
	}
	_emit_changed();
}

void Inventory::set_slot(int p_idx, const StringName &p_id, int p_qty) {
	if (p_idx < 0 || p_idx >= _capacity) {
		return;
	}
	_slots[p_idx].item_id = p_id;
	_slots[p_idx].quantity = p_qty;
	_emit_changed();
}

int Inventory::_find_stackable_slot(const StringName &p_id) const {
	const Item *def = ItemDatabase::get_singleton()->get_item(p_id);
	if (!def) {
		return -1;
	}
	int max_per_stack = def->max_stack;

	for (int i = 0; i < _capacity; i++) {
		if (_slots[i].item_id == p_id && _slots[i].quantity < max_per_stack) {
			return i;
		}
	}
	return -1;
}

int Inventory::_find_empty_slot() const {
	for (int i = 0; i < _capacity; i++) {
		if (_slots[i].is_empty()) {
			return i;
		}
	}
	return -1;
}

void Inventory::_emit_changed() {
	emit_signal("inventory_changed");
}

} // namespace godot
