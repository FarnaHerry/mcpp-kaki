module;

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

module mcpp_kaki.core;
namespace godot {

void DataLoader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_item", "id"), &DataLoader::get_item);
	ClassDB::bind_method(D_METHOD("get_all_items"), &DataLoader::get_all_items);
	ClassDB::bind_method(D_METHOD("get_skill", "id"), &DataLoader::get_skill);
	ClassDB::bind_method(D_METHOD("get_all_skills"), &DataLoader::get_all_skills);
	ClassDB::bind_method(D_METHOD("get_skills_of_type", "type"), &DataLoader::get_skills_of_type);
	ClassDB::bind_method(D_METHOD("get_buff", "id"), &DataLoader::get_buff);
	ClassDB::bind_method(D_METHOD("get_all_buffs"), &DataLoader::get_all_buffs);
	ClassDB::bind_method(D_METHOD("get_gongfa", "id"), &DataLoader::get_gongfa);
	ClassDB::bind_method(D_METHOD("get_all_gongfas"), &DataLoader::get_all_gongfas);
	ClassDB::bind_method(D_METHOD("get_sect", "id"), &DataLoader::get_sect);
	ClassDB::bind_method(D_METHOD("get_all_sects"), &DataLoader::get_all_sects);
	ClassDB::bind_method(D_METHOD("get_drop_table"), &DataLoader::get_drop_table);
	ClassDB::bind_method(D_METHOD("get_recipe", "id"), &DataLoader::get_recipe);
	ClassDB::bind_method(D_METHOD("get_all_recipes"), &DataLoader::get_all_recipes);
	ClassDB::bind_method(D_METHOD("get_all_realms"), &DataLoader::get_all_realms);
	ClassDB::bind_method(D_METHOD("get_realm_tuning"), &DataLoader::get_realm_tuning);
	ClassDB::bind_method(D_METHOD("get_all_teleports"), &DataLoader::get_all_teleports);
	ClassDB::bind_method(D_METHOD("get_all_abilities"), &DataLoader::get_all_abilities);
}

void DataLoader::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	_load_json_array("res://data/items.json", _items);
	_load_json_array("res://data/skills.json", _skills);
	_load_json_array("res://data/buffs.json", _buffs);
	_load_json_array("res://data/gongfas.json", _gongfas);
	_load_json_array("res://data/sects.json", _sects);
	_load_json_array("res://data/recipes.json", _recipes);
	_load_json_array("res://data/continents.json", _continents);

	// Events: keyed by realm index, not string id
	if (FileAccess::file_exists("res://data/events.json")) {
		String raw = FileAccess::get_file_as_string("res://data/events.json");
		Variant p = JSON::parse_string(raw);
		if (p.get_type() == Variant::ARRAY) {
			Array arr = p;
			for (int i = 0; i < arr.size(); i++) {
				Dictionary d = arr[i];
				_events[int(d["realm"])] = d;
			}
		}
	}

	// Drops structured differently (object with named arrays, not {id,...} array)
	if (FileAccess::file_exists("res://data/drops.json")) {
		String raw = FileAccess::get_file_as_string("res://data/drops.json");
		Variant p = JSON::parse_string(raw);
		if (p.get_type() == Variant::DICTIONARY) {
			_drop_table = p;
		}
	}

	// 云游阵（data/teleports.json）：数组原样保留（序=面板列表序）
	if (FileAccess::file_exists("res://data/teleports.json")) {
		String raw = FileAccess::get_file_as_string("res://data/teleports.json");
		Variant p = JSON::parse_string(raw);
		if (p.get_type() == Variant::ARRAY)
			_teleports = p;
	}

	// 能力解锁表（data/abilities.json）：数组原样保留（序=能力页显示序）
	if (FileAccess::file_exists("res://data/abilities.json")) {
		String raw = FileAccess::get_file_as_string("res://data/abilities.json");
		Variant p = JSON::parse_string(raw);
		if (p.get_type() == Variant::ARRAY)
			_abilities = p;
	}

	// Realms: object with "tuning" (global knobs) + "realms" array indexed by realm order
	if (FileAccess::file_exists("res://data/realms.json")) {
		String raw = FileAccess::get_file_as_string("res://data/realms.json");
		Variant p = JSON::parse_string(raw);
		if (p.get_type() == Variant::DICTIONARY) {
			Dictionary d = p;
			if (d.has("realms") && d["realms"].get_type() == Variant::ARRAY)
				_realms = d["realms"];
			if (d.has("tuning") && d["tuning"].get_type() == Variant::DICTIONARY)
				_realm_tuning = d["tuning"];
		}
	}

	UtilityFunctions::print(
		vformat(TXT("DataLoader: %d items, %d skills, %d buffs, %d gongfas, %d sects, %d recipes, %d continents, %d events, drops=%s, realms=%d, abilities=%d"),
			_items.size(), _skills.size(), _buffs.size(), _gongfas.size(), _sects.size(),
			_recipes.size(), _continents.size(), _events.size(),
			_drop_table.is_empty() ? TXT("no") : TXT("yes"), _realms.size(), _abilities.size()));
}

