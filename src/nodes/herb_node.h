#ifndef CPP_KAKI_HERBNODE_H
#define CPP_KAKI_HERBNODE_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

namespace godot {

class Player;

class HerbNode : public Area2D {
	GDCLASS(HerbNode, Area2D);

public:
	void set_herb_id(const StringName &p_id) { _herb_id = p_id; }
	StringName get_herb_id() const { return _herb_id; }

	void set_quantity(int p_qty) { _quantity = p_qty; }
	int get_quantity() const { return _quantity; }

	bool is_harvested() const { return _harvested; }

	void _ready() override;
	void _process(double p_delta) override;
	void _physics_process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	StringName _herb_id;
	int _quantity = 1;
	bool _harvested = false;
	Player *_player = nullptr;
	Polygon2D *_visual = nullptr;
	float _magnet_speed = 0.0f;
	bool _ring_checked = false;
	bool _has_ring = false;

	void _create_visual();
	void _harvest();
};

} // namespace godot

#endif // CPP_KAKI_HERBNODE_H
