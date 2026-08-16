module;
#include "../nodes/player.h"
#include "../nodes/dongtian_manager.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
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
	set_process_input(true);

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

	static const int PANE_X[2] = { 62, 252 };
	static const char *PANE_NAMES[2] = { "背包", "仓库" };
	for (int p = 0; p < 2; p++) {
		_headers[p] = memnew(Label);
		_headers[p]->set_position(Vector2(PANE_X[p], 52));
		_headers[p]->add_theme_font_size_override("font_size", 12);
		_headers[p]->set_text(LOC(PANE_NAMES[p]));
		add_child(_headers[p]);

		// 统一格子列表：2 列 × 5 行窗口
		_grids[p] = memnew(GridList);
		_grids[p]->set_position(Vector2(PANE_X[p], 70));
		_grids[p]->set_size(Vector2(176, 128));
		add_child(_grids[p]);
		_grids[p]->set_columns(2);
		_grids[p]->set_cell_size(Vector2(88, 24));
	}

	_msg = memnew(Label);
	_msg->set_position(Vector2(62, 204));
	_msg->add_theme_font_size_override("font_size", 12);
	_msg->add_theme_color_override("font_color", Color(0.9f, 0.8f, 0.4f, 1.0f));
	add_child(_msg);

	_hint = memnew(Label);
	_hint->set_position(Vector2(62, 226));
	_hint->add_theme_font_size_override("font_size", 10);
	_hint->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f, 1.0f));
	_hint->set_text(LOC("↑/↓ 选择  Q/E 切栏  X 移送  ESC/O 关闭"));
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
	for (int p = 0; p < 2; p++)
		if (_grids[p]) _grids[p]->set_selected(0);
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
		// 激活页签高亮
		_headers[p]->add_theme_color_override("font_color",
				_pane == p ? Color(1.0f, 0.9f, 0.4f, 1.0f) : Color(0.6f, 0.6f, 0.6f, 1.0f));
		_grids[p]->set_active(_pane == p);

		Array items;
		for (int li = 0; li < int(_slots[p].size()); li++) {
			Dictionary cell;
			if (p == 0) {
				Dictionary s = inv->get_slot(_slots[0][li]);
				StringName id = s.get("id", StringName());
				const Item *def = db ? db->get_item(id) : nullptr;
				String txt = (def ? LOC(def->name) : String(id));
				int qty = int(s.get("quantity", 0));
				if (qty > 1)
					txt += " ×" + String::num_int64(qty);
				cell["text"] = txt;
				cell["color"] = grade_color(def ? def->grade : 0);
			} else {
				Dictionary s = mgr->get_storage_slot(_slots[1][li]);
				String txt = LOC(String(s.get("name", "")));
				int qty = int(s.get("quantity", 0));
				if (qty > 1)
					txt += " ×" + String::num_int64(qty);
				cell["text"] = txt;
				const Item *sdef = db ? db->get_item(StringName(s.get("id", StringName()))) : nullptr;
				cell["color"] = grade_color(sdef ? sdef->grade : 0);
			}
			items.push_back(cell);
		}
		_grids[p]->set_items(items);
	}
}

void StoragePanel::_transfer() {
	DongtianManager *mgr = _find_manager();
	if (!mgr || _slots[_pane].empty())
		return;
	int sel = _grids[_pane]->get_selected();
	if (sel < 0 || sel >= int(_slots[_pane].size()))
		return;
	int real_slot = _slots[_pane][sel];

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

void StoragePanel::_input(const Ref<InputEvent> &p_event) {
	if (!_visible)
		return;
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo())
		return;
	// 切栏 Q/E（与 GameMenu 翻页一致；←/→ 留给页内横向导航）
	if (k->get_keycode() == KEY_Q) {
		_pane = 0;
		_refresh();
	} else if (k->get_keycode() == KEY_E) {
		_pane = 1;
		_refresh();
	}
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
	// 切栏走 _input 的 Q/E；←/→ 留给页内横向导航（2D 卡片列移）
	if (input->is_action_just_pressed(LOC("up"))) {
		_grids[_pane]->move_selection(0, -1);
		_refresh();
	} else if (input->is_action_just_pressed(LOC("down"))) {
		_grids[_pane]->move_selection(0, +1);
		_refresh();
	} else if (input->is_action_just_pressed(LOC("left"))) {
		_grids[_pane]->move_selection(-1, 0);
		_refresh();
	} else if (input->is_action_just_pressed(LOC("right"))) {
		_grids[_pane]->move_selection(+1, 0);
		_refresh();
	} else if (input->is_action_just_pressed(LOC("interact"))) {
		_transfer();
	}
}

} // namespace godot
