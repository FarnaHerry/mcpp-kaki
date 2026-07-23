#ifndef CPP_KAKI_PROJECTILE_H
#define CPP_KAKI_PROJECTILE_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class Node2D;

// Reusable projectile — flies in a direction, damages on body hit, auto-destroys.
// Used by archer enemies and future player abilities.
class Projectile : public Area2D {
	GDCLASS(Projectile, Area2D);

public:
	float speed = 250.0f;
	float damage = 1.0f;
	Vector2 direction = Vector2(1.0f, 0.0f);
	float lifetime = 3.0f;

	// Source node (who fired this) — omitted from damage to prevent self-hit
	void set_source(Node *p_source) { _source = p_source; }
	Node *get_source() const { return _source; }

	void _ready() override;
	void _physics_process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	Node *_source = nullptr;
	double _age = 0.0;

	void _create_visual();
};

} // namespace godot

#endif // CPP_KAKI_PROJECTILE_H
