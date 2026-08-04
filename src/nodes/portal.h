#ifndef CPP_KAKI_PORTAL_H
#define CPP_KAKI_PORTAL_H

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
class CameraRoom2D;

class Portal : public Area2D {
	GDCLASS(Portal, Area2D);

public:
	void set_target_scene(const String &p_path) { _target_scene = p_path; }
	String get_target_scene() const { return _target_scene; }

	void set_spawn_marker(const String &p_name) { _spawn_marker = p_name; }
	String get_spawn_marker() const { return _spawn_marker; }

	void set_prompt_text(const String &p_text) { _prompt_text = p_text; }
	String get_prompt_text() const { return _prompt_text; }

	void set_room_bounds(const Rect2 &p_bounds) { _room_bounds = p_bounds; }
	Rect2 get_room_bounds() const { return _room_bounds; }

	void set_player(Node2D *p) { _player = p; }
	void set_camera(CameraRoom2D *c) { _camera = c; }

	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);
	void trigger();

protected:
	static void _bind_methods();

private:
	String _target_scene;
	String _spawn_marker = "SpawnEntrance";
	String _prompt_text = "[↑] Enter";
	Rect2 _room_bounds = Rect2(0, 0, 400, 270);

	Node2D *_player = nullptr;
	CameraRoom2D *_camera = nullptr;

	bool _player_inside = false;
	Node *_loaded_scene = nullptr;
	Portal *_entrance_portal = nullptr;
	Vector2 _saved_world_pos;

	void _enter();
	void _exit();
	void _create_exit_portal();
};

} // namespace godot

#endif // CPP_KAKI_PORTAL_H
