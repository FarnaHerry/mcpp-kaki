#include "portal.h"
#include "camera_room_2d.h"
#include "../nodes/player.h"
#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/marker2d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.utils;
namespace godot {

    void Portal::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_target_scene", "path"), &Portal::set_target_scene);
        ClassDB::bind_method(D_METHOD("get_target_scene"), &Portal::get_target_scene);
        ClassDB::bind_method(D_METHOD("set_spawn_marker", "name"), &Portal::set_spawn_marker);
        ClassDB::bind_method(D_METHOD("get_spawn_marker"), &Portal::get_spawn_marker);
        ClassDB::bind_method(D_METHOD("set_prompt_text", "text"), &Portal::set_prompt_text);
        ClassDB::bind_method(D_METHOD("get_prompt_text"), &Portal::get_prompt_text);
        ClassDB::bind_method(D_METHOD("set_room_bounds", "bounds"), &Portal::set_room_bounds);
        ClassDB::bind_method(D_METHOD("get_room_bounds"), &Portal::get_room_bounds);
        ClassDB::bind_method(D_METHOD("set_player", "player"), &Portal::set_player);
        ClassDB::bind_method(D_METHOD("set_camera", "camera"), &Portal::set_camera);
        ClassDB::bind_method(D_METHOD("trigger"), &Portal::trigger);
        ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &Portal::_on_body_entered);
        ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &Portal::_on_body_exited);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_scene"), "set_target_scene", "get_target_scene");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "spawn_marker"), "set_spawn_marker", "get_spawn_marker");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "prompt_text"), "set_prompt_text", "get_prompt_text");
        ADD_PROPERTY(PropertyInfo(Variant::RECT2, "room_bounds"), "set_room_bounds", "get_room_bounds");

        ADD_SIGNAL(MethodInfo("portal_prompt",
                              PropertyInfo(Variant::STRING, "text"),
                              PropertyInfo(Variant::BOOL, "show")));
    }

    // ============================================================
    // Lifecycle
    // ============================================================

    void Portal::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        // Only detect Player (layer 3)
        set_collision_mask(0);
        set_collision_mask_value(3, true);
        set_monitoring(true);

        connect("body_entered", Callable(this, "_on_body_entered"));
        connect("body_exited", Callable(this, "_on_body_exited"));
        set_process(true);
    }

    void Portal::_process(double p_delta) {
        if (!_player_inside || !_player)
            return;
        if (Input::get_singleton()->is_action_just_pressed("interact")) {
            trigger();
        }
    }

    // ============================================================
    // Body detection
    // ============================================================

    void Portal::_on_body_entered(Node2D *p_body) {
        if (p_body->get_name() != StringName("Player"))
            return;
        _player_inside = true;
        if (!_player)
            _player = p_body;
        emit_signal("portal_prompt", _prompt_text, true);

    }

    void Portal::_on_body_exited(Node2D *p_body) {
        if (p_body->get_name() != StringName("Player"))
            return;
        _player_inside = false;
        emit_signal("portal_prompt", "", false);

    }

    // ============================================================
    // Transition logic
    // ============================================================

    void Portal::trigger() {
        if (_entrance_portal) {
            _entrance_portal->trigger();
        } else if (_loaded_scene) {
            _exit();
        } else if (!_target_scene.is_empty()) {
            _enter();
        }
    }

    void Portal::_enter() {
        ERR_FAIL_COND(_target_scene.is_empty());
        if (!_player) return;

        // Save world position for return
        _saved_world_pos = _player->get_global_position();

        // Load target scene
        Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(_target_scene);
        ERR_FAIL_COND(scene.is_null());
        _loaded_scene = scene->instantiate();
        ERR_FAIL_NULL(_loaded_scene);

        Node *root = get_tree()->get_current_scene();
        if (root) root->add_child(_loaded_scene);

        // Move player into the room
        Node *parent = _player->get_parent();
        if (parent) parent->remove_child(_player);
        _loaded_scene->add_child(_player);

        // Place at spawn marker
        Vector2 spawn = _room_bounds.position;
        Marker2D *marker = Object::cast_to<Marker2D>(_loaded_scene->get_node_or_null(_spawn_marker));
        if (marker) spawn = marker->get_position();
        _player->set_position(spawn);
        _player->set("velocity", Vector2(0, 0));

        // Lock camera to room
        if (_camera) _camera->enter_room(_room_bounds);

        // Create exit portal inside the room
        _create_exit_portal();

        _prompt_text = LOC("[X] 离开");
    }

    void Portal::_exit() {
        if (!_loaded_scene || !_player) return;

        // Move player back to world
        Node *parent = _player->get_parent();
        if (parent) parent->remove_child(_player);
        Node *root = get_tree()->get_current_scene();
        if (root) root->add_child(_player);

        _player->set_global_position(_saved_world_pos);
        _player->set("velocity", Vector2(0, 0));

        // Remove room scene (exit portal is cleaned up with it)
        _loaded_scene->queue_free();
        _loaded_scene = nullptr;

        // Unlock camera
        if (_camera) _camera->exit_room();

        _prompt_text = LOC("[X] 进入");
    }

    void Portal::_create_exit_portal() {
        Portal *ep = memnew(Portal);
        ep->set_name("ExitPortal");
        ep->_prompt_text = LOC("[X] 离开");
        ep->_entrance_portal = this; // delegate back
        ep->_player = _player;
        ep->_camera = _camera;
        ep->set_collision_mask(0);
        ep->set_collision_mask_value(3, true);
        ep->set_position(Vector2(_room_bounds.size.x / 2.0f, _room_bounds.size.y - 50.0f));

        CollisionShape2D *shape = memnew(CollisionShape2D);
        Ref<RectangleShape2D> rect;
        rect.instantiate();
        rect->set_size(Vector2(40, 80));
        shape->set_shape(rect);
        ep->add_child(shape);

        _loaded_scene->add_child(ep);
    }

} // namespace godot
