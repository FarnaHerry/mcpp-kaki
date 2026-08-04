// mcpp-kaki utils module — SignalBus / Localization / globals.
// godot-cpp 经模块导入（import godot_cpp）；宏（GDCLASS 等）走 godot-cpp-m/macros.h。
module;

#include <godot-cpp-m/macros.h>
#include <godot_cpp/templates/hash_map.hpp> // HashMap 不被模块重导出，保持文本包含

export module mcpp_kaki.utils;

import godot_cpp;

namespace godot {

// Global pointer set by Localization::_ready().
// LOC() checks this before calling translate() — safe during early init.
export class Localization;
export extern Localization *g_localization;

// Global signal bus — autoload singleton.
// All systems communicate through this node to stay decoupled.
export class SignalBus : public Node {
	GDCLASS(SignalBus, Node);

public:
	static SignalBus *get_singleton() { return _singleton; }

	void _ready() override;

protected:
	static void _bind_methods();

private:
	static SignalBus *_singleton;
};

export class Localization : public Node {
	GDCLASS(Localization, Node);

public:
	static Localization *get_singleton() { return _singleton; }

	void _ready() override;

	void set_language(const String &p_lang);
	String get_language() const { return _language; }
	String translate(const String &p_key) const;

protected:
	static void _bind_methods();

private:
	static Localization *_singleton;

	HashMap<String, String> _table;
	String _language = "zh";

	void _load_translations();
};

} // namespace godot
