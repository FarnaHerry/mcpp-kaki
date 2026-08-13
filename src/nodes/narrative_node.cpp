#include "narrative_node.h"
#include "player.h"
#include "../core/game_manager.h"
#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.utils; // SignalBus

namespace godot {

void NarrativeNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_title", "title"), &NarrativeNode::set_title);
	ClassDB::bind_method(D_METHOD("get_title"), &NarrativeNode::get_title);
	ClassDB::bind_method(D_METHOD("set_lines", "lines"), &NarrativeNode::set_lines);
	ClassDB::bind_method(D_METHOD("get_lines"), &NarrativeNode::get_lines);
	ClassDB::bind_method(D_METHOD("set_after_lines", "lines"), &NarrativeNode::set_after_lines);
	ClassDB::bind_method(D_METHOD("get_after_lines"), &NarrativeNode::get_after_lines);
	ClassDB::bind_method(D_METHOD("set_prompt", "prompt"), &NarrativeNode::set_prompt);
	ClassDB::bind_method(D_METHOD("get_prompt"), &NarrativeNode::get_prompt);
	ClassDB::bind_method(D_METHOD("set_gm_method", "method"), &NarrativeNode::set_gm_method);
	ClassDB::bind_method(D_METHOD("get_gm_method"), &NarrativeNode::get_gm_method);
	ClassDB::bind_method(D_METHOD("set_precheck_method", "method"), &NarrativeNode::set_precheck_method);
	ClassDB::bind_method(D_METHOD("get_precheck_method"), &NarrativeNode::get_precheck_method);
	ClassDB::bind_method(D_METHOD("set_once_flag", "flag"), &NarrativeNode::set_once_flag);
	ClassDB::bind_method(D_METHOD("get_once_flag"), &NarrativeNode::get_once_flag);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &NarrativeNode::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &NarrativeNode::get_color);
	ClassDB::bind_method(D_METHOD("is_overlay_open"), &NarrativeNode::is_overlay_open);
	ClassDB::bind_method(D_METHOD("get_current_line"), &NarrativeNode::get_current_line);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &NarrativeNode::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &NarrativeNode::_on_body_exited);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "lines"), "set_lines", "get_lines");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "after_lines"), "set_after_lines", "get_after_lines");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt"), "set_prompt", "get_prompt");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "gm_method"), "set_gm_method", "get_gm_method");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "precheck_method"), "set_precheck_method", "get_precheck_method");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "once_flag"), "set_once_flag", "get_once_flag");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
}

void NarrativeNode::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	if (_prompt.is_empty())
		_prompt = LOC("[X] 交谈");

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

	// NPC 视觉：袍 + 冠（颜色由 color 属性定）
	Polygon2D *robe = memnew(Polygon2D);
	robe->set_color(_color);
	PackedVector2Array body;
	body.append(Vector2(-9, -12));
	body.append(Vector2(9, -12));
	body.append(Vector2(10, 10));
	body.append(Vector2(-10, 10));
	robe->set_polygon(body);
	add_child(robe);

	Polygon2D *hat = memnew(Polygon2D);
	hat->set_color(_color.darkened(0.45f));
	PackedVector2Array hp;
	hp.append(Vector2(-7, -12));
	hp.append(Vector2(7, -12));
	hp.append(Vector2(0, -21));
	hat->set_polygon(hp);
	add_child(hat);

	// 叙事 overlay 期间暂停世界，本节点仍需处理输入
	set_process_mode(PROCESS_MODE_ALWAYS);
	_create_overlay();
	set_process(true);
}

// ============================================================
// Overlay
// ============================================================

