module;
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

module mcpp_kaki.nodes;

namespace godot {

void GridList::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_items", "items"), &GridList::set_items);
	ClassDB::bind_method(D_METHOD("get_item_count"), &GridList::get_item_count);
	ClassDB::bind_method(D_METHOD("set_columns", "cols"), &GridList::set_columns);
	ClassDB::bind_method(D_METHOD("get_columns"), &GridList::get_columns);
	ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &GridList::set_cell_size);
	ClassDB::bind_method(D_METHOD("set_active", "active"), &GridList::set_active);
	ClassDB::bind_method(D_METHOD("set_selected", "idx"), &GridList::set_selected);
	ClassDB::bind_method(D_METHOD("get_selected"), &GridList::get_selected);
	ClassDB::bind_method(D_METHOD("move_selection", "dx", "dy"), &GridList::move_selection);
	ClassDB::bind_method(D_METHOD("refresh"), &GridList::refresh);
}

void GridList::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;
	_build_pool();
	refresh();
}

void GridList::set_columns(int p_cols) {
	_columns = Math::max(1, p_cols);
	_built = false;
	if (is_inside_tree())
		_build_pool();
	refresh();
}

void GridList::set_cell_size(const Vector2 &p_size) {
	_cell_size = p_size;
	_built = false;
	if (is_inside_tree())
		_build_pool();
	refresh();
}

void GridList::set_items(const Array &p_items) {
	_items = p_items;
	if (_selected >= int(_items.size()))
		_selected = int(_items.size()) > 0 ? int(_items.size()) - 1 : 0;
	refresh();
}

void GridList::set_active(bool p_active) {
	_active = p_active;
	refresh();
}

void GridList::set_selected(int p_idx) {
	int count = int(_items.size());
	_selected = count > 0 ? CLAMP(p_idx, 0, count - 1) : 0;
	_ensure_visible();
	refresh();
}

void GridList::move_selection(int p_dx, int p_dy) {
	int count = int(_items.size());
	if (count == 0)
		return;
	if (p_dx != 0) {
		// 水平：钳到行内（不跨行回卷）；最后一行不满时退到行末
		int row = _selected / _columns;
		int col = CLAMP(_selected % _columns + p_dx, 0, _columns - 1);
		int idx = row * _columns + col;
		_selected = idx < count ? idx : count - 1;
	}
	if (p_dy != 0) {
		int next = _selected + p_dy * _columns;
		_selected = CLAMP(next, 0, count - 1);
	}
	_ensure_visible();
	refresh();
}

void GridList::_ensure_visible() {
	int row = _selected / _columns;
	int vis_rows = _pool_rows > 0 ? _pool_rows : 1;
	if (row < _scroll_row)
		_scroll_row = row;
	else if (row >= _scroll_row + vis_rows)
		_scroll_row = row - vis_rows + 1;
}

void GridList::_build_pool() {
	// 清旧池
	for (Cell &c : _cells) {
		if (c.frame)
			c.frame->queue_free(); // bg/label 是 frame 子节点，随之一同释放
	}
	_cells.clear();

	float h = get_size().y > 0.0f ? get_size().y : 120.0f;
	_pool_rows = Math::max(1, int(h / _cell_size.y));
	int pool = _pool_rows * _columns;
	_cells.reserve(pool);
	for (int i = 0; i < pool; i++) {
		Cell c;
		int col = i % _columns;
		int row = i / _columns;
		Vector2 pos = Vector2(col * _cell_size.x, row * _cell_size.y);

		c.frame = memnew(ColorRect);
		c.frame->set_position(pos);
		c.frame->set_size(_cell_size - Vector2(2, 2));
		c.frame->set_color(Color(0.25f, 0.28f, 0.35f, 0.9f));
		add_child(c.frame);

		c.bg = memnew(ColorRect);
		c.bg->set_position(Vector2(1, 1));
		c.bg->set_size(_cell_size - Vector2(4, 4));
		c.bg->set_color(Color(0.09f, 0.10f, 0.14f, 0.95f));
		c.frame->add_child(c.bg);

		c.label = memnew(Label);
		c.label->set_position(Vector2(3, 1));
		c.label->set_size(_cell_size - Vector2(8, 4));
		c.label->add_theme_font_size_override("font_size", 11);
		c.label->set_clip_text(true);
		c.bg->add_child(c.label);

		_cells.push_back(c);
	}
	_built = true;
}

void GridList::refresh() {
	if (!is_inside_tree())
		return;

	// size 变化（宿主 set_size 后）自动重建对象池
	float h = get_size().y > 0.0f ? get_size().y : 120.0f;
	int rows = Math::max(1, int(h / _cell_size.y));
	if (!_built || rows != _pool_rows)
		_build_pool();

	static const Color FRAME_IDLE(0.25f, 0.28f, 0.35f, 0.9f);
	static const Color FRAME_SEL(0.95f, 0.80f, 0.30f, 1.0f);
	static const Color BG_IDLE(0.09f, 0.10f, 0.14f, 0.95f);
	static const Color BG_SEL(0.22f, 0.20f, 0.12f, 0.98f);
	static const Color TEXT_DIM(0.40f, 0.40f, 0.40f, 1.0f);
	static const Color TEXT_INACTIVE(0.55f, 0.55f, 0.55f, 1.0f);

	int count = int(_items.size());
	int base = _scroll_row * _columns;

	for (int i = 0; i < int(_cells.size()); i++) {
		Cell &c = _cells[i];
		int idx = base + i;
		if (idx >= count) {
			c.frame->set_visible(false);
			continue;
		}
		c.frame->set_visible(true);

		Dictionary d = _items[idx];
		String text = d.get("text", String());
		Color color = d.get("color", Color(0.85f, 0.85f, 0.85f, 1.0f));
		Color bg_custom = d.get("bg_color", Color(0.0f, 0.0f, 0.0f, 0.0f)); // alpha=0 = 无自定义底色
		bool dim = bool(d.get("dim", false));

		bool selected = _active && idx == _selected;
		c.frame->set_color(selected ? FRAME_SEL : FRAME_IDLE);
		// 选中高亮优先（金框+选中底）；未选中有自定义底色（如品级淡染）则用之，否则默认底
		c.bg->set_color(selected ? BG_SEL : (bg_custom.a > 0.0f ? bg_custom : BG_IDLE));
		c.label->set_text(text);
		if (dim)
			c.label->add_theme_color_override("font_color", TEXT_DIM);
		else if (!_active)
			c.label->add_theme_color_override("font_color", TEXT_INACTIVE);
		else
			c.label->add_theme_color_override("font_color", color);
	}
}

} // namespace godot
