module;
#include "../nodes/player.h"
#include "../nodes/dongtian_manager.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>

#include "../utils/text.h"

module mcpp_kaki.nodes;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;

namespace godot {

void StoragePanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_player", "player"), &StoragePanel::set_player);
	ClassDB::bind_method(D_METHOD("open"), &StoragePanel::open);
	ClassDB::bind_method(D_METHOD("close"), &StoragePanel::close);
	ClassDB::bind_method(D_METHOD("is_open"), &StoragePanel::is_open);
}

void StoragePanel::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_layer(115); // GameHUD(100) / InventoryPanel(110) 之上
	set_process_mode(Node::PROCESS_MODE_ALWAYS); // 打开时暂停世界，面板照常轮询

	// 背景：中央面板
	_background = memnew(ColorRect);
	_background->set_color(Color(0.05f, 0.06f, 0.10f, 0.92f));
	_background->set_position(Vector2(50, 24));
	_background->set_size(Vector2(380, 222));
	add_child(_background);

	_title = memnew(Label);
	_title->set_position(Vector2(200, 32));
	_title->add_theme_font_size_override("font_size", 14);
	_title->set_text(LOC("—— 洞天仓库 ——"));
	add_child(_title);

	static const int PANE_X[2] = { 70, 260 };
	static const char *PANE_NAMES[2] = { "背包", "仓库" };
	for (int p = 0; p < 2; p++) {
		_headers[p] = memnew(Label);
		_headers[p]->set_position(Vector2(PANE_X[p], 56));
		_headers[p]->add_theme_font_size_override("font_size", 12);
		_headers[p]->set_text(LOC(PANE_NAMES[p]));
		add_child(_headers[p]);
		for (int r = 0; r < ROWS; r++) {
			_rows[p][r] = memnew(Label);
			_rows[p][r]->set_position(Vector2(PANE_X[p], 76 + r * 16));
			_rows[p][r]->add_theme_font_size_override("font_size", 12);
			add_child(_rows[p][r]);
		}
	}

	_msg = memnew(Label);
	_msg->set_position(Vector2(70, 208));
	_msg->add_theme_font_size_override("font_size", 12);
	_msg->add_theme_color_override("font_color", Color(0.9f, 0.8f, 0.4f, 1.0f));
	add_child(_msg);

	_hint = memnew(Label);
	_hint->set_position(Vector2(70, 226));
	_hint->add_theme_font_size_override("font_size", 10);
	_hint->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f, 1.0f));
	_hint->set_text(LOC("↑/↓ 选择  ←/→ 切换  X 移送  ESC/O 关闭"));
	add_child(_hint);

	set_visible(false);
	set_process(true);
}

DongtianManager *StoragePanel::_find_manager() {
	if (_manager)
		return _manager;
	Node *root = get_tree()->get_current_scene();
	if (root)
		_manager = Object::cast_to<DongtianManager>(root->find_child("DongtianManager", false, false));
	return _manager;
}

void StoragePanel::open() {
	if (_visible)
		return;
	_visible = true;
	_restore_pause = get_tree()->is_paused();
	get_tree()->set_pause(true);
	_pane = 0;
	_sel[0] = _sel[1] = 0;
	_scroll[0] = _scroll[1] = 0;
	_msg_t = 0.0f;
	if (_msg) _msg->set_text(String());
	set_visible(true);
	_refresh();
}

void StoragePanel::close() {
	if (!_visible)
		return;
	_visible = false;
	set_visible(false);
	get_tree()->set_pause(_restore_pause); // 嵌套暂停安全
}

void StoragePanel::_set_msg(const String &p_text) {
	_msg->set_text(p_text);
	_msg_t = 2.0f;
}

void StoragePanel::_rebuild_lists() {
	_slots[0].clear();
	_slots[1].clear();
	if (_player && _player->get_inventory()) {
		Inventory *inv = _player->get_inventory();
		int cap = inv->get_capacity();
		for (int i = 0; i < cap; i++) {
			Dictionary s = inv->get_slot(i);
			if (!StringName(s.get("id", StringName())).is_empty() && int(s.get("quantity", 0)) > 0)
				_slots[0].push_back(i);
		}
	}
	DongtianManager *mgr = _find_manager();
	if (mgr) {
		for (int i = 0; i < DongtianManager::STORAGE_SLOTS; i++) {
			if (!mgr->get_storage_slot(i).is_empty())
				_slots[1].push_back(i);
		}
	}
}