void NarrativeNode::_create_overlay() {
	_overlay = memnew(CanvasLayer);
	_overlay->set_layer(121);
	add_child(_overlay);

	ColorRect *dim = memnew(ColorRect);
	dim->set_color(Color(0, 0, 0, 0.78f));
	// CanvasLayer 无父 Control，全屏锚不生效——显式铺大（覆盖视口及扩展区域）
	dim->set_size(Vector2(4096, 4096));
	dim->set_position(Vector2(-2048, -2048));
	_overlay->add_child(dim);

	_title_label = memnew(Label);
	_title_label->set_position(Vector2(0, 58));
	_title_label->set_size(Vector2(480, 20));
	_title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_title_label->add_theme_font_size_override("font_size", 15);
	_title_label->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f, 1.0f));
	_overlay->add_child(_title_label);

	_body_label = memnew(Label);
	_body_label->set_position(Vector2(48, 100));
	_body_label->set_size(Vector2(384, 110));
	_body_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	_body_label->add_theme_font_size_override("font_size", 12);
	_body_label->add_theme_color_override("font_color", Color(0.95f, 0.95f, 0.95f, 1.0f));
	_overlay->add_child(_body_label);

	_hint_label = memnew(Label);
	_hint_label->set_position(Vector2(0, 232));
	_hint_label->set_size(Vector2(480, 16));
	_hint_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	_hint_label->add_theme_font_size_override("font_size", 10);
	_hint_label->add_theme_color_override("font_color", Color(0.7f, 0.7f, 0.7f, 1.0f));
	_hint_label->set_text(LOC("[X] 继续"));
	_overlay->add_child(_hint_label);

	_overlay->set_visible(false);
}

String NarrativeNode::get_current_line() const {
	if (!_overlay_open || !_body_label)
		return String();
	return _body_label->get_text();
}

// ============================================================
// 交互
// ============================================================

void NarrativeNode::_on_body_entered(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	// 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if (p_body->get_global_position().distance_to(get_global_position()) > 48.0f)
		return;
	_player = Object::cast_to<Player>(p_body);
	_show_prompt(true);
}

void NarrativeNode::_on_body_exited(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player"))
		return;
	if (!_player)
		return;
	_player = nullptr;
	// 同空间离开才清提示
	if (p_body->get_parent() != get_parent())
		return;
	_show_prompt(false);
}

void NarrativeNode::_show_prompt(bool p_show) {
	SignalBus *bus = SignalBus::get_singleton();
	if (!bus)
		return;
	bus->emit_signal("interaction_prompt", p_show ? _prompt : String(), p_show);
}

void NarrativeNode::_process(double p_delta) {
	if (_overlay_open) {
		if (Input::get_singleton()->is_action_just_pressed("interact") ||
			Input::get_singleton()->is_action_just_pressed("ui_accept")) {
			_advance();
		}
		return;
	}
	// 菜单/其他系统暂停时不响应交互（本节点 PROCESS_MODE_ALWAYS 会继续跑）
	if (get_tree()->is_paused())
		return;
	if (_player && Input::get_singleton()->is_action_just_pressed("interact")) {
		_interact();
	}
}

void NarrativeNode::_interact() {
	GameManager *gm = GameManager::get_singleton();

	// 首轮已完成后：播 after_lines（不再 precheck/回调）
	if (gm && !_once_flag.is_empty() && gm->has_flag(_once_flag) && !_after_lines.is_empty()) {
		_play(_after_lines, true);
		return;
	}

	// 前置校验（条件不足 → 单行拒绝叙事）
	if (gm && !_precheck_method.is_empty() && gm->has_method(_precheck_method)) {
		String reason = gm->call(_precheck_method);
		if (!reason.is_empty()) {
			PackedStringArray refuse;
			refuse.append(reason);
			_play(refuse, true); // 拒绝不算完成，不落 flag
			return;
		}
	}

	if (!_lines.is_empty())
		_play(_lines, false);
}

void NarrativeNode::_play(const PackedStringArray &p_lines, bool p_repeat) {
	if (p_lines.is_empty() || !_overlay)
		return;
	_repeat_mode = p_repeat;
	_active = p_lines;
	_line_idx = 0;
	_overlay_open = true;

	_was_paused = get_tree()->is_paused();
	get_tree()->set_pause(true);

	_title_label->set_text(_title);
	_body_label->set_text(_active[0]);
	_overlay->set_visible(true);
	_show_prompt(false); // 叙事期间收起交互提示
}

void NarrativeNode::_advance() {
	_line_idx++;
	if (_line_idx >= int(_active.size())) {
		_close();
		return;
	}
	_body_label->set_text(_active[_line_idx]);
}

void NarrativeNode::_close() {
	if (_overlay)
		_overlay->set_visible(false);
	_overlay_open = false;
	get_tree()->set_pause(_was_paused);

	GameManager *gm = GameManager::get_singleton();
	if (!_repeat_mode) {
		if (gm && !_once_flag.is_empty())
			gm->set_flag(_once_flag, true);
		if (gm && !_gm_method.is_empty() && gm->has_method(_gm_method))
			gm->call(_gm_method);
	}
	// 玩家仍在旁则恢复提示
	if (_player)
		_show_prompt(true);
}

} // namespace godot
