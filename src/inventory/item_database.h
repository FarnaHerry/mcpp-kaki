#ifndef CPP_KAKI_ITEM_DATABASE_H
#define CPP_KAKI_ITEM_DATABASE_H

#include "item.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

// Singleton registry of all item definitions in the game.
// Items are hardcoded in _register_items() for now; future sessions
// will load them from .tres Resource files.
// Extends Node so it can be added to the scene tree (like SignalBus).
class ItemDatabase : public Node {
	GDCLASS(ItemDatabase, Node);

public:
	static ItemDatabase *get_singleton() { return _singleton; }

	ItemDatabase();
	~ItemDatabase();

	// Look up an item by its string ID. Returns nullptr if not found.
	const Item *get_item(const StringName &p_id) const;

	// Number of registered items
	int get_item_count() const { return _items.size(); }
	// GDScript/UI 查询口（get_item 返回 const Item* 无法绑定）
	bool has_item(const StringName &p_id) const { return _items.has(p_id); }
	Dictionary get_item_info(const StringName &p_id) const;

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static ItemDatabase *_singleton;

	HashMap<StringName, Item> _items;

	void _register_items();
};

} // namespace godot

#endif // CPP_KAKI_ITEM_DATABASE_H
