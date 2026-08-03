// mcpp-kaki utils module — SignalBus / Localization / globals.
// godot-cpp stays header-based (official, untouched); our own classes are modular.
module;

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string.hpp>

export module mcpp_kaki.utils;

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
