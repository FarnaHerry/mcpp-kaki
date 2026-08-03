#ifndef CPP_KAKI_CAMERAROOM2D_H
#define CPP_KAKI_CAMERAROOM2D_H

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

class CameraRoom2D : public Camera2D {
	GDCLASS(CameraRoom2D, Camera2D);

public:
	enum CameraMode {
		WORLD_FOLLOW,
		ROOM_LOCKED
	};

	float follow_speed = 4.0f;
	Vector2 dead_zone = Vector2(30, 16);
	float look_ahead_amount = 48.0f;
	Rect2 room_bounds = Rect2(0, 0, 480, 270);
	float room_transition_duration = 0.5f;

	int get_mode() const { return (int)_mode; }

	void set_follow_target(Node2D *p_target);
	void set_world_bounds(const Rect2 &p_bounds);
	void enter_room(const Rect2 &p_bounds);
	void exit_room();

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	CameraMode _mode = WORLD_FOLLOW;
	Node2D *_follow_target = nullptr;
	Rect2 _world_bounds = Rect2(0, 0, 10000, 10000);
	float _current_look_ahead = 0.0f;

	bool _transitioning = false;
	double _transition_time = 0.0;
	Vector2 _transition_start;
	Vector2 _transition_target;

	void _update_world_follow(double p_delta);
	void _update_room_locked(double p_delta);
	void _update_transition(double p_delta);
};

} // namespace godot

#endif // CPP_KAKI_CAMERAROOM2D_H
