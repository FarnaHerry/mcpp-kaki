#include "camera_room_2d.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

    void CameraRoom2D::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_follow_target", "target"), &CameraRoom2D::set_follow_target);
        ClassDB::bind_method(D_METHOD("set_world_bounds", "bounds"), &CameraRoom2D::set_world_bounds);
        ClassDB::bind_method(D_METHOD("enter_room", "bounds"), &CameraRoom2D::enter_room);
        ClassDB::bind_method(D_METHOD("exit_room"), &CameraRoom2D::exit_room);
        ClassDB::bind_method(D_METHOD("get_mode"), &CameraRoom2D::get_mode);
    }

    void CameraRoom2D::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        set_enabled(true);
        make_current();
        set_position(Vector2(240, 135)); // center of viewport
    }

    void CameraRoom2D::_process(double p_delta) {
        if (Engine::get_singleton()->is_editor_hint() || !_follow_target)
            return;

        if (_transitioning) {
            _update_transition(p_delta);
            return;
        }

        switch (_mode) {
            case WORLD_FOLLOW:
                _update_world_follow(p_delta);
                break;
            case ROOM_LOCKED:
                _update_room_locked(p_delta);
                break;
        }
    }

    void CameraRoom2D::set_follow_target(Node2D *p_target) {
        _follow_target = p_target;
    }

    void CameraRoom2D::set_world_bounds(const Rect2 &p_bounds) {
        _world_bounds = p_bounds;
    }

    void CameraRoom2D::enter_room(const Rect2 &p_bounds) {
        room_bounds = p_bounds;
        _mode = ROOM_LOCKED;

        // Smooth transition to room center
        _transitioning = true;
        _transition_time = 0.0;
        _transition_start = get_position();
        _transition_target = room_bounds.get_center();
    }

    void CameraRoom2D::exit_room() {
        _mode = WORLD_FOLLOW;
        // Smooth transition back to player
        _transitioning = true;
        _transition_time = 0.0;
        _transition_start = get_position();
        _transition_target = _follow_target ? _follow_target->get_global_position() : get_position();
    }

    void CameraRoom2D::_update_world_follow(double p_delta) {
        Vector2 player_pos = _follow_target->get_global_position();
        Vector2 viewport_size = get_viewport_rect().size;
        Vector2 camera_pos = get_position();

        // Smooth look-ahead based on input direction
        // Read facing from player (try to get it, fallback to velocity direction)
        float target_look = 0.0f;
        Node *player = _follow_target;
        if (player) {
            // Try to get facing_direction from Player
            Variant facing = player->get("facing_direction");
            if (facing.get_type() != Variant::NIL) {
                target_look = float(facing) * look_ahead_amount;
            }
        }
        _current_look_ahead = Math::lerp(_current_look_ahead, target_look, float(p_delta * 3.0));

        // Target position: player + look-ahead
        Vector2 target = player_pos + Vector2(_current_look_ahead, 0);

        // Dead zone: only move camera if player is outside the zone.
        // Gain scales with overflow distance — at high target speed (flight)
        // the camera accelerates to match instead of lagging behind.
        float half_dz_x = dead_zone.x * 0.5f;
        float half_dz_y = dead_zone.y * 0.5f;

        if (Math::abs(target.x - camera_pos.x) > half_dz_x) {
            float edge_x = (target.x > camera_pos.x)
                ? target.x - half_dz_x
                : target.x + half_dz_x;
            float overflow = Math::abs(edge_x - camera_pos.x);
            float gain = follow_speed + overflow * 0.2f;
            camera_pos.x = Math::lerp(camera_pos.x, edge_x, float(Math::min(gain * p_delta, 1.0)));
        }

        if (Math::abs(target.y - camera_pos.y) > half_dz_y) {
            float edge_y = (target.y > camera_pos.y)
                ? target.y - half_dz_y
                : target.y + half_dz_y;
            float overflow = Math::abs(edge_y - camera_pos.y);
            float gain = follow_speed + overflow * 0.2f;
            camera_pos.y = Math::lerp(camera_pos.y, edge_y, float(Math::min(gain * p_delta, 1.0)));
        }

        set_position(camera_pos);
    }

    void CameraRoom2D::_update_room_locked(double p_delta) {
        Vector2 player_pos = _follow_target->get_global_position();
        Vector2 viewport_size = get_viewport_rect().size;
        float half_w = viewport_size.x * 0.5f;
        float half_h = viewport_size.y * 0.5f;

        // Target follows player within room bounds
        Vector2 target = player_pos;

        // Clamp to room bounds
        if (room_bounds.size.x > viewport_size.x) {
            target.x = Math::clamp(target.x,
                                   room_bounds.position.x + half_w,
                                   room_bounds.position.x + room_bounds.size.x - half_w);
        } else {
            target.x = room_bounds.get_center().x;
        }

        if (room_bounds.size.y > viewport_size.y) {
            target.y = Math::clamp(target.y,
                                   room_bounds.position.y + half_h,
                                   room_bounds.position.y + room_bounds.size.y - half_h);
        } else {
            target.y = room_bounds.get_center().y;
        }

        // Smooth follow within room
        Vector2 camera_pos = get_position();
        camera_pos = camera_pos.lerp(target, float(Math::min(follow_speed * p_delta * 2.0, 1.0)));
        set_position(camera_pos);
    }

    void CameraRoom2D::_update_transition(double p_delta) {
        _transition_time += p_delta;
        float t = Math::clamp(float(_transition_time / room_transition_duration), 0.0f, 1.0f);
        // Ease in-out
        t = t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;

        Vector2 pos = _transition_start.lerp(_transition_target, t);
        set_position(pos);

        if (_transition_time >= room_transition_duration) {
            _transitioning = false;
            set_position(_transition_target);
        }
    }

} // namespace godot
