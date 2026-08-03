// mcpp-kaki utils module implementation unit.
// Implements SignalBus / Localization / g_localization declared in utils.cppm.
module;

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

module mcpp_kaki.utils;

namespace godot {

// ---------------- SignalBus ----------------

SignalBus *SignalBus::_singleton = nullptr;

void SignalBus::_ready() {
	_singleton = this;
}

void SignalBus::_bind_methods() {
	// ---- Player signals ----
	ADD_SIGNAL(MethodInfo("player_health_changed",
	                      PropertyInfo(Variant::FLOAT, "current"),
	                      PropertyInfo(Variant::FLOAT, "max")));
	ADD_SIGNAL(MethodInfo("player_damaged",
	                      PropertyInfo(Variant::FLOAT, "amount"),
	                      PropertyInfo(Variant::OBJECT, "source")));
	ADD_SIGNAL(MethodInfo("player_died"));
	ADD_SIGNAL(MethodInfo("player_respawned"));

	// ---- Combat signals ----
	ADD_SIGNAL(MethodInfo("enemy_killed",
	                      PropertyInfo(Variant::OBJECT, "enemy"),
	                      PropertyInfo(Variant::OBJECT, "killer")));
	ADD_SIGNAL(MethodInfo("combo_changed",
	                      PropertyInfo(Variant::INT, "count")));
	ADD_SIGNAL(MethodInfo("combo_ended",
	                      PropertyInfo(Variant::INT, "final_count")));
	ADD_SIGNAL(MethodInfo("boss_died"));
	// 伤害结算广播（伤害数字显示等）。is_player_victim=true 表示玩家挨打。
	ADD_SIGNAL(MethodInfo("boss_fight_update", PropertyInfo(Variant::STRING, "name"),
	                      PropertyInfo(Variant::FLOAT, "current"), PropertyInfo(Variant::FLOAT, "max")));
	ADD_SIGNAL(MethodInfo("boss_fight_ended"));
	ADD_SIGNAL(MethodInfo("law_power_changed", PropertyInfo(Variant::FLOAT, "current"), PropertyInfo(Variant::FLOAT, "max")));
	ADD_SIGNAL(MethodInfo("buffs_changed", PropertyInfo(Variant::ARRAY, "active"))); // BuffSystem 施加/到期/清空
	ADD_SIGNAL(MethodInfo("skill_page_changed", PropertyInfo(Variant::INT, "page")));
	ADD_SIGNAL(MethodInfo("damage_dealt",
	                      PropertyInfo(Variant::VECTOR2, "world_pos"),
	                      PropertyInfo(Variant::FLOAT, "amount"),
	                      PropertyInfo(Variant::BOOL, "is_player_victim")));

	// ---- Cultivation signals ----
	ADD_SIGNAL(MethodInfo("spiritual_energy_changed",
	                      PropertyInfo(Variant::INT, "current"),
	                      PropertyInfo(Variant::INT, "max"),
	                      PropertyInfo(Variant::FLOAT, "progress")));
	ADD_SIGNAL(MethodInfo("mana_changed",
	                      PropertyInfo(Variant::FLOAT, "current"),
	                      PropertyInfo(Variant::FLOAT, "max")));
	ADD_SIGNAL(MethodInfo("realm_changed",
	                      PropertyInfo(Variant::INT, "old_realm"),
	                      PropertyInfo(Variant::INT, "new_realm"),
	                      PropertyInfo(Variant::STRING, "realm_name")));
	ADD_SIGNAL(MethodInfo("immortal_type_changed",
	                      PropertyInfo(Variant::INT, "type"),
	                      PropertyInfo(Variant::STRING, "type_name")));
	ADD_SIGNAL(MethodInfo("sect_changed",
	                      PropertyInfo(Variant::INT, "sect"),
	                      PropertyInfo(Variant::STRING, "sect_name")));

	// ---- Breakthrough (机缘) signals ----
	ADD_SIGNAL(MethodInfo("breakthrough_requested"));
	ADD_SIGNAL(MethodInfo("breakthrough_event_started",
	                      PropertyInfo(Variant::INT, "event_id")));
	ADD_SIGNAL(MethodInfo("breakthrough_event_finished",
	                      PropertyInfo(Variant::INT, "event_id"),
	                      PropertyInfo(Variant::BOOL, "success")));

	// ---- Game state signals ----
	ADD_SIGNAL(MethodInfo("game_paused"));
	ADD_SIGNAL(MethodInfo("game_resumed"));
	ADD_SIGNAL(MethodInfo("checkpoint_set",
	                      PropertyInfo(Variant::VECTOR2, "position"),
	                      PropertyInfo(Variant::STRING, "scene_path")));
	ADD_SIGNAL(MethodInfo("scene_transition_start",
	                      PropertyInfo(Variant::STRING, "from_scene"),
	                      PropertyInfo(Variant::STRING, "to_scene")));
	ADD_SIGNAL(MethodInfo("scene_transition_end",
	                      PropertyInfo(Variant::STRING, "scene_path")));
	ADD_SIGNAL(MethodInfo("continent_changed",
	                      PropertyInfo(Variant::STRING, "continent_id"),
	                      PropertyInfo(Variant::STRING, "continent_name")));

	// ---- Interaction signals ----
	ADD_SIGNAL(MethodInfo("interaction_prompt",
	                      PropertyInfo(Variant::STRING, "text"),
	                      PropertyInfo(Variant::BOOL, "show")));
	ADD_SIGNAL(MethodInfo("item_picked_up",
	                      PropertyInfo(Variant::STRING, "item_id"),
	                      PropertyInfo(Variant::INT, "quantity")));
	ADD_SIGNAL(MethodInfo("item_used",
	                      PropertyInfo(Variant::STRING, "item_id"),
	                      PropertyInfo(Variant::INT, "quantity")));

	// ---- Save / Load signals ----
	ADD_SIGNAL(MethodInfo("game_saved",
	                      PropertyInfo(Variant::STRING, "slot_name")));
	ADD_SIGNAL(MethodInfo("game_loaded",
	                      PropertyInfo(Variant::STRING, "slot_name")));
	ADD_SIGNAL(MethodInfo("save_error",
	                      PropertyInfo(Variant::STRING, "message")));

	// ---- Localization ----
	ADD_SIGNAL(MethodInfo("language_changed",
	                      PropertyInfo(Variant::STRING, "locale")));
}

// ---------------- Localization ----------------

Localization *g_localization = nullptr;

Localization *Localization::_singleton = nullptr;

void Localization::_ready() {
	_singleton = this;
	g_localization = this;
	_load_translations();

	// Load persisted language preference
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	if (cfg->load("user://settings.cfg") == OK) {
		String saved = cfg->get_value("language", "locale", "zh");
		_language = (saved == "en") ? "en" : "zh";
	}
}

void Localization::_load_translations() {
	_table.clear();

	String path = "res://data/locale_en.json";
	if (!FileAccess::file_exists(path)) {
		UtilityFunctions::printerr("Localization: ", path, " not found.");
		return;
	}

	String raw = FileAccess::get_file_as_string(path);
	Variant parsed = JSON::parse_string(raw);
	if (parsed.get_type() != Variant::DICTIONARY) {
		UtilityFunctions::printerr("Localization: ", path, " is not a JSON object.");
		return;
	}

	Dictionary dict = parsed;
	Array keys = dict.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		String val = dict[keys[i]];
		if (!key.is_empty() && !val.is_empty()) {
			_table[key] = val;
		}
	}

	UtilityFunctions::print("Localization: loaded ", _table.size(), " entries from ", path);
}

String Localization::translate(const String &p_key) const {
	// In Chinese mode, always return empty → LOC() falls back to Chinese
	if (_language == "zh") return String();

	HashMap<String, String>::ConstIterator it = _table.find(p_key);
	if (it != _table.end()) {
		return it->value;
	}
	return String(); // not found → LOC() fallback handles it
}

void Localization::set_language(const String &p_lang) {
	if (_language == p_lang) return;
	_language = p_lang;

	// Persist
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->set_value("language", "locale", _language);
	cfg->save("user://settings.cfg");

	// Notify UI
	SignalBus *sb = SignalBus::get_singleton();
	if (sb) {
		sb->emit_signal("language_changed", p_lang);
	}
}

void Localization::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_language", "lang"), &Localization::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &Localization::get_language);
}

} // namespace godot
