#ifndef CPP_KAKI_PROJECTILE_H
#define CPP_KAKI_PROJECTILE_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "damage_types.h"

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
	// 伤害类型（DamageCalculator 统一结算）+ 投射物颜色（创建者在 add_child 前设置）
	DamageCategory damage_category = DMG_PHYSICAL;
	Element element = ELEM_NONE;
	Color visual_color = Color(1.0f, 0.4f, 0.2f, 0.9f);

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
