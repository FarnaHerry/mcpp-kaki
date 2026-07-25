#include "data_loader.h"

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void DataLoader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_item", "id"), &DataLoader::get_item);
	ClassDB::bind_method(D_METHOD("get_all_items"), &DataLoader::get_all_items);
	ClassDB::bind_method(D_METHOD("get_skill", "id"), &DataLoader::get_skill);
	ClassDB::bind_method(D_METHOD("get_all_skills"), &DataLoader::get_all_skills);
	ClassDB::bind_method(D_METHOD("get_skills_of_type", "type"), &DataLoader::get_skills_of_type);
}

void DataLoader::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	_load_json("res://data/items.json", _items);
	_load_json("res://data/skills.json", _skills);

	UtilityFunctions::print(
		vformat(TXT("DataLoader: %d items, %d skills loaded"),
			_items.size(), _skills.size()));
}

void DataLoader::_load_json(const String &p_path, HashMap<StringName, Dictionary> &r_out) {
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
	for (const auto &kv : _items) {
		arr.append(kv.value);
	}
	return arr;
}

Dictionary DataLoader::get_skill(const StringName &p_id) const {
	HashMap<StringName, Dictionary>::ConstIterator it = _skills.find(p_id);
	if (it) return it->value;
	return Dictionary();
}

Array DataLoader::get_all_skills() const {
	Array arr;
	for (const auto &kv : _skills) {
		arr.append(kv.value);
	}
	return arr;
}

Array DataLoader::get_skills_of_type(int p_type) const {
	Array arr;
	for (const auto &kv : _skills) {
		Dictionary d = kv.value;
		if (d.has("type") && int(d["type"]) == p_type) {
			arr.append(d);
		}
	}
	return arr;
}

} // namespace godot
