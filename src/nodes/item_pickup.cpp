#include "item_pickup.h"

#include "../inventory/item.h"
#include "../inventory/item_database.h"
#include "../utils/signal_bus.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void ItemPickup::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_item_id", "id"), &ItemPickup::set_item_id);
	ClassDB::bind_method(D_METHOD("get_item_id"), &ItemPickup::get_item_id);
	ClassDB::bind_method(D_METHOD("set_quantity", "qty"), &ItemPickup::set_quantity);
	ClassDB::bind_method(D_METHOD("get_quantity"), &ItemPickup::get_quantity);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &ItemPickup::_on_body_entered);

	// 必须注册为属性——bootstrap 用 pickup.set("item_id", ...) 赋值
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "item_id"), "set_item_id", "get_item_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity"), "set_quantity", "get_quantity");
}

void ItemPickup::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	// Detect Player body (layer 3) — USE body_entered not area_entered
	set_collision_layer_value(1, false); // Not on any body layer
	set_collision_mask_value(3, true);   // Detect Player body layer
	// Deferred: DropSystem may spawn pickups during physics callbacks
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));

	// Create collision shape (small pickup radius)
	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(20, 20));
	shape->set_shape(rect);
	add_child(shape);

	_create_visual();
}

void ItemPickup::_create_visual() {
	const Item *def = ItemDatabase::get_singleton()->get_item(_item_id);

	// Color by item type
	Color pick_color;
	switch (def ? def->type : Item::CONSUMABLE) {
		case Item::CONSUMABLE:
			pick_color = Color(1.0f, 0.3f, 0.3f, 0.7f); // Red for consumables
			break;
		case Item::MATERIAL:
			pick_color = Color(0.3f, 0.7f, 1.0f, 0.7f); // Blue for materials
			break;
		case Item::KEY_ITEM:
			pick_color = Color(1.0f, 0.85f, 0.3f, 0.7f); // Gold for key items
			break;
	}

	// Diamond shape for pickup
	Polygon2D *visual = memnew(Polygon2D);
	visual->set_name("PickupVisual");
	visual->set_color(pick_color);
	PackedVector2Array diamond;
	diamond.append(Vector2(0, -8));
	diamond.append(Vector2(6, 0));
	diamond.append(Vector2(0, 8));
	diamond.append(Vector2(-6, 0));
	visual->set_polygon(diamond);
	add_child(visual);
}

void ItemPickup::_on_body_entered(Node2D *p_body) {
	if (!p_body || p_body->get_name() != StringName("Player"))
		return;

	// Check that the item exists
	const Item *def = ItemDatabase::get_singleton()->get_item(_item_id);
	if (!def)
		return;

	// Call pickup_item on the player
	p_body->call("pickup_item", _item_id, _quantity);

	// Remove from world
	queue_free();
}

} // namespace godot
