#ifndef CPP_KAKI_TEXT_H
#define CPP_KAKI_TEXT_H

#include <godot_cpp/variant/string.hpp>

namespace godot {

// Unified text wrapper: ALL string literals in this project must go through
// TXT() instead of String("..."). godot-cpp's String(const char*) decodes as
// Latin-1, so any CJK/non-ASCII literal built directly turns into mojibake.
// TXT() always decodes UTF-8 (our source files are UTF-8).
inline String TXT(const char *p_str) {
	return String::utf8(p_str);
}

} // namespace godot

#endif // CPP_KAKI_TEXT_H
