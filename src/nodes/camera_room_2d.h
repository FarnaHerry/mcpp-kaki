#ifndef CPP_KAKI_CAMERA_ROOM_2D_H
#define CPP_KAKI_CAMERA_ROOM_2D_H

#include <godot_cpp/classes/camera2d.hpp>

namespace godot {

    class CameraRoom2D : public Camera2D {
        GDCLASS(CameraRoom2D, Camera2D);

    public:
        enum CameraMode {
            WORLD_FOLLOW, // Smooth follow across open world
            ROOM_LOCKED   // Locked to specific room bounds
        };

        // Smooth follow speed (higher = snappier, 0 = instant)
        float follow_speed = 4.0f;

        // Dead zone: player can move within this rect without camera moving
        Vector2 dead_zone = Vector2(30, 16);

        // Look ahead distance in facing direction
        float look_ahead_amount = 48.0f;

        // Room bounds (used in ROOM_LOCKED mode)
        Rect2 room_bounds = Rect2(0, 0, 480, 270);

        // Room transition duration
        float room_transition_duration = 0.5f;

        int get_mode() const { return (int)_mode; }

        void set_follow_target(Node2D *p_target);
        void set_world_bounds(const Rect2 &p_bounds);

        // Switch to room-locked mode
        void enter_room(const Rect2 &p_bounds);

        // Return to world follow mode
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

        // Room transition state
        bool _transitioning = false;
        double _transition_time = 0.0;
        Vector2 _transition_start;
        Vector2 _transition_target;

        void _update_world_follow(double p_delta);
        void _update_room_locked(double p_delta);
        void _update_transition(double p_delta);
    };

} // namespace godot

#endif // CPP_KAKI_CAMERA_ROOM_2D_H
