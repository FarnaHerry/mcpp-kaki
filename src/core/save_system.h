#ifndef CPP_KAKI_SAVE_SYSTEM_H
#define CPP_KAKI_SAVE_SYSTEM_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class SignalBus;

// Save/Load system using Godot's ConfigFile.
// Owned by GameManager, persists checkpoint, cultivation, abilities, player stats.
class SaveSystem : public Object {
	GDCLASS(SaveSystem, Object);

public:
	SaveSystem();
	~SaveSystem();

	// ---- Save ----
	// Collect current game state and write to disk.
	// Expects all state as a Dictionary (collected by GameManager).
	bool save_game(const String &p_slot_name, const Dictionary &p_data);

	// ---- Load ----
	// Read saved data from disk. Returns empty Dictionary if no save exists.
	Dictionary load_game(const String &p_slot_name);

	// ---- Query ----
	bool has_save(const String &p_slot_name) const;
	bool delete_save(const String &p_slot_name);

	// ---- Helpers ----
	// Build the filesystem path for a slot. user://savegames/<slot>.cfg
	static String get_save_path(const String &p_slot_name);

protected:
	static void _bind_methods();

private:
	SignalBus *_signal_bus = nullptr;

	void _ensure_save_dir() const;
};

} // namespace godot

#endif // CPP_KAKI_SAVE_SYSTEM_H
