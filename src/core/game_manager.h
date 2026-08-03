#ifndef CPP_KAKI_GAME_MANAGER_H
#define CPP_KAKI_GAME_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

import mcpp_kaki.utils;
import mcpp_kaki.core;

namespace godot {

class Player;
class CameraRoom2D;

// Global game manager — autoload singleton.
// Owns high-level game state: pause, death/respawn, checkpoints, scene flow, save/load.
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
	String get_travel_dest() const { return _travel_dest; }              // 云海强渡目的洲（cp.travel_dest 持久化）
	void set_travel_dest(const String &p_id) { _travel_dest = p_id; }
	bool has_checkpoint() const { return _has_checkpoint; }

	// ---- Respawn ----
	void trigger_respawn();
	float get_respawn_delay() const { return _respawn_delay; }
	void set_respawn_delay(float p_delay) { _respawn_delay = p_delay; }

	// ---- Death / game over ----
	void on_player_died();

	// ---- Scene transitions ----
	void request_scene_change(const String &p_scene_path, const Vector2 &p_spawn_pos);

	// ---- Save / Load ----
	void save_game(const String &p_slot_name = "auto") const;
	void load_game(const String &p_slot_name = "auto");
	bool has_save(const String &p_slot_name = "auto") const;
	Dictionary collect_save_data() const;

	// ---- 跨场景旅行桥（design/world-map.md 洲切换）----
	// 旧场景 collect_save_data → 静态桥 → change_scene → 新场景 GameManager 应用。
	// 静态成员：场景切换后新 GameManager 实例仍能取到。
	static void set_travel_bridge(const Dictionary &p_data);
	static void set_travel_target(const Vector2 &p_spawn); // 到岸落点（不设=读档：落点/血量按存档）
	bool has_pending_bridge() const { return !_pending_bridge.is_empty(); } // bootstrap 跳过初始检查点自动存档用

	// ---- Enemy kill tracking ----
	int get_kill_count() const { return _kill_count; }
	void increment_kill_count();

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	static GameManager *_singleton;
	// 跨场景旅行桥（洲切换时暂存全量存档）——函数内静态：
	// Dictionary 全局静态会在引擎内存初始化前构造，必崩
	static Dictionary &_bridge_storage();
	static Vector2 _s_travel_spawn;
	static bool _s_has_travel_spawn;

	GameState _state = STATE_PLAYING;
	Player *_player = nullptr;
	CameraRoom2D *_camera = nullptr;
	SignalBus *_signal_bus = nullptr;
	SaveSystem *_save_system = nullptr;

	// Checkpoint
	Vector2 _respawn_pos;
	String _respawn_scene;
	String _travel_dest; // 渡海目的地（空=非渡海中）；随 checkpoint 段存取
	bool _has_checkpoint = false;

	// 旅行桥：新场景启动后待应用的全量存档 + 落点
	Dictionary _pending_bridge;
	Vector2 _travel_spawn;
	bool _has_travel_target = false;

	void _apply_save_dict(const Dictionary &p_data); // load_game / 旅行桥共用的恢复逻辑

	// Config
	float _respawn_delay = 1.5f;
	int _kill_count = 0;
};

} // namespace godot

#endif // CPP_KAKI_GAME_MANAGER_H
