#include "damage_numbers.h"

#include "../utils/signal_bus.h"
#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {

void DamageNumbers::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_damage_dealt", "world_pos", "amount", "is_player_victim"),
			&DamageNumbers::_on_damage_dealt);
}

void DamageNumbers::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->connect("damage_dealt", Callable(this, "_on_damage_dealt"));
	}
}

void DamageNumbers::_on_damage_dealt(Vector2 p_world_pos, float p_amount, bool p_is_player_victim) {
	Node *scene = get_tree()->get_current_scene();
	if (!scene) return;

	Node2D *root = memnew(Node2D);
	// 交错偏移，连续命中时数字不重叠
	float ox = float((_spawn_counter % 5) - 2) * 5.0f;
	_spawn_counter++;
	root->set_position(p_world_pos + Vector2(ox, -18.0f));
	root->set_z_index(100);

	Label *label = memnew(Label);
	label->set_text(String::num_int64(int64_t(Math::round(p_amount))));
	label->add_theme_font_size_override("font_size", 8);
	label->add_theme_constant_override("outline_size", 3);
	label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9f));
	if (p_is_player_victim) {
		label->add_theme_color_override("font_color", Color(1.0f, 0.3f, 0.25f)); // 玩家挨打：红
	} else {
		label->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.55f)); // 敌人挨打：金
	}
	label->set_position(Vector2(-6.0f, -5.0f));
	root->add_child(label);

	Entry e;
	e.root = root;
	e.drift_x = ox * 0.4f;
	_active.push_back(e);

	// take_damage 可能在物理回调里触发，add_child 推迟到 flush 后
	scene->call_deferred("add_child", root);
}

void DamageNumbers::_process(double p_delta) {
	for (size_t i = 0; i < _active.size();) {
		Entry &e = _active[i];
		e.t += float(p_delta);
		Node2D *root = e.root;
		if (!root || !root->is_inside_tree()) {
			_active.erase(_active.begin() + i);
			continue;
		}
		if (e.t >= LIFETIME) {
			root->queue_free();
			_active.erase(_active.begin() + i);
			continue;
		}
		Vector2 pos = root->get_position();
		pos.y -= RISE_SPEED * float(p_delta);
		pos.x += e.drift_x * float(p_delta);
		root->set_position(pos);
		float k = e.t / LIFETIME;
		root->set_modulate(Color(1, 1, 1, 1.0f - k * k));
		i++;
	}
}

} // namespace godot