void StoragePanel::_refresh() {
	_rebuild_lists();

	ItemDatabase *db = ItemDatabase::get_singleton();
	Inventory *inv = (_player && _player->get_inventory()) ? _player->get_inventory() : nullptr;
	DongtianManager *mgr = _find_manager();

	for (int p = 0; p < 2; p++) {
		int count = int(_slots[p].size());
		// 选择/滚动钳制
		if (_sel[p] >= count) _sel[p] = count > 0 ? count - 1 : 0;
		if (_scroll[p] > _sel[p]) _scroll[p] = _sel[p];
		if (_scroll[p] + ROWS <= _sel[p]) _scroll[p] = _sel[p] - ROWS + 1;

		// 激活页签高亮
		_headers[p]->add_theme_color_override("font_color",
				_pane == p ? Color(1.0f, 0.9f, 0.4f, 1.0f) : Color(0.6f, 0.6f, 0.6f, 1.0f));

		for (int r = 0; r < ROWS; r++) {
			int li = _scroll[p] + r; // 紧凑列表索引
			Label *row = _rows[p][r];
			if (li >= count) {
				row->set_text(r == 0 && count == 0 ? LOC("（空）") : String());
				row->add_theme_color_override("font_color", Color(0.35f, 0.35f, 0.35f, 1.0f));
				continue;
			}
			String text;
			if (p == 0) {
				Dictionary s = inv->get_slot(_slots[0][li]);
				const Item *def = db ? db->get_item(s.get("id", StringName())) : nullptr;
				text = (def ? LOC(def->name) : String(s.get("id", StringName()))) + " ×" + String::num_int64(int(s.get("quantity", 0)));
			} else {
				Dictionary s = mgr->get_storage_slot(_slots[1][li]);
				text = LOC(String(s.get("name", ""))) + " ×" + String::num_int64(int(s.get("quantity", 0)));
			}
			bool selected = (_pane == p && li == _sel[p]);
			row->set_text((selected ? String("▶ ") : String("  ")) + text);
			row->add_theme_color_override("font_color",
					selected ? Color(1.0f, 0.95f, 0.6f, 1.0f) : Color(0.85f, 0.85f, 0.85f, 1.0f));
		}
	}
}

void StoragePanel::_transfer() {
	DongtianManager *mgr = _find_manager();
	if (!mgr || _slots[_pane].empty())
		return;
	int real_slot = _slots[_pane][_sel[_pane]];

	if (_pane == 0) {
		// 背包 → 仓库
		Dictionary s = _player->get_inventory()->get_slot(real_slot);
		StringName id = s.get("id", StringName());
		int had = int(s.get("quantity", 0));
		int n = mgr->deposit_from_player(real_slot);
		if (n <= 0) {
			_set_msg(LOC("仓库已满"));
		} else {
			const Item *def = ItemDatabase::get_singleton()->get_item(id);
			String name = def ? LOC(def->name) : String(id);
			_set_msg(LOC("存入 ") + name + " ×" + String::num_int64(n) + (n < had ? LOC("（仓库满，部分留存）") : String()));
		}
	} else {
		// 仓库 → 背包
		Dictionary s = mgr->get_storage_slot(real_slot);
		String name = LOC(String(s.get("name", "")));
		int n = mgr->withdraw_to_player(real_slot);
		_set_msg(n > 0 ? LOC("取出 ") + name + " ×" + String::num_int64(n) : LOC("背包已满"));
	}
	_refresh();
}

void StoragePanel::_process(double p_delta) {
	if (!_visible)
		return;

	if (_msg_t > 0.0f) {
		_msg_t -= float(p_delta);
		if (_msg_t <= 0.0f)
			_msg->set_text(String());
	}

	Input *input = Input::get_singleton();

	if (input->is_action_just_pressed(LOC("menu")) || input->is_action_just_pressed(LOC("dongtian"))) {
		close();
		return;
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		_pane = 0;
		_refresh();
	} else if (input->is_action_just_pressed(LOC("right"))) {
		_pane = 1;
		_refresh();
	} else if (input->is_action_just_pressed(LOC("up"))) {
		if (_sel[_pane] > 0) _sel[_pane]--;
		_refresh();
	} else if (input->is_action_just_pressed(LOC("down"))) {
		if (_sel[_pane] + 1 < int(_slots[_pane].size())) _sel[_pane]++;
		_refresh();
	} else if (input->is_action_just_pressed(LOC("interact"))) {
		_transfer();
	}
}

} // namespace godot
