#include "game_manager.h"
#include "save_system.h"

#include "../combat/skill_system.h"
#include "../cultivation/artifact_system.h"

#include "../cultivation/ability_manager.h"
#include "../cultivation/cultivation_system.h"
#include "../nodes/camera_room_2d.h"
#include "../nodes/player.h"
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

	// Create SaveSystem
	_save_system = memnew(SaveSystem);
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

	// Save / Load
	ClassDB::bind_method(D_METHOD("save_game", "slot_name"), &GameManager::save_game, DEFVAL("auto"));
	ClassDB::bind_method(D_METHOD("load_game", "slot_name"), &GameManager::load_game, DEFVAL("auto"));
	ClassDB::bind_method(D_METHOD("has_save", "slot_name"), &GameManager::has_save, DEFVAL("auto"));

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

	// Auto-save on checkpoint
	save_game("auto");
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
	respawn_timer->set_process_mode(Node::PROCESS_MODE_ALWAYS); // 世界已暂停，计时器必须继续走
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
// Save / Load
// ============================================================

Dictionary GameManager::collect_save_data() const {
	Dictionary data;

	// ---- Checkpoint ----
	{
		Dictionary cp;
		cp["position_x"] = _respawn_pos.x;
		cp["position_y"] = _respawn_pos.y;
		cp["scene_path"] = _respawn_scene;
		cp["has_checkpoint"] = _has_checkpoint;
		data["checkpoint"] = cp;
	}

	// ---- Player ----
	if (_player) {
		Dictionary pd;
		pd["health"] = _player->current_health;
		pd["max_health"] = _player->max_health;
		pd["position_x"] = _player->get_global_position().x;
		pd["position_y"] = _player->get_global_position().y;
		if (_player->get_gongfa()) {
			pd["gongfa"] = _player->get_gongfa()->save_to_dict();
		}
		if (_player->get_skills()) {
			pd["skills"] = _player->get_skills()->save_to_dict();
		}
		if (_player->get_artifacts()) {
			pd["artifacts"] = _player->get_artifacts()->save_to_dict();
		if (_player->get_buffs()) {
			pd["buffs"] = _player->get_buffs()->save_to_dict();
		}
		{
			Array bar;
			for (int i = 0; i < Player::CONSUMABLE_BAR_SLOTS; i++) {
				bar.push_back(String(_player->get_consumable_bar_slot(i)));
			}
			pd["consumable_bar"] = bar;
		}
		}
		data["player"] = pd;

		// ---- Cultivation ----
		if (_player->get_cultivation()) {
			Dictionary cd;
			cd["realm"] = _player->get_cultivation()->get_realm_index();
			cd["spiritual_energy"] = _player->get_cultivation()->get_spiritual_energy();
			cd["xianyuan"] = _player->get_cultivation()->get_xianyuan();
			cd["mana"] = _player->get_cultivation()->get_mana();
			cd["law_power"] = _player->get_cultivation()->get_law_power();
			cd["immortal_type"] = (int)_player->get_cultivation()->get_immortal_type();
			cd["sect"] = (int)_player->get_cultivation()->get_sect();
			cd["origin"] = (int)_player->get_cultivation()->get_origin();
			cd["buddhist_rank"] = (int)_player->get_cultivation()->get_buddhist_rank();
			cd["focus"] = (int)_player->get_cultivation()->get_focus();
			cd["hunyuan"] = _player->get_cultivation()->is_hunyuan();
			data["cultivation"] = cd;

			// ---- 本命法宝 ----
			Dictionary bd;
			bd["item"] = String(_player->get_benming_artifact());
			bd["nurture"] = _player->get_benming_nurture();
			bd["awakened"] = _player->is_benming_awakened();
			data["benming"] = bd;
		}

		// ---- Abilities ----
		if (_player->get_ability_manager()) {
			Dictionary ad;
			ad["unlocked"] = _player->get_ability_manager()->get_unlocked_list();
			data["abilities"] = ad;
		}

		// ---- Inventory ----
		if (_player->get_inventory()) {
			Dictionary inv;
			for (int i = 0; i < _player->get_inventory()->get_capacity(); i++) {
				Dictionary slot = _player->get_inventory()->get_slot(i);
				if (!slot.is_empty()) {
					inv[String::num_int64(i)] = slot;
				}
			}
			inv["_unlimited"] = _player->get_inventory()->is_unlimited();
			data["inventory"] = inv;

		// ---- Equipment ----
		Dictionary eq;
		for (int i = 0; i < 3; i++) {
			StringName item_id = _player->get_equipment_in_slot(i);
			eq[String::num_int64(i)] = String(item_id);
		}
		data["equipment"] = eq;
	}
	}

	// ---- Progress ----
	{
		Dictionary pg;
		pg["kill_count"] = _kill_count;
		data["progress"] = pg;
	}

	return data;
}

void GameManager::save_game(const String &p_slot_name) const {
	if (!_save_system) {
		return;
	}

	Dictionary save_data = collect_save_data();
	_save_system->save_game(p_slot_name, save_data);
}

