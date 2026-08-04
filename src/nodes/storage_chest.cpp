#include "storage_chest.h"
#include "player.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.utils; // SignalBus

namespace godot {

void StorageChest::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &StorageChest::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &StorageChest::_on_body_exited);
}

void StorageChest::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	// Detect Player body (layer 3)
	set_collision_layer_value(1, false);
	set_collision_mask_value(3, true);
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));
	connect("body_exited", Callable(this, "_on_body_exited"));

	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(26, 22));
	shape->set_shape(rect);
	add_child(shape);

	// 视觉：木箱（身 + 盖 + 锁扣）
	Polygon2D *box = memnew(Polygon2D);
	box->set_color(Color(0.42f, 0.29f, 0.16f, 1.0f));
	PackedVector2Array body;
	body.append(Vector2(-10, -10));
	body.append(Vector2(10, -10));
	body.append(Vector2(10, 10));
	body.append(Vector2(-10, 10));
	box->set_polygon(body);
	add_child(box);

	Polygon2D *lid = memnew(Polygon2D);
	lid->set_color(Color(0.55f, 0.4f, 0.22f, 1.0f));
	PackedVector2Array lid_poly;
	lid_poly.append(Vector2(-10, -10));
	lid_poly.append(Vector2(10, -10));
	lid_poly.append(Vector2(7, -14));
	lid_poly.append(Vector2(-7, -14));
	lid->set_polygon(lid_poly);
	add_child(lid);

	Polygon2D *lock = memnew(Polygon2D);
	lock->set_color(Color(0.85f, 0.7f, 0.3f, 1.0f));
	PackedVector2Array lock_poly;
	lock_poly.append(Vector2(-2, -2));
	lock_poly.append(Vector2(2, -2));
	lock_poly.append(Vector2(2, 2));
	lock_poly.append(Vector2(-2, 2));
	lock->set_polygon(lock_poly);
	add_child(lock);

	set_process(true);
}

void StorageChest::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	_player = Object::cast_to<Player>(p_body);
	_update_prompt();
}

void StorageChest::_on_body_exited(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	_player = nullptr;
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", "", false);
}

void StorageChest::_update_prompt() {
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", LOC("[X] 打开仓库"), true);
}

void StorageChest::_open_panel() {
	Node *root = get_tree()->get_current_scene();
	if (!root)
		return;
	// 面板是主场景根下的全局覆盖层（CanvasLayer 115），非洞天场景内部
	Node *panel = root->find_child("StoragePanel", true, false);
	if (panel)
		panel->call("open");
}

void StorageChest::_process(double p_delta) {
	if (!_player)
		return;
	if (Input::get_singleton()->is_action_just_pressed("interact")) {
		_open_panel();
	}
}

} // namespace godot
