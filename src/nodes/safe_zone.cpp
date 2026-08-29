#include "safe_zone.h"

#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

import mcpp_kaki.utils; // SignalBus

namespace godot {

void SafeZone::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "size", "town_name"), &SafeZone::setup);
	ClassDB::bind_method(D_METHOD("get_town_name"), &SafeZone::get_town_name);
	ClassDB::bind_static_method("SafeZone", D_METHOD("is_point_safe", "point"), &SafeZone::is_point_safe);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &SafeZone::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &SafeZone::_on_body_exited);
}

void SafeZone::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	add_to_group("safe_zones");
	set_collision_layer_value(1, false);
	set_collision_mask_value(3, true); // 玩家层
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));
	connect("body_exited", Callable(this, "_on_body_exited"));

	_shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(_half * 2.0f);
	_shape->set_shape(rect);
	add_child(_shape);

	_rebuild_tint();
	set_process(true);
}

void SafeZone::setup(const Vector2 &p_size, const String &p_town_name) {
	_half = p_size * 0.5f;
	_town_name = p_town_name;
	if (_shape) {
		Ref<RectangleShape2D> rect = _shape->get_shape();
		if (rect.is_valid())
			rect->set_size(p_size);
	}
	_rebuild_tint();
}

void SafeZone::_rebuild_tint() {
	if (!_tint) {
		_tint = memnew(Polygon2D);
		add_child(_tint);
	}
	// 暖色地染：安全感的视觉暗示（极低透明度，不遮挡世界）
	_tint->set_color(Color(1.0f, 0.78f, 0.42f, 0.05f));
	PackedVector2Array pts;
	pts.append(Vector2(-_half.x, -_half.y));
	pts.append(Vector2(_half.x, -_half.y));
	pts.append(Vector2(_half.x, _half.y));
	pts.append(Vector2(-_half.x, _half.y));
	_tint->set_polygon(pts);
}

bool SafeZone::is_point_safe(const Vector2 &p_point) {
	SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (!st)
		return false;
	TypedArray<Node> zones = st->get_nodes_in_group("safe_zones");
	for (int i = 0; i < zones.size(); i++) {
		SafeZone *z = Object::cast_to<SafeZone>(zones[i]);
		if (!z)
			continue;
		Vector2 d = p_point - z->get_global_position();
		if (Math::abs(d.x) <= z->_half.x && Math::abs(d.y) <= z->_half.y)
			return true;
	}
	return false;
}

void SafeZone::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	if (p_body->get_global_position().distance_to(get_global_position()) > _half.length())
		return; // 幽灵 enter 守卫（同 StorageChest）
	_player_inside = true;
	_say(LOC("已进入安全区 · ") + _town_name + LOC("（缓速休整中）"), 2.2f);
}

void SafeZone::_on_body_exited(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	_player_inside = false;
	_say(LOC("已离开安全区，妖兽重新窥伺"), 1.5f);
}

void SafeZone::_say(const String &p_text, float p_secs) {
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", p_text, true);
	_msg_t = p_secs;
}

void SafeZone::_process(double p_delta) {
	if (_msg_t <= 0.0f)
		return;
	_msg_t -= float(p_delta);
	if (_msg_t <= 0.0f) {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus)
			bus->emit_signal("interaction_prompt", "", false);
	}
}

} // namespace godot
