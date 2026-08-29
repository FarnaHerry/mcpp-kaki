#include "teleport_array.h"
#include "player.h"

#include "../core/game_manager.h"
#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.utils; // SignalBus

namespace godot {

void TeleportArray::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "id", "name", "continent", "scene_path"), &TeleportArray::setup);
	ClassDB::bind_method(D_METHOD("get_tp_id"), &TeleportArray::get_tp_id);
	ClassDB::bind_method(D_METHOD("get_tp_name"), &TeleportArray::get_tp_name);
	ClassDB::bind_method(D_METHOD("is_activated"), &TeleportArray::is_activated);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &TeleportArray::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &TeleportArray::_on_body_exited);
}

void TeleportArray::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	add_to_group("tp_arrays");
	set_collision_layer_value(1, false);
	set_collision_mask_value(3, true);
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));
	connect("body_exited", Callable(this, "_on_body_exited"));

	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(44, 60));
	shape->set_shape(rect);
	add_child(shape);

	_build_visuals();
	_refresh_visual();
	set_process(true);
}

void TeleportArray::setup(const String &p_id, const String &p_name, const String &p_continent, const String &p_scene_path) {
	_id = p_id;
	_name = p_name;
	_continent = p_continent;
	_scene_path = p_scene_path;
	_build_visuals();
	_refresh_visual();
}

bool TeleportArray::is_activated() const {
	GameManager *gm = GameManager::get_singleton();
	return gm && gm->has_flag("tp:" + _id);
}

void TeleportArray::_build_visuals() {
	// 重复 setup 重建：清旧视觉子节点（保留 CollisionShape2D）
	for (int i = get_child_count() - 1; i >= 0; i--) {
		Node *c = get_child(i);
		if (Object::cast_to<CollisionShape2D>(c))
			continue;
		c->queue_free();
	}
	_glow = nullptr;

	// 底座云台（青灰阶石）
	Polygon2D *base = memnew(Polygon2D);
	base->set_name("Base");
	base->set_color(Color(0.38f, 0.42f, 0.48f, 1.0f));
	PackedVector2Array bp;
	bp.append(Vector2(-18, 12));
	bp.append(Vector2(18, 12));
	bp.append(Vector2(14, 20));
	bp.append(Vector2(-14, 20));
	base->set_polygon(bp);
	add_child(base);

	// 碑身
	Polygon2D *stone = memnew(Polygon2D);
	stone->set_name("Stone");
	stone->set_color(Color(0.52f, 0.56f, 0.62f, 1.0f));
	PackedVector2Array sp;
	sp.append(Vector2(-8, 10));
	sp.append(Vector2(8, 10));
	sp.append(Vector2(6, -22));
	sp.append(Vector2(-6, -22));
	stone->set_polygon(sp);
	add_child(stone);

	// 碑顶云纹（发光片——激活青色 / 未激活暗沉）
	_glow = memnew(Polygon2D);
	_glow->set_name("Glow");
	PackedVector2Array gp;
	for (int i = 0; i < 8; i++) {
		float a = Math_TAU * i / 8.0f;
		float r = (i % 2 == 0) ? 7.0f : 3.5f;
		gp.append(Vector2(Math::cos(a) * r, Math::sin(a) * r - 27.0f));
	}
	_glow->set_polygon(gp);
	add_child(_glow);

	// 阵名（头顶）
	Label *name = memnew(Label);
	name->set_name("TpName");
	name->set_text(LOC("云游阵 · ") + _name);
	name->add_theme_font_size_override("font_size", 7);
	name->add_theme_color_override("font_color", Color(0.75f, 0.9f, 1.0f, 0.95f));
	name->set_position(Vector2(-45, -44));
	name->set_size(Vector2(90, 10));
	name->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	add_child(name);
}

void TeleportArray::_refresh_visual() {
	if (!_glow)
		return;
	bool on = is_activated();
	_glow->set_color(on ? Color(0.4f, 0.95f, 1.0f, 0.95f) : Color(0.3f, 0.34f, 0.4f, 0.8f));
}

void TeleportArray::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	if (p_body->get_global_position().distance_to(get_global_position()) > 48.0f)
		return; // 幽灵 enter 守卫（同 StorageChest）
	_player = Object::cast_to<Player>(p_body);
	if (!is_activated()) {
		GameManager *gm = GameManager::get_singleton();
		if (gm) {
			gm->set_flag("tp:" + _id);
			SignalBus *bus = SignalBus::get_singleton();
			if (bus)
				bus->emit_signal("interaction_prompt", LOC("云游阵 · ") + _name + LOC(" 已铭刻"), true);
		}
		_refresh_visual();
	} else {
		_update_prompt();
	}
}

void TeleportArray::_on_body_exited(Node2D *p_body) {
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

void TeleportArray::_update_prompt() {
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", LOC("[X] 驾云"), true);
}

void TeleportArray::_process(double p_delta) {
	if (!_player)
		return;
	if (Input::get_singleton()->is_action_just_pressed("interact"))
		_open_panel();
}

void TeleportArray::_open_panel() {
	Node *root = get_tree()->get_current_scene();
	Node *panel = root ? root->find_child("TeleportPanel", true, false) : nullptr;
	if (!panel || !panel->has_method("open_panel"))
		return;
	if (bool(panel->call("is_open")))
		return; // 面板已开（防同帧重复开）
	panel->call("open_panel", _player);
}

} // namespace godot