void DataLoader::_load_json_array(const String &p_path, HashMap<StringName, Dictionary> &r_out) {
	if (!FileAccess::file_exists(p_path)) {
		UtilityFunctions::printerr(TXT("DataLoader: file not found — "), p_path);
		return;
	}

	String raw = FileAccess::get_file_as_string(p_path);
	Variant parsed = JSON::parse_string(raw);
	if (parsed.get_type() != Variant::ARRAY) {
		UtilityFunctions::printerr(TXT("DataLoader: expected JSON array — "), p_path);
		return;
	}

	Array arr = parsed;
	for (int i = 0; i < arr.size(); i++) {
		Dictionary d = arr[i];
		if (!d.has("id")) {
			UtilityFunctions::printerr(TXT("DataLoader: entry missing 'id' at index "), i);
			continue;
		}
		StringName id = d["id"];
		r_out[id] = d;
	}
}

Dictionary DataLoader::get_item(const StringName &p_id) const {
	HashMap<StringName, Dictionary>::ConstIterator it = _items.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_items() const {
	Array arr;
	for (const auto &kv : _items) arr.append(kv.value);
	return arr;
}

Dictionary DataLoader::get_skill(const StringName &p_id) const {
	HashMap<StringName, Dictionary>::ConstIterator it = _skills.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_skills() const {
	Array arr;
	for (const auto &kv : _skills) arr.append(kv.value);
	return arr;
}

Array DataLoader::get_skills_of_type(int p_type) const {
	Array arr;
	for (const auto &kv : _skills) {
		Dictionary d = kv.value;
		if (d.has("type") && int(d["type"]) == p_type) arr.append(d);
	}
	return arr;
}

Dictionary DataLoader::get_buff(const StringName &p_id) const {
	auto it = _buffs.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_buffs() const {
	Array arr;
	for (const auto &kv : _buffs) arr.append(kv.value);
	return arr;
}

Dictionary DataLoader::get_gongfa(const StringName &p_id) const {
	auto it = _gongfas.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_gongfas() const {
	Array arr;
	for (const auto &kv : _gongfas) arr.append(kv.value);
	return arr;
}

Dictionary DataLoader::get_sect(const StringName &p_id) const {
	auto it = _sects.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_sects() const {
	Array arr;
	for (const auto &kv : _sects) arr.append(kv.value);
	return arr;
}

Dictionary DataLoader::get_drop_table() const {
	return _drop_table;
}

Dictionary DataLoader::get_recipe(const StringName &p_id) const {
	auto it = _recipes.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_recipes() const {
	Array arr;
	for (const auto &kv : _recipes) arr.append(kv.value);
	return arr;
}

Dictionary DataLoader::get_continent(const StringName &p_id) const {
	auto it = _continents.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_continents() const {
	Array arr;
	for (const auto &kv : _continents) arr.append(kv.value);
	return arr;
}

Dictionary DataLoader::get_event(int p_realm) const {
	HashMap<int, Dictionary>::ConstIterator it = _events.find(p_realm);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_realms() const {
	return _realms;
}

Array DataLoader::get_all_teleports() const {
	return _teleports;
}

Array DataLoader::get_all_abilities() const {
	return _abilities;
}

Dictionary DataLoader::get_realm_tuning() const {
	return _realm_tuning;
}

} // namespace godot
