// mcpp-kaki core module — data loader / save system / continent manager.
// GameManager & DropSystem stay headers: they bind godot engine pointer params
// (Player*/Object*) which trigger the make_property_info ADL failure in modules.
module;

#include <vector>

#include <godot-cpp-m/macros.h>
#include <godot_cpp/templates/hash_map.hpp> // HashMap 不被模块重导出，保持文本包含

#include "../utils/text.h"
import mcpp_kaki.utils;

namespace godot {
class Player; // external (nodes header) — global fragment, no module linkage
}

export module mcpp_kaki.core;

import godot_cpp;

namespace godot {

export class DataLoader : public Node {
	GDCLASS(DataLoader, Node);

public:
	void _ready() override;

	Dictionary get_item(const StringName &p_id) const;
	Array get_all_items() const;

	Dictionary get_skill(const StringName &p_id) const;
	Array get_all_skills() const;
	Array get_skills_of_type(int p_type) const;

	Dictionary get_buff(const StringName &p_id) const;
	Array get_all_buffs() const;

	Dictionary get_gongfa(const StringName &p_id) const;
	Array get_all_gongfas() const;

	Dictionary get_sect(const StringName &p_id) const;
	Array get_all_sects() const;

	Dictionary get_drop_table() const;

	Dictionary get_recipe(const StringName &p_id) const;
	Array get_all_recipes() const;

	Dictionary get_continent(const StringName &p_id) const;
	Array get_all_continents() const;

	Dictionary get_event(int p_realm) const;

	// 境界表（data/realms.json）：realms 数组按境界序号排列 + 全局 tuning
	Array get_all_realms() const;
	Dictionary get_realm_tuning() const;

protected:
	static void _bind_methods();

private:
	HashMap<StringName, Dictionary> _items;
	HashMap<StringName, Dictionary> _skills;
	HashMap<StringName, Dictionary> _buffs;
	HashMap<StringName, Dictionary> _gongfas;
	HashMap<StringName, Dictionary> _sects;
	HashMap<StringName, Dictionary> _recipes;
	HashMap<StringName, Dictionary> _continents;
	HashMap<int, Dictionary> _events;
	Dictionary _drop_table;
	Array _realms;         // 下标 = 境界序号
	Dictionary _realm_tuning;

	void _load_json_array(const String &p_path, HashMap<StringName, Dictionary> &r_out);
};

export class SaveSystem : public Object {
	GDCLASS(SaveSystem, Object);

public:
	SaveSystem();
	~SaveSystem();

	bool save_game(const String &p_slot_name, const Dictionary &p_data);
	Dictionary load_game(const String &p_slot_name);
	bool has_save(const String &p_slot_name) const;
	bool delete_save(const String &p_slot_name);

	static String get_save_path(const String &p_slot_name);

protected:
	static void _bind_methods();

private:
	SignalBus *_signal_bus = nullptr;

	void _ensure_save_dir() const;
};

export class ContinentManager : public Node {
	GDCLASS(ContinentManager, Node);

public:
	struct Def {
		const char *id;
		const char *name;
		const char *scene;
		float spawn_x;
		float spawn_y;
		int min_realm;
		const char *desc;
		const char *gate;
	};

	static const Def *find_def(const String &p_id);
	static void ensure_loaded();

	String get_current_id() const { return _current_id; }
	String get_current_name() const;
	bool is_unlocked(const String &p_id) const;
	bool can_travel(const String &p_id) const;
	bool travel_to(const String &p_id);
	bool travel_to_direct(const String &p_id);
	bool complete_travel();
	Array get_continent_list() const;

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static std::vector<Def> s_defs;
	static bool s_loaded;
	String _current_id;

	Player *_find_player() const;
};

} // namespace godot
