#ifndef CPP_KAKI_TEXT_H
#define CPP_KAKI_TEXT_H

#include <godot_cpp/variant/string.hpp>

import mcpp_kaki.utils; // for g_localization (Localization is a module class)

namespace godot {

// Unified text wrapper: ALL string literals in this project must go through
// TXT() instead of String("..."). godot-cpp's String(const char*) decodes as
// Latin-1, so any CJK/non-ASCII literal built directly turns into mojibake.
// TXT() always decodes UTF-8 (our source files are UTF-8).
inline String TXT(const char *p_str) {
	return String::utf8(p_str);
}

// LOC() — localization-aware text wrapper. Use instead of TXT() for
// user-visible display strings. If English is active and a translation
// exists, returns English; otherwise falls back to Chinese.
// Safe during early init: g_localization is nullptr until Localization::_ready().
inline String LOC(const char *p_str) {
	String cn = String::utf8(p_str);
	if (g_localization) {
		String tr = g_localization->translate(cn);
		if (!tr.is_empty()) return tr;
	}
	return cn;
}

// String overload — for localizing data-driven strings (item names, etc.)
inline String LOC(const String &p_key) {
	if (g_localization) {
		String tr = g_localization->translate(p_key);
		if (!tr.is_empty()) return tr;
	}
	return p_key;
}

} // namespace godot

#endif // CPP_KAKI_TEXT_H
