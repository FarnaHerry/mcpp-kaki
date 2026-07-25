#ifndef CPP_KAKI_PORTAL_H
#define CPP_KAKI_PORTAL_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

    class CameraRoom2D;
    class Node2D;

    // Self-contained room transition portal. Place anywhere, press F to enter/exit.
    //
    // Composition pattern: each Portal owns its scene lifecycle.
    //   - Entrance portal: target_scene set → loads scene on trigger
    //   - Exit portal: created automatically inside the room → delegates to entrance
    //
    // Usage from GDScript:
    //   var p = ClassDB.instantiate("Portal")
    //   p.target_scene = "res://scenes/rooms/town.tscn"
    //   p.prompt_text = "[X] Enter"
    //   p.call("set_player", player)
    //   p.call("set_camera", camera)
    //   add_child(p)
    class Portal : public Area2D {
        GDCLASS(Portal, Area2D);

    public:
        // ---- Configuration (set before or after adding to scene) ----
        void set_target_scene(const String &p_path) { _target_scene = p_path; }
        String get_target_scene() const { return _target_scene; }

        void set_spawn_marker(const String &p_name) { _spawn_marker = p_name; }
        String get_spawn_marker() const { return _spawn_marker; }

        void set_prompt_text(const String &p_text) { _prompt_text = p_text; }
        String get_prompt_text() const { return _prompt_text; }

        void set_room_bounds(const Rect2 &p_bounds) { _room_bounds = p_bounds; }
        Rect2 get_room_bounds() const { return _room_bounds; }

        // ---- Dependencies (inject via GDScript call or C++) ----
        void set_player(Node2D *p) { _player = p; }
        void set_camera(CameraRoom2D *c) { _camera = c; }

        // ---- Godot lifecycle ----
        void _ready() override;
        void _process(double p_delta) override;
        void _on_body_entered(Node2D *p_body);
        void _on_body_exited(Node2D *p_body);

        // Trigger the portal (called on F press when player is inside)
        void trigger();

        // Signal: emitted when prompt should be shown/hidden
        // portal_prompt(text: String, show: bool)

    protected:
        static void _bind_methods();

    private:
        // Configuration
        String _target_scene;
        String _spawn_marker = "SpawnEntrance";
        String _prompt_text = "[X] Enter";
        Rect2 _room_bounds = Rect2(0, 0, 400, 270);

        // Dependencies
        Node2D *_player = nullptr;
        CameraRoom2D *_camera = nullptr;

        // State
        bool _player_inside = false;
        Node *_loaded_scene = nullptr;
        Portal *_entrance_portal = nullptr; // set on exit portals to delegate back
        Vector2 _saved_world_pos;

        void _enter();
        void _exit();
        void _create_exit_portal();
    };

} // namespace godot

#endif // CPP_KAKI_PORTAL_H
