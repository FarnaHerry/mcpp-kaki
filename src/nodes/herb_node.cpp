#include "herb_node.h"

#include "player.h"
#include "../cultivation/ability_manager.h"
#include "../cultivation/gongfa_system.h"
#include "../inventory/item.h"
#include "../inventory/item_database.h"
#include "../utils/signal_bus.h"
#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// 纳戒磁吸参数（与 ItemPickup 一致）
static constexpr float HERB_MAGNET_RANGE = 150.0f;
static constexpr float HERB_MAGNET_ACCEL = 80.0f;
static constexpr float HERB_MAGNET_MAX_SPEED = 200.0f;

void HerbNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_herb_id", "id"), &HerbNode::set_herb_id);
	ClassDB::bind_method(D_METHOD("get_herb_id"), &HerbNode::get_herb_id);
	ClassDB::bind_method(D_METHOD("set_quantity", "qty"), &HerbNode::set_quantity);
	ClassDB::bind_method(D_METHOD("get_quantity"), &HerbNode::get_quantity);
	ClassDB::bind_method(D_METHOD("is_harvested"), &HerbNode::is_harvested);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &HerbNode::_on_body_entered);
	ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &HerbNode::_on_body_exited);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "herb_id"), "set_herb_id", "get_herb_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "quantity"), "set_quantity", "get_quantity");
}

void HerbNode::_ready() {
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
	rect->set_size(Vector2(28, 28)); // 比掉落物大一圈，采集判定宽松
	shape->set_shape(rect);
	add_child(shape);

	_create_visual();
	set_process(true);
}

void HerbNode::_create_visual() {
	const Item *def = ItemDatabase::get_singleton()->get_item(_herb_id);

	// 品级配色：凡=翠绿 / 灵=冰蓝 / 地=紫金
	Color c = Color(0.3f, 0.9f, 0.4f, 0.85f);
	if (def) {
		if (def->grade == 1) c = Color(0.4f, 0.7f, 1.0f, 0.85f);
		else if (def->grade >= 2) c = Color(0.8f, 0.5f, 1.0f, 0.9f);
	}

	// 小草形状（三叶）
	_visual = memnew(Polygon2D);
	_visual->set_name("HerbVisual");
	_visual->set_color(c);
	PackedVector2Array herb;
	herb.append(Vector2(0, -9));
	herb.append(Vector2(3, -2));
	herb.append(Vector2(7, -6));
	herb.append(Vector2(5, 0));
	herb.append(Vector2(3, 2));
	herb.append(Vector2(-3, 2));
	herb.append(Vector2(-5, 0));
	herb.append(Vector2(-7, -6));
	herb.append(Vector2(-3, -2));
	_visual->set_polygon(herb);
	add_child(_visual);
}

void HerbNode::_on_body_entered(Node2D *p_body) {
	if (_harvested) return;
	if (p_body->get_name() != StringName("Player")) return;
	_player = Object::cast_to<Player>(p_body);

	const Item *def = ItemDatabase::get_singleton()->get_item(_herb_id);
	String name = def ? def->name : String(_herb_id);
	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->emit_signal("interaction_prompt", String(TXT("[X] 采集 ·")) + name, true);
	}
}

void HerbNode::_on_body_exited(Node2D *p_body) {
	if (p_body->get_name() != StringName("Player")) return;
	_player = nullptr;
	if (_harvested) return;
	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->emit_signal("interaction_prompt", "", false);
	}
}

void HerbNode::_process(double p_delta) {
	if (_harvested || !_player) return;

	// 纳戒已解锁 → 自动吸附采集（不走 X 交互）
	if (_has_ring) {
		float dist = _player->get_global_position().distance_to(get_global_position());
		if (dist < 24.0f) {
			_harvest();
			return;
		}
		return; // 仍在磁吸途中，不响应 X
	}

	// 无纳戒：手动 X 采集
	if (Input::get_singleton()->is_action_just_pressed("interact")) {
		_harvest();
	}
}

void HerbNode::_physics_process(double p_delta) {
	if (_harvested) return;
	if (Engine::get_singleton()->is_editor_hint()) return;

	// Lazy check 纳戒 via player
	if (!_ring_checked && _player) {
		_ring_checked = true;
		AbilityManager *am = _player->get_ability_manager();
		_has_ring = am && am->has_ability(StringName(AbilityManager::ABILITY_STORAGE_RING));
	}

	// 纳戒磁吸
	if (!_has_ring || !_player) return;

	Vector2 to_player = _player->get_global_position() - get_global_position();
	float dist = to_player.length();
	if (dist > HERB_MAGNET_RANGE || dist < 4.0f) return;

	Vector2 dir = to_player.normalized();
	_magnet_speed += HERB_MAGNET_ACCEL * float(p_delta);
	if (_magnet_speed > HERB_MAGNET_MAX_SPEED)
		_magnet_speed = HERB_MAGNET_MAX_SPEED;

	set_global_position(get_global_position() + dir * _magnet_speed * float(p_delta));
}

void HerbNode::_harvest() {
	if (_harvested || !_player) return;
	_harvested = true;

	// 入背包
	_player->pickup_item(_herb_id, _quantity);

	// 采集 = 练气行为（喂熟练 +2，量少不刷）
	if (_player->get_gongfa()) {
		_player->get_gongfa()->feed(GongfaSystem::SCHOOL_QI, 2.0f);
	}

	// 枯萎：视觉变暗 + 关闭交互
	if (_visual) {
		Color c = _visual->get_color();
		c.a = 0.2f;
		c.r *= 0.4f;
		c.g *= 0.4f;
		c.b *= 0.4f;
		_visual->set_color(c);
	}
	set_deferred("monitoring", false);

	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->emit_signal("interaction_prompt", "", false);
	}
}

} // namespace godot
