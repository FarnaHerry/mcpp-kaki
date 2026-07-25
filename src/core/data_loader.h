#ifndef CPP_KAKI_DATA_LOADER_H
#define CPP_KAKI_DATA_LOADER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace godot {

// Singleton Node that loads data/*.json files at startup.
// Instantiated by WorldCommon BEFORE ItemDatabase and SkillSystem.
// All game data tables (items, skills, buffs, recipes, etc.) are
// externalized as JSON — no more C++ static arrays.
class DataLoader : public Node {
	GDCLASS(DataLoader, Node);

public:
	void _ready() override;

	// ---- Items ----
	Dictionary get_item(const StringName &p_id) const;
	Array get_all_items() const; // [{id,name,type,...}, ...]

	// ---- Skills ----
	Dictionary get_skill(const StringName &p_id) const;
	Array get_all_skills() const;
	Array get_skills_of_type(int p_type) const;

	// ---- Buffs ----
	Dictionary get_buff(const StringName &p_id) const;
	Array get_all_buffs() const;

	// ---- Gongfas ----
	Dictionary get_gongfa(const StringName &p_id) const;
	Array get_all_gongfas() const;

	// ---- Sects ----
	Dictionary get_sect(const StringName &p_id) const;
	Array get_all_sects() const;

	// ---- Drops ----
	Dictionary get_drop_table() const;

	// ---- Recipes ----
	Dictionary get_recipe(const StringName &p_id) const;
	Array get_all_recipes() const;

	// ---- Continents ----
	Dictionary get_continent(const StringName &p_id) const;
	Array get_all_continents() const;

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
	Dictionary _drop_table;

	void _load_json_array(const String &p_path, HashMap<StringName, Dictionary> &r_out);
};

} // namespace godot

#endif // CPP_KAKI_DATA_LOADER_H
