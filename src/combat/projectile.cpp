#include "projectile.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

void Projectile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_source", "source"), &Projectile::set_source);
	ClassDB::bind_method(D_METHOD("get_source"), &Projectile::get_source);
	ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &Projectile::_on_body_entered);
}

void Projectile::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	// Projectile detects bodies on target layer (set by creator, default 3 = Player)
	set_collision_layer_value(1, false); // Not on any body layer itself
	// Collision mask is set by the creator — default to detect Player (layer 3)
	set_collision_mask_value(3, true);
	set_monitoring(false);
	set_monitorable(true);

	connect("body_entered", Callable(this, "_on_body_entered"));

	// Collision shape (small)
	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<RectangleShape2D> rect;
	rect.instantiate();
	rect->set_size(Vector2(12, 8));
	shape->set_shape(rect);
	add_child(shape);

	_create_visual();
}

void Projectile::_create_visual() {
	// Small arrow/diamond shape
	Polygon2D *visual = memnew(Polygon2D);
	visual->set_name("ProjVisual");
	visual->set_color(Color(1.0f, 0.4f, 0.2f, 0.9f));
	PackedVector2Array arrow;
	arrow.append(Vector2(8, 0));   // tip
	arrow.append(Vector2(-4, -4)); // top back
	arrow.append(Vector2(-2, 0));  // notch
	arrow.append(Vector2(-4, 4));  // bottom back
	visual->set_polygon(arrow);
	add_child(visual);
}

void Projectile::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	_age += p_delta;
	if (_age >= lifetime) {
		queue_free();
		return;
	}

	// Move in direction
	Vector2 pos = get_position();
	pos += direction * speed * p_delta;
	set_position(pos);

	// Rotate to face direction
	set_rotation(direction.angle());
}

void Projectile::_on_body_entered(Node2D *p_body) {
	if (!p_body) return;

	// Don't hit our own source
	if (p_body == _source) return;

	// Deal damage if the body has take_damage method
	if (p_body->has_method("take_damage")) {
		p_body->call("take_damage", damage, _source);
	}

	queue_free();
}

} // namespace godot
