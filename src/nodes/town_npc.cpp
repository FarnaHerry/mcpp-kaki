#include "town_npc.h"
#include "player.h"

#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

import mcpp_kaki.cultivation; // 灵力恢复
import mcpp_kaki.utils;       // SignalBus

namespace godot {

void TownNpc::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "npc_name", "robe", "lines", "heal"), &TownNpc::setup);
	ClassDB::bind_method(D_METHOD("get_npc_name"), &TownNpc::get_npc_name);
	ClassDB::bind_method(D_METHOD("is_healer"), &TownNpc::is_healer);
	ClassDB::bind_method(D_METHOD("get_bubble_text"), &TownNpc::get_bubble_text);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &TownNpc::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &TownNpc::_on_body_exited);
}

void TownNpc::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	add_to_group("town_npcs");
	set_collision_layer_value(1, false);
	set_collision_mask_value(3, true);
	set_deferred("monitoring", true);
	set_deferred("monitorable", false);

	connect("body_entered", Callable(this, "_on_body_entered"));
	connect("body_exited", Callable(this, "_on_body_exited"));

	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(36, 56));
	shape->set_shape(rect);
	add_child(shape);

	_build_visuals();
	set_process(true);
}

void TownNpc::setup(const String &p_npc_name, const Color &p_robe, const PackedStringArray &p_lines, bool p_heal) {
	_npc_name = p_npc_name;
	_robe = p_robe;
	_lines = p_lines;
	_heal = p_heal;
	_line_idx = 0;
	_build_visuals();
}

void TownNpc::_build_visuals() {
	// 重复 setup 重建：先清旧视觉子节点（保留 CollisionShape2D）
	for (int i = get_child_count() - 1; i >= 0; i--) {
		Node *c = get_child(i);
		if (Object::cast_to<CollisionShape2D>(c))
			continue;
		c->queue_free();
	}
	_bubble = nullptr;
	_bubble_bg = nullptr;

	// 长袍（梯形）+ 头（肤色）+ 发髻
	Polygon2D *robe = memnew(Polygon2D);
	robe->set_name("Robe");
	robe->set_color(_robe);
	PackedVector2Array rp;
	rp.append(Vector2(-8, -10));
	rp.append(Vector2(8, -10));
	rp.append(Vector2(10, 14));
	rp.append(Vector2(-10, 14));
	robe->set_polygon(rp);
	add_child(robe);

	Polygon2D *head = memnew(Polygon2D);
	head->set_name("Head");
	head->set_color(Color(0.92f, 0.78f, 0.62f, 1.0f));
	PackedVector2Array hp;
	for (int i = 0; i < 8; i++) {
		float a = Math_TAU * i / 8.0f;
		hp.append(Vector2(Math::cos(a) * 6.0f, Math::sin(a) * 6.0f - 16.0f));
	}
	head->set_polygon(hp);
	add_child(head);

	Polygon2D *hair = memnew(Polygon2D);
	hair->set_name("Hair");
	hair->set_color(Color(0.16f, 0.12f, 0.1f, 1.0f));
	PackedVector2Array tp;
	tp.append(Vector2(-6, -24));
	tp.append(Vector2(6, -24));
	tp.append(Vector2(3, -28));
	tp.append(Vector2(-3, -28));
	hair->set_polygon(tp);
	add_child(hair);

	// 头顶名字（固定宽居中）
	Label *name = memnew(Label);
	name->set_name("NpcName");
	name->set_text(_npc_name);
	name->add_theme_font_size_override("font_size", 7);
	name->add_theme_color_override("font_color", Color(0.95f, 0.92f, 0.8f, 1.0f));
	name->set_position(Vector2(-40, -44));
	name->set_size(Vector2(80, 10));
	name->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	add_child(name);

	// 对话气泡（隐藏，X 时显示）
	_bubble_bg = memnew(ColorRect);
	_bubble_bg->set_name("BubbleBg");
	_bubble_bg->set_position(Vector2(-70, -66));
	_bubble_bg->set_size(Vector2(140, 16));
	_bubble_bg->set_color(Color(0.08f, 0.08f, 0.1f, 0.85f));
	_bubble_bg->set_visible(false);
	add_child(_bubble_bg);

	_bubble = memnew(Label);
	_bubble->set_name("Bubble");
	_bubble->add_theme_font_size_override("font_size", 8);
	_bubble->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.8f, 1.0f));
	_bubble->set_position(Vector2(-70, -64));
	_bubble->set_size(Vector2(140, 14));
	_bubble->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	_bubble->set_visible(false);
	add_child(_bubble);
}

void TownNpc::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	if (p_body->get_global_position().distance_to(get_global_position()) > 48.0f)
		return; // 幽灵 enter 守卫（同 StorageChest）
	_player = Object::cast_to<Player>(p_body);
	_update_prompt();
}

void TownNpc::_on_body_exited(Node2D *p_body) {
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

void TownNpc::_update_prompt() {
	SignalBus *bus = SignalBus::get_singleton();
	if (!bus)
		return;
	if (_heal)
		bus->emit_signal("interaction_prompt", LOC("[X] 歇息"), true);
	else
		bus->emit_signal("interaction_prompt", LOC("[X] 交谈"), true);
}

void TownNpc::_process(double p_delta) {
	// 气泡计时收起
	if (_bubble_t > 0.0f) {
		_bubble_t -= float(p_delta);
		if (_bubble_t <= 0.0f && _bubble) {
			_bubble->set_visible(false);
			_bubble_bg->set_visible(false);
		}
	}
	if (!_player)
		return;
	if (Input::get_singleton()->is_action_just_pressed("interact"))
		_interact();
}

void TownNpc::_interact() {
	if (_heal) {
		// 客栈歇息：HP/灵力全恢复
		if (!_player || _player->is_dead())
			return;
		_player->set_current_health(_player->get_max_health());
		SignalBus *bus = SignalBus::get_singleton();
		if (bus)
			bus->emit_signal("player_health_changed", _player->get_current_health(), _player->get_max_health());
		CultivationSystem *cult = _player->get_cultivation();
		if (cult)
			cult->set_mana(cult->get_max_mana());
		_say_bubble(LOC("歇息片刻，气血圆满。"));
		return;
	}
	if (_lines.is_empty())
		return;
	_say_bubble(_lines[_line_idx % _lines.size()]);
	_line_idx++;
}

void TownNpc::_say_bubble(const String &p_text) {
	if (!_bubble || !_bubble_bg)
		return;
	_bubble->set_text(p_text);
	_bubble->set_visible(true);
	_bubble_bg->set_visible(true);
	_bubble_t = 2.5f;
}

String TownNpc::get_bubble_text() const {
	return _bubble && _bubble->is_visible() ? _bubble->get_text() : String();
}

} // namespace godot
