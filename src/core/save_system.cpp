module;


#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.core;
import mcpp_kaki.utils;
namespace godot {

SaveSystem::SaveSystem() {
	_signal_bus = SignalBus::get_singleton();
}

SaveSystem::~SaveSystem() {}

void SaveSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("save_game", "slot_name", "data"), &SaveSystem::save_game);
	ClassDB::bind_method(D_METHOD("load_game", "slot_name"), &SaveSystem::load_game);
	ClassDB::bind_method(D_METHOD("has_save", "slot_name"), &SaveSystem::has_save);
	ClassDB::bind_method(D_METHOD("delete_save", "slot_name"), &SaveSystem::delete_save);
}

String SaveSystem::get_save_path(const String &p_slot_name) {
	return String("user://savegames/") + p_slot_name + ".cfg";
}

void SaveSystem::_ensure_save_dir() const {
	Ref<DirAccess> dir = DirAccess::open("user://");
	if (dir.is_valid() && !dir->dir_exists("savegames")) {
		dir->make_dir("savegames");
	}
}

bool SaveSystem::save_game(const String &p_slot_name, const Dictionary &p_data) {
	_ensure_save_dir();

	Ref<ConfigFile> cfg;
	cfg.instantiate();

	// Write each section from the data dictionary
	// Expected structure: { "section_name": { "key": value, ... }, ... }
	Array sections = p_data.keys();
	for (int i = 0; i < sections.size(); i++) {
		String section = sections[i];
		Variant section_data = p_data[section];

		if (section_data.get_type() != Variant::DICTIONARY) {
			continue;
		}

		Dictionary section_dict = section_data;
		Array keys = section_dict.keys();
		for (int j = 0; j < keys.size(); j++) {
			String key = keys[j];
			cfg->set_value(section, key, section_dict[key]);
		}
	}

	String path = get_save_path(p_slot_name);
	Error err = cfg->save(path);

	if (err != Error::OK) {
		if (_signal_bus) {
			_signal_bus->emit_signal("save_error",
				String("Failed to save: ") + path);
		}
		return false;
	}

	if (_signal_bus) {
		_signal_bus->emit_signal("game_saved", p_slot_name);
	}
	return true;
}

Dictionary SaveSystem::load_game(const String &p_slot_name) {
	String path = get_save_path(p_slot_name);

	if (!FileAccess::file_exists(path)) {
		if (_signal_bus) {
			_signal_bus->emit_signal("save_error",
				String("No save file: ") + path);
		}
		return Dictionary();
	}

	Ref<ConfigFile> cfg;
	cfg.instantiate();

	Error err = cfg->load(path);
	if (err != Error::OK) {
		if (_signal_bus) {
			_signal_bus->emit_signal("save_error",
				String("Failed to load: ") + path);
		}
		return Dictionary();
	}

	// Rebuild the data dictionary from all sections
	Dictionary result;
	PackedStringArray sections = cfg->get_sections();
	for (int64_t i = 0; i < sections.size(); i++) {
		String section = sections[i];
		Dictionary section_dict;
		PackedStringArray keys = cfg->get_section_keys(section);
		for (int64_t j = 0; j < keys.size(); j++) {
			String key = keys[j];
			section_dict[key] = cfg->get_value(section, key);
		}
		result[section] = section_dict;
	}

	if (_signal_bus) {
		_signal_bus->emit_signal("game_loaded", p_slot_name);
	}
	return result;
}

bool SaveSystem::has_save(const String &p_slot_name) const {
	return FileAccess::file_exists(get_save_path(p_slot_name));
}

bool SaveSystem::delete_save(const String &p_slot_name) {
	String path = get_save_path(p_slot_name);
	if (!FileAccess::file_exists(path)) {
		return false;
	}

	Ref<DirAccess> dir = DirAccess::open("user://savegames/");
	if (dir.is_valid()) {
		Error err = dir->remove(p_slot_name + String(".cfg"));
		return err == Error::OK;
	}
	return false;
}

} // namespace godot
