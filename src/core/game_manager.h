#ifndef CPP_KAKI_GAME_MANAGER_H
#define CPP_KAKI_GAME_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class Player;
class CameraRoom2D;
class SignalBus;

// Global game manager — autoload singleton.
// Owns high-level game state: pause, death/respawn, checkpoints, scene flow.
//
// Usage (via GDScript or C++):
//   GameManager *gm = GameManager::get_singleton();
//   gm->set_player(player);
//   gm->set_camera(camera);
//   gm->set_checkpoint(pos, scene_path);
class GameManager : public Node {
    GDCLASS(GameManager, Node);

public:
    enum GameState {
        STATE_PLAYING,
        STATE_PAUSED,
        STATE_GAME_OVER,
        STATE_TRANSITIONING,
    };

    static GameManager *get_singleton() { return _singleton; }

    // ---- Game state ----
    GameState get_game_state() const { return _state; }
    void set_game_state(GameState p_state);
    // Int wrappers for Godot binding (enum not supported directly)
    void set_game_state_int(int p_state);
    int get_game_state_int() const;
    bool is_playing() const { return _state == STATE_PLAYING; }
    bool is_paused() const { return _state == STATE_PAUSED; }

    void pause_game();
    void resume_game();

    // ---- Player / Camera refs ----
    void set_player(Player *p_player);
    Player *get_player() const { return _player; }
    void set_camera(CameraRoom2D *p_camera);
    CameraRoom2D *get_camera() const { return _camera; }

    // ---- Checkpoint system ----
    void set_checkpoint(const Vector2 &p_position, const String &p_scene_path = "");
    Vector2 get_respawn_position() const { return _respawn_pos; }
    String get_respawn_scene() const { return _respawn_scene; }
    bool has_checkpoint() const { return _has_checkpoint; }

    // ---- Respawn ----
    void trigger_respawn();
    float get_respawn_delay() const { return _respawn_delay; }
    void set_respawn_delay(float p_delay) { _respawn_delay = p_delay; }

    // ---- Death / game over ----
    void on_player_died();

    // ---- Scene transitions ----
    void request_scene_change(const String &p_scene_path, const Vector2 &p_spawn_pos);

    // ---- Enemy kill tracking ----
    int get_kill_count() const { return _kill_count; }
    void increment_kill_count();

    void _ready() override;

protected:
    static void _bind_methods();

private:
    static GameManager *_singleton;

    GameState _state = STATE_PLAYING;
    Player *_player = nullptr;
    CameraRoom2D *_camera = nullptr;
    SignalBus *_signal_bus = nullptr;

    // Checkpoint
    Vector2 _respawn_pos;
    String _respawn_scene;
    bool _has_checkpoint = false;

    // Config
    float _respawn_delay = 1.5f;
    int _kill_count = 0;
};

} // namespace godot

#endif // CPP_KAKI_GAME_MANAGER_H