void GameManager::load_game(const String &p_slot_name) {
	if (!_save_system || !_player) {
		return;
	}

	Dictionary data = _save_system->load_game(p_slot_name);
	if (data.is_empty()) {
		return;
	}

	// ---- Restore checkpoint ----
	Dictionary cp = data.get("checkpoint", Dictionary());
	if (!cp.is_empty()) {
		_respawn_pos = Vector2(
			float(cp.get("position_x", 0.0)),
			float(cp.get("position_y", 0.0)));
		_respawn_scene = String(cp.get("scene_path", ""));
		_has_checkpoint = bool(cp.get("has_checkpoint", false));
	}

	// ---- Restore player ----
	// 注意：生命在境界恢复之后再回填——set_realm 触发"突破回满"会覆盖这里的值
	float saved_health = -1.0f;
	Dictionary pd = data.get("player", Dictionary());
	if (!pd.is_empty()) {
		saved_health = float(pd.get("health", -1.0f));
		float px = float(pd.get("position_x", _player->get_global_position().x));
		float py = float(pd.get("position_y", _player->get_global_position().y));
		_player->set_global_position(Vector2(px, py));
	}

	// ---- Restore cultivation ----
	Dictionary cd = data.get("cultivation", Dictionary());
	if (!cd.is_empty() && _player->get_cultivation()) {
		_player->get_cultivation()->set_realm(int(cd.get("realm", 0)));
		_player->get_cultivation()->set_spiritual_energy(
			int64_t(cd.get("spiritual_energy", 0)));
		_player->get_cultivation()->set_xianyuan(
			int64_t(cd.get("xianyuan", 0)));
		_player->get_cultivation()->set_mana(
			double(cd.get("mana", 0.0)));
		_player->get_cultivation()->set_law_power(
			double(cd.get("law_power", 0.0)));
		_player->get_cultivation()->set_immortal_type(
			int(cd.get("immortal_type", 0)));
		_player->get_cultivation()->set_sect(
			int(cd.get("sect", 0)));
		_player->get_cultivation()->set_origin(
			int(cd.get("origin", 0)));
		_player->get_cultivation()->set_buddhist_rank(
			int(cd.get("buddhist_rank", 0)));
		_player->get_cultivation()->set_focus(
			int(cd.get("focus", 0)));
		_player->get_cultivation()->set_hunyuan(
			bool(cd.get("hunyuan", false)));
	}

	// ---- 回填存档生命（在境界恢复之后，避免被突破回满覆盖）----
	{
		_player->max_health = float(_player->get_cultivation()->get_max_health());
		if (saved_health >= 0.0f) {
			_player->current_health = Math::min(saved_health, _player->max_health);
		}
		if (_signal_bus) {
			_signal_bus->emit_signal("player_health_changed",
			                        _player->current_health, _player->max_health);
		}
	}

	// ---- Restore 本命法宝 ----
	Dictionary bd = data.get("benming", Dictionary());
	if (!bd.is_empty()) {
		_player->set_benming_artifact(StringName(String(bd.get("item", ""))));
		_player->nurture_benming(float(bd.get("nurture", 0.0f)));
		if (bool(bd.get("awakened", false))) {
			_player->awaken_benming_artifact();
		}
	}

	// ---- Restore abilities ----
	Dictionary ad = data.get("abilities", Dictionary());
	if (!ad.is_empty() && _player->get_ability_manager()) {
		String list = ad.get("unlocked", "");
		if (!list.is_empty()) {
			PackedStringArray abilities = list.split(",");
			for (int64_t i = 0; i < abilities.size(); i++) {
				String name = abilities[i].strip_edges();
				if (!name.is_empty()) {
					_player->get_ability_manager()->unlock_ability(name);
				}
			}
		}
		_player->get_ability_manager()->check_realm_unlocks();
	}

	// ---- Restore inventory ----
	Dictionary inv = data.get("inventory", Dictionary());
	if (!inv.is_empty() && _player->get_inventory()) {
		// 先恢复纳戒容量，再放回格子
		if (bool(inv.get("_unlimited", false))) {
			_player->get_inventory()->unlock_unlimited();
		}
		Array keys = inv.keys();
		for (int i = 0; i < keys.size(); i++) {
			String key = String(keys[i]);
			if (key.begins_with("_")) continue;
			int slot_idx = key.to_int();
			Dictionary slot_data = inv.get(keys[i], Dictionary());
			if (!slot_data.is_empty()) {
				StringName item_id = slot_data.get("id", StringName());
				int qty = int(slot_data.get("quantity", 0));
				_player->get_inventory()->set_slot(slot_idx, item_id, qty);
			}
		}
	}

	// ---- Restore progress ----
	Dictionary pg = data.get("progress", Dictionary());
	if (!pg.is_empty()) {
		_kill_count = int(pg.get("kill_count", 0));
	}
}

bool GameManager::has_save(const String &p_slot_name) const {
	return _save_system && _save_system->has_save(p_slot_name);
}

// ============================================================
// Kill tracking
// ============================================================

void GameManager::increment_kill_count() {
	_kill_count++;
}

} // namespace godot
