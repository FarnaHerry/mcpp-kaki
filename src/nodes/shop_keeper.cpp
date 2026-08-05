#include "shop_keeper.h"
#include "player.h"

#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.utils; // SignalBus

namespace godot {

void ShopKeeper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &ShopKeeper::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &ShopKeeper::_on_body_exited);
}

void ShopKeeper::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_collision_layer_value(1, false);
	set_collision_mask_value(3, true);
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));
	connect("body_exited", Callable(this, "_on_body_exited"));

	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(30, 50));
	shape->set_shape(rect);
	add_child(shape);

	// 掌柜：青衫 + 帽 + 摊位小旗
	Polygon2D *body = memnew(Polygon2D);
	body->set_color(Color(0.35f, 0.5f, 0.45f, 1.0f));
	PackedVector2Array bp;
	bp.append(Vector2(-8, -10));
	bp.append(Vector2(8, -10));
	bp.append(Vector2(9, 14));
	bp.append(Vector2(-9, 14));
	body->set_polygon(bp);
	add_child(body);

	Polygon2D *hat = memnew(Polygon2D);
	hat->set_color(Color(0.25f, 0.2f, 0.16f, 1.0f));
	PackedVector2Array hp;
	hp.append(Vector2(-9, -12));
	hp.append(Vector2(9, -12));
	hp.append(Vector2(6, -20));
	hp.append(Vector2(-6, -20));
	hat->set_polygon(hp);
	add_child(hat);

	Polygon2D *flag = memnew(Polygon2D);
	flag->set_color(Color(0.85f, 0.75f, 0.3f, 1.0f));
	PackedVector2Array fp;
	fp.append(Vector2(9, -8));
	fp.append(Vector2(18, -11));
	fp.append(Vector2(9, -14));
	flag->set_polygon(fp);
	add_child(flag);

	set_process(true);
}

void ShopKeeper::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	// 幽灵 enter 守卫（同 StorageChest）
	if (p_body->get_global_position().distance_to(get_global_position()) > 48.0f)
		return;
	_player = Object::cast_to<Player>(p_body);
	_update_prompt();
}

void ShopKeeper::_on_body_exited(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	if (!_player)
		return;
	_player = nullptr;
	if (p_body->get_parent() != get_parent())
		return;
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", "", false);
}

void ShopKeeper::_update_prompt() {
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", LOC("[X] 交易"), true);
}

void ShopKeeper::_open_panel() {
	Node *root = get_tree()->get_current_scene();
	if (!root)
		return;
	Node *panel = root->find_child("ShopPanel", true, false);
	if (panel)
		panel->call("open");
}

void ShopKeeper::_process(double p_delta) {
	if (!_player)
		return;
	if (Input::get_singleton()->is_action_just_pressed("interact")) {
		_open_panel();
	}
}

} // namespace godot
