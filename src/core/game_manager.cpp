#include "game_manager.h"

#include "../nodes/player.h"
#include "../nodes/camera_room_2d.h"
#include "../utils/signal_bus.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

GameManager *GameManager::_singleton = nullptr;

void GameManager::_ready() {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    _singleton = this;
    _signal_bus = SignalBus::get_singleton();
}

void GameManager::_bind_methods() {
    // Use int for set_game_state / get_game_state since Godot binder can't handle enum types
    ClassDB::bind_method(D_METHOD("set_game_state", "state"), &GameManager::set_game_state_int);
    ClassDB::bind_method(D_METHOD("get_game_state"), &GameManager::get_game_state_int);
    ClassDB::bind_method(D_METHOD("pause_game"), &GameManager::pause_game);
    ClassDB::bind_method(D_METHOD("resume_game"), &GameManager::resume_game);
    ClassDB::bind_method(D_METHOD("set_player", "player"), &GameManager::set_player);
    ClassDB::bind_method(D_METHOD("get_player"), &GameManager::get_player);
    ClassDB::bind_method(D_METHOD("set_camera", "camera"), &GameManager::set_camera);
    ClassDB::bind_method(D_METHOD("get_camera"), &GameManager::get_camera);
    ClassDB::bind_method(D_METHOD("set_checkpoint", "position", "scene_path"),
                         &GameManager::set_checkpoint, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("get_respawn_position"), &GameManager::get_respawn_position);
    ClassDB::bind_method(D_METHOD("trigger_respawn"), &GameManager::trigger_respawn);
    ClassDB::bind_method(D_METHOD("on_player_died"), &GameManager::on_player_died);
    ClassDB::bind_method(D_METHOD("request_scene_change", "scene_path", "spawn_pos"),
                         &GameManager::request_scene_change);
    ClassDB::bind_method(D_METHOD("get_kill_count"), &GameManager::get_kill_count);
    ClassDB::bind_method(D_METHOD("increment_kill_count"), &GameManager::increment_kill_count);

    ADD_SIGNAL(MethodInfo("respawn_triggered",
                          PropertyInfo(Variant::VECTOR2, "position")));
}

// ============================================================
// Game State
// ============================================================

void GameManager::set_game_state(GameState p_state) {
    if (_state == p_state)
        return;
    _state = p_state;
}

void GameManager::set_game_state_int(int p_state) {
    set_game_state(static_cast<GameState>(p_state));
}

int GameManager::get_game_state_int() const {
    return static_cast<int>(_state);
}

void GameManager::pause_game() {
    if (_state != STATE_PLAYING)
        return;
    _state = STATE_PAUSED;
    get_tree()->set_pause(true);
    if (_signal_bus) {
        _signal_bus->emit_signal("game_paused");
    }
}

void GameManager::resume_game() {
    if (_state != STATE_PAUSED)
        return;
    _state = STATE_PLAYING;
    get_tree()->set_pause(false);
    if (_signal_bus) {
        _signal_bus->emit_signal("game_resumed");
    }
}

// ============================================================
// References
// ============================================================

void GameManager::set_player(Player *p_player) {
    _player = p_player;
}

void GameManager::set_camera(CameraRoom2D *p_camera) {
    _camera = p_camera;
}

// ============================================================
// Checkpoint System
// ============================================================

void GameManager::set_checkpoint(const Vector2 &p_position, const String &p_scene_path) {
    _respawn_pos = p_position;
    _respawn_scene = p_scene_path;
    _has_checkpoint = true;

    if (_signal_bus) {
        _signal_bus->emit_signal("checkpoint_set", p_position, p_scene_path);
    }
}

// ============================================================
// Death & Respawn
// ============================================================

void GameManager::on_player_died() {
    if (_state == STATE_GAME_OVER)
        return;

    _state = STATE_GAME_OVER;

    // Brief pause before respawn
    get_tree()->set_pause(true);

    // Use a timer to delay respawn
    Timer *respawn_timer = memnew(Timer);
    respawn_timer->set_name("RespawnTimer");
    respawn_timer->set_one_shot(true);
    respawn_timer->set_wait_time(_respawn_delay);
    respawn_timer->connect("timeout", Callable(this, "trigger_respawn"));
    add_child(respawn_timer);
    respawn_timer->start();
}

void GameManager::trigger_respawn() {
    // Unpause
    get_tree()->set_pause(false);

    if (!_player)
        return;

    // Respawn at checkpoint or initial position
    if (_has_checkpoint) {
        _player->set_global_position(_respawn_pos);
    }

    // Reset player state
    _player->set("velocity", Vector2(0, 0));
    _player->current_health = _player->max_health;
    _state = STATE_PLAYING;

    // Clean up respawn timer
    Node *timer = get_node_or_null("RespawnTimer");
    if (timer) {
        timer->queue_free();
    }

    if (_signal_bus) {
        _signal_bus->emit_signal("player_respawned");
        _signal_bus->emit_signal("player_health_changed",
                                _player->current_health, _player->max_health);
    }

    emit_signal("respawn_triggered", _player->get_global_position());
}

// ============================================================
// Scene Transitions
// ============================================================

void GameManager::request_scene_change(const String &p_scene_path, const Vector2 &p_spawn_pos) {
    _respawn_pos = p_spawn_pos;
    _respawn_scene = p_scene_path;

    if (_signal_bus) {
        _signal_bus->emit_signal("scene_transition_start",
                                get_tree()->get_current_scene()->get_scene_file_path(),
                                p_scene_path);
    }

    // Delegate to Godot's scene changer
    get_tree()->change_scene_to_file(p_scene_path);

    if (_signal_bus) {
        _signal_bus->emit_signal("scene_transition_end", p_scene_path);
    }
}

// ============================================================
// Kill tracking
// ============================================================

void GameManager::increment_kill_count() {
    _kill_count++;
}

} // namespace godot
