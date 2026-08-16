#ifndef CPP_KAKI_ITEMPICKUP_H
#define CPP_KAKI_ITEMPICKUP_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

namespace godot {

class Player;

class ItemPickup : public Area2D {
	GDCLASS(ItemPickup, Area2D);

public:
	void set_item_id(const StringName &p_id);
	StringName get_item_id() const { return _item_id; }

	void set_quantity(int p_qty) { _quantity = p_qty; }
	int get_quantity() const { return _quantity; }

	void _ready() override;
	void _physics_process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	StringName _item_id;
	int _quantity = 1;
	Node2D *_player_cache = nullptr;
	bool _player_checked = false;
	float _magnet_speed = 0.0f;

	void _create_visual();
};

} // namespace godot

#endif // CPP_KAKI_ITEMPICKUP_H
