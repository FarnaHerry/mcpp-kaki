#ifndef CPP_KAKI_ITEM_PICKUP_H
#define CPP_KAKI_ITEM_PICKUP_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

class Node2D;

// A world-placed item that the player can walk over to collect.
// On body_entered (Player body), adds the item to the player's inventory.
//
// Usage from GDScript:
//   var pickup = ClassDB.instantiate("ItemPickup")
//   pickup.item_id = "healing_pill"
//   pickup.quantity = 1
//   pickup.position = Vector2(300, 220)
//   add_child(pickup)
class ItemPickup : public Area2D {
	GDCLASS(ItemPickup, Area2D);

public:
	// ---- Configuration (set before adding to scene) ----
	void set_item_id(const StringName &p_id) { _item_id = p_id; }
	StringName get_item_id() const { return _item_id; }

	void set_quantity(int p_qty) { _quantity = p_qty; }
	int get_quantity() const { return _quantity; }

	void _ready() override;
	void _on_body_entered(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	StringName _item_id;
	int _quantity = 1;

	void _create_visual();
};

} // namespace godot

#endif // CPP_KAKI_ITEM_PICKUP_H
