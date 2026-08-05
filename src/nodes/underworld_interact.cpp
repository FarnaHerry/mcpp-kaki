#include "underworld_interact.h"
#include "player.h"
#include "../core/soul_ledger_system.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.utils;        // SignalBus
import mcpp_kaki.cultivation;  // get_realm_name

namespace godot {

void UnderworldInteractNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &UnderworldInteractNode::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &UnderworldInteractNode::get_mode);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &UnderworldInteractNode::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &UnderworldInteractNode::_on_body_exited);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode"), "set_mode", "get_mode");
}

void UnderworldInteractNode::_ready() {
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
	rect->set_size(Vector2(30, 30));
	shape->set_shape(rect);
	add_child(shape);

	if (_mode == MODE_INSPECT) {
		// 判官：青袍 + 判官帽
		Polygon2D *robe = memnew(Polygon2D);
		robe->set_color(Color(0.20f, 0.35f, 0.45f, 1.0f));
		PackedVector2Array body;
		body.append(Vector2(-9, -12));
		body.append(Vector2(9, -12));
		body.append(Vector2(10, 10));
		body.append(Vector2(-10, 10));
		robe->set_polygon(body);
		add_child(robe);

		Polygon2D *hat = memnew(Polygon2D);
		hat->set_color(Color(0.10f, 0.15f, 0.22f, 1.0f));
		PackedVector2Array hp;
		hp.append(Vector2(-10, -12));
		hp.append(Vector2(10, -12));
		hp.append(Vector2(0, -22));
		hat->set_polygon(hp);
		add_child(hat);
	} else {
		// 生死簿：卷轴 + 朱印
		Polygon2D *scroll = memnew(Polygon2D);
		scroll->set_color(Color(0.55f, 0.45f, 0.25f, 1.0f));
		PackedVector2Array sp;
		sp.append(Vector2(-8, -14));
		sp.append(Vector2(8, -14));
		sp.append(Vector2(8, 14));
		sp.append(Vector2(-8, 14));
		scroll->set_polygon(sp);
		add_child(scroll);

		Polygon2D *seal = memnew(Polygon2D);
		seal->set_color(Color(0.80f, 0.20f, 0.20f, 1.0f));
		PackedVector2Array sp2;
		sp2.append(Vector2(-4, -4));
		sp2.append(Vector2(4, -4));
		sp2.append(Vector2(4, 4));
		sp2.append(Vector2(-4, 4));
		seal->set_polygon(sp2);
		add_child(seal);
	}

	set_process(true);
}

void UnderworldInteractNode::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	// 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if (p_body->get_global_position().distance_to(get_global_position()) > 48.0f)
		return;
	_player = Object::cast_to<Player>(p_body);
	_update_prompt();
}

void UnderworldInteractNode::_on_body_exited(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	if (!_player)
		return;
	_player = nullptr;
	// 同空间离开才清提示
	if (p_body->get_parent() != get_parent())
		return;
	SignalBus *bus = SignalBus::get_singleton();
	if (!bus)
		return;
	bus->emit_signal("interaction_prompt", "", false);
	// 离开关掉查簿 overlay
	if (_overlay_open) {
		_overlay_open = false;
		bus->emit_signal("ledger_inspect_requested", Dictionary(), false);
	}
}

void UnderworldInteractNode::_update_prompt() {
	SignalBus *bus = SignalBus::get_singleton();
	if (!bus)
		return;
	if (_mode == MODE_INSPECT)
		bus->emit_signal("interaction_prompt", LOC("[X] 查生死簿"), true);
	else
		bus->emit_signal("interaction_prompt", LOC("[X] 改簿划名（免死一次）"), true);
}

void UnderworldInteractNode::_process(double p_delta) {
	if (_msg_t > 0.0f) {
		_msg_t -= float(p_delta);
		if (_msg_t <= 0.0f) {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus)
				bus->emit_signal("interaction_prompt", "", false);
		}
	}
	if (!_player)
		return;
	if (Input::get_singleton()->is_action_just_pressed("interact")) {
		_interact();
	}
}

void UnderworldInteractNode::_interact() {
	Node *root = get_tree()->get_current_scene();
	if (!root)
		return;
	SoulLedgerSystem *ledger = Object::cast_to<SoulLedgerSystem>(root->find_child("SoulLedgerSystem", true, false));
	SignalBus *bus = SignalBus::get_singleton();
	if (!ledger || !bus)
		return;

	if (_mode == MODE_INSPECT) {
		_overlay_open = !_overlay_open;
		Dictionary d;
		d["origin"] = ledger->get_origin_name();
		d["original_body"] = ledger->get_original_body();
		d["ledger_lifespan"] = ledger->get_ledger_lifespan();
		d["actual_lifespan"] = ledger->get_actual_lifespan();
		d["soul_protection"] = ledger->has_soul_protection();
		if (_player && _player->get_cultivation())
			d["realm_name"] = _player->get_cultivation()->get_realm_name();
		bus->emit_signal("ledger_inspect_requested", d, _overlay_open);
	} else {
		if (ledger->mark_soul_exempt())
			bus->emit_signal("interaction_prompt", LOC("改簿成功——已划名，免死一次！"), true);
		else
			bus->emit_signal("interaction_prompt", LOC("已划名，无需再改。"), true);
		_msg_t = 2.5f;
	}
}

} // namespace godot
