#include "item_pickup.h"
#include "../nodes/player.h"


#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.cultivation;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;
namespace godot {

// 纳戒磁吸基础参数（随境界缩放：base × (1 + realm × 0.3)）
static constexpr float MAGNET_RANGE_BASE = 120.0f;
static constexpr float MAGNET_ACCEL_BASE = 60.0f;
static constexpr float MAGNET_MAX_SPEED_BASE = 150.0f;

static float _magnet_mult(int p_realm) {
	return 1.0f + float(p_realm) * 0.3f; // 炼气 1.3x → 天尊 4.6x
}

void ItemPickup::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_item_id", "id"), &ItemPickup::set_item_id);
	ClassDB::bind_method(D_METHOD("get_item_id"), &ItemPickup::get_item_id);
	ClassDB::bind_method(D_METHOD("set_quantity", "qty"), &ItemPickup::set_quantity);
	ClassDB::bind_method(D_METHOD("get_quantity"), &ItemPickup::get_quantity);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &ItemPickup::_on_body_entered);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "item_id"), "set_item_id", "get_item_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity"), "set_quantity", "get_quantity");
}

void ItemPickup::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_collision_layer_value(1, false);
	set_collision_mask_value(3, true);
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));

	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(20, 20));
	shape->set_shape(rect);
	add_child(shape);

	_create_visual();
}

void ItemPickup::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	// Lazy-find player once
	if (!_player_checked) {
		_player_checked = true;
		Node *scene = get_tree()->get_current_scene();
		if (scene) {
			_player_cache = Object::cast_to<Node2D>(scene->find_child("Player", true, false));
		}
	}
	if (!_player_cache)
		return;

	// Check 纳戒 unlocked
	Object *am = _player_cache->call("get_ability_manager");
	if (!am)
		return;
	bool has_ring = am->call("has_ability", StringName(AbilityManager::ABILITY_STORAGE_RING));
	if (!has_ring)
		return;

	// Magnet pull toward player (speed scales with realm)
	Vector2 to_player = _player_cache->get_global_position() - get_global_position();
	float dist = to_player.length();

	// Get realm for scaling
	int realm = 1;
	Object *cult = _player_cache->call("get_cultivation");
	if (cult) realm = cult->call("get_realm_index");
	float mult = _magnet_mult(realm);
	float range = MAGNET_RANGE_BASE * mult;
	float accel = MAGNET_ACCEL_BASE * mult;
	float max_spd = MAGNET_MAX_SPEED_BASE * mult;

	if (dist > range || dist < 4.0f)
		return;

	Vector2 dir = to_player.normalized();
	_magnet_speed += accel * float(p_delta);
	if (_magnet_speed > max_spd)
		_magnet_speed = max_spd;

	Vector2 vel = dir * _magnet_speed;
	set_global_position(get_global_position() + vel * float(p_delta));
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
