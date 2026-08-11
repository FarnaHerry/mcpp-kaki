module;
#include "../nodes/player.h"
#include "../core/shop_system.h"
#include "../core/currency_system.h"

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

void ShopPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_player", "player"), &ShopPanel::set_player);
	ClassDB::bind_method(D_METHOD("open"), &ShopPanel::open);
	ClassDB::bind_method(D_METHOD("close"), &ShopPanel::close);
	ClassDB::bind_method(D_METHOD("is_open"), &ShopPanel::is_open);
}

void ShopPanel::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_layer(116); // StoragePanel(115) 之上
	set_process_mode(Node::PROCESS_MODE_ALWAYS); // 打开时暂停世界，面板照常轮询
	set_process_input(true);

	_background = memnew(ColorRect);
	_background->set_color(Color(0.05f, 0.06f, 0.10f, 0.92f));
	_background->set_position(Vector2(50, 24));
	_background->set_size(Vector2(380, 222));
	add_child(_background);

	_title = memnew(Label);
	_title->set_position(Vector2(200, 26));
	_title->add_theme_font_size_override("font_size", 14);
	_title->set_text(LOC("—— 长安坊市 ——"));
	add_child(_title);

	// 灵石余额（四阶通用货币，session 012）
	_balance = memnew(Label);
	_balance->set_position(Vector2(60, 39));
	_balance->add_theme_font_size_override("font_size", 10);
	_balance->add_theme_color_override("font_color", Color(0.6f, 0.85f, 1.0f, 1.0f));
	_balance->set_text(LOC("灵石 0"));
	add_child(_balance);

	static const int PANE_X[3] = { 62, 252, 62 };
	static const char *PANE_NAMES[3] = { "货架", "背包", "兑换" };
	for (int p = 0; p < 3; p++) {
		_headers[p] = memnew(Label);
		_headers[p]->set_position(Vector2(PANE_X[p], 52));
		_headers[p]->add_theme_font_size_override("font_size", 12);
		_headers[p]->set_text(LOC(PANE_NAMES[p]));
		add_child(_headers[p]);

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
	_hint->set_text(LOC("↑/↓ 选择  Q/E 切栏  X 购买/卖出/兑换  ESC 关闭"));
	add_child(_hint);

	set_visible(false);
	set_process(true);
}

ShopSystem *ShopPanel::_find_shop() {
	if (_shop)
		return _shop;
	Node *root = get_tree()->get_current_scene();
	if (root)
		_shop = Object::cast_to<ShopSystem>(root->find_child("ShopSystem", true, false));
	return _shop;
}

void ShopPanel::open() {
	if (_visible)
		return;
	_visible = true;
	_restore_pause = get_tree()->is_paused();
	get_tree()->set_pause(true);
	_pane = 0;
	_msg_t = 0.0f;
	if (_msg)
		_msg->set_text(String());
	ShopSystem *shop = _find_shop();
	if (shop)
		_stock = shop->get_stock();
	else
		_stock = Array();
	set_visible(true);
	_refresh();
}

void ShopPanel::close() {
	if (!_visible)
		return;
	_visible = false;
	set_visible(false);
	get_tree()->set_pause(_restore_pause); // 嵌套暂停安全
}

void ShopPanel::_set_msg(const String &p_text) {
	_msg->set_text(p_text);
	_msg_t = 2.0f;
}

void ShopPanel::_refresh() {
	ShopSystem *shop = _find_shop();
	ItemDatabase *db = ItemDatabase::get_singleton();

	// 灵石余额（四阶：下品/中品/上品/极品）
	if (_balance) {
		CurrencySystem *cs = CurrencySystem::get_singleton();
		String txt = LOC("灵石 下") + String::num_int64(cs ? cs->get_amount(CurrencySystem::TIER_LOW) : 0);
		txt += LOC(" 中") + String::num_int64(cs ? cs->get_amount(CurrencySystem::TIER_MID) : 0);
		txt += LOC(" 上") + String::num_int64(cs ? cs->get_amount(CurrencySystem::TIER_HIGH) : 0);
		txt += LOC(" 极") + String::num_int64(cs ? cs->get_amount(CurrencySystem::TIER_PEAK) : 0);
		_balance->set_text(txt);
	}

	// 栏 0：商店货架
	Array stock;
	for (int i = 0; i < int(_stock.size()); i++) {
		Dictionary s = _stock[i];
		const Item *def = db ? db->get_item(s.get("id", StringName())) : nullptr;
		Dictionary cell;
		String txt = (def ? LOC(def->name) : String(s.get("name", ""))) +
			LOC(" ×") + String::num_int64(int(s.get("price", 0)));
		cell["text"] = txt;
		cell["color"] = Color(0.8f, 0.9f, 0.6f, 1.0f);
		stock.push_back(cell);
	}
	_grids[0]->set_items(stock);

	// 栏 1：玩家背包（只显示商店可收之物：sell_price>0）
	_slots[0].clear();
	Array inv_items;
	if (_player && _player->get_inventory()) {
		Inventory *inv = _player->get_inventory();
		int cap = inv->get_capacity();
		for (int i = 0; i < cap; i++) {
			Dictionary sd = inv->get_slot(i);
			if (sd.is_empty())
				continue;
			StringName id = sd.get("id", StringName());
			const Item *def = db ? db->get_item(id) : nullptr;
			if (!def || def->sell_price <= 0)
				continue; // 只列可卖
			Dictionary cell;
			String txt = (def ? LOC(def->name) : String(id));
			int qty = int(sd.get("quantity", 0));
			if (qty > 1)
				txt += " ×" + String::num_int64(qty);
			cell["text"] = txt;
			cell["color"] = Color(0.8f, 0.8f, 0.85f, 1.0f);
			inv_items.push_back(cell);
			_slots[0].push_back(i);
		}
	}
	_grids[1]->set_items(inv_items);

	// 栏 2：灵石兑换（6 条保值兑换，X 全额）
	Array ex_items;
	CurrencySystem *cs = CurrencySystem::get_singleton();
	if (cs) {
		// {from, to}：0下 1中 2上 3极；RATIO=10
		static const int EX[6][2] = {
			{ CurrencySystem::TIER_LOW, CurrencySystem::TIER_MID },   // 10下→1中
			{ CurrencySystem::TIER_MID, CurrencySystem::TIER_LOW },   // 1中→10下
			{ CurrencySystem::TIER_MID, CurrencySystem::TIER_HIGH },  // 10中→1上
			{ CurrencySystem::TIER_HIGH, CurrencySystem::TIER_MID },  // 1上→10中
			{ CurrencySystem::TIER_HIGH, CurrencySystem::TIER_PEAK }, // 10上→1极
			{ CurrencySystem::TIER_PEAK, CurrencySystem::TIER_HIGH }, // 1极→10上
		};
		for (int i = 0; i < 6; i++) {
			int from = EX[i][0], to = EX[i][1];
			bool combine = from < to; // 低→高 = 合成
			int have = cs->get_amount(from);
			int can = combine ? have / CurrencySystem::RATIO : have;
			Dictionary cell;
			cell["text"] = LOC("「") + CurrencySystem::tier_name(from) + LOC("→") + CurrencySystem::tier_name(to) +
				LOC("」×") + String::num_int64(can);
			cell["color"] = can > 0 ? Color(0.7f, 0.95f, 0.8f, 1.0f) : Color(0.45f, 0.45f, 0.5f, 1.0f);
			ex_items.push_back(cell);
		}
	}
	_grids[2]->set_items(ex_items);

	// 激活页签高亮 + 显隐（兑换页独占左栏）
	for (int p = 0; p < 3; p++) {
		_headers[p]->add_theme_color_override("font_color",
			_pane == p ? Color(1.0f, 0.9f, 0.4f, 1.0f) : Color(0.6f, 0.6f, 0.6f, 1.0f));
		bool active = _pane == p;
		bool visible = (p < 2) ? (_pane != 2) : (_pane == 2);
		_grids[p]->set_active(active);
		_grids[p]->set_visible(visible);
	}
}

void ShopPanel::_trade() {
	ShopSystem *shop = _find_shop();
	if (!shop || !_player)
		return;
	if (_pane == 2) {
		// 灵石兑换：X 全额兑换选中行（保值，RATIO=10）
		int sel = _grids[2]->get_selected();
		if (sel < 0 || sel >= 6)
			return;
		static const int EX[6][2] = {
			{ CurrencySystem::TIER_LOW, CurrencySystem::TIER_MID },
			{ CurrencySystem::TIER_MID, CurrencySystem::TIER_LOW },
			{ CurrencySystem::TIER_MID, CurrencySystem::TIER_HIGH },
			{ CurrencySystem::TIER_HIGH, CurrencySystem::TIER_MID },
			{ CurrencySystem::TIER_HIGH, CurrencySystem::TIER_PEAK },
			{ CurrencySystem::TIER_PEAK, CurrencySystem::TIER_HIGH },
		};
		CurrencySystem *cs = CurrencySystem::get_singleton();
		if (!cs)
			return;
		int from = EX[sel][0], to = EX[sel][1];
		bool combine = from < to;
		int have = cs->get_amount(from);
		int qty = combine ? have / CurrencySystem::RATIO : have;
		if (qty <= 0) {
			_set_msg(LOC("灵石不足，无法兑换"));
			return;
		}
		if (cs->exchange(from, qty, to)) {
			_set_msg(LOC("兑换 ") + CurrencySystem::tier_name(from) + LOC(" → ") + CurrencySystem::tier_name(to));
		}
		_refresh();
		return;
	}
	if (_pane == 0) {
		// 买
		int sel = _grids[0]->get_selected();
		if (sel < 0 || sel >= int(_stock.size()))
			return;
		Dictionary s = _stock[sel];
		StringName id = s.get("id", StringName());
		const Item *def = ItemDatabase::get_singleton()->get_item(id);
		String name = def ? LOC(def->name) : String(id);
		if (shop->buy(_player, id))
			_set_msg(LOC("购得 ") + name);
		else
			_set_msg(LOC("灵石不足或背包已满"));
	} else {
		// 卖
		int sel = _grids[1]->get_selected();
		if (sel < 0 || sel >= int(_slots[0].size()))
			return;
		int real_slot = _slots[0][sel];
		Dictionary sd = _player->get_inventory()->get_slot(real_slot);
		StringName id = sd.get("id", StringName());
		const Item *def = ItemDatabase::get_singleton()->get_item(id);
		String name = def ? LOC(def->name) : String(id);
		if (shop->sell(_player, id))
			_set_msg(LOC("售出 ") + name);
		else
			_set_msg(LOC("商店不收此物"));
	}
	_refresh();
}

void ShopPanel::_input(const Ref<InputEvent> &p_event) {
	if (!_visible)
		return;
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo())
		return;
	// 切栏 Q/E（三栏循环：货架/背包/兑换；与 GameMenu 翻页一致）；←/→ 留给页内横向导航
	if (k->get_keycode() == KEY_Q) {
		_pane = (_pane + 2) % 3;
		_refresh();
	} else if (k->get_keycode() == KEY_E) {
		_pane = (_pane + 1) % 3;
		_refresh();
	}
}

void ShopPanel::_process(double p_delta) {
	if (!_visible)
		return;

	if (_msg_t > 0.0f) {
		_msg_t -= float(p_delta);
		if (_msg_t <= 0.0f)
			_msg->set_text(String());
	}

	Input *input = Input::get_singleton();

	if (input->is_action_just_pressed(LOC("menu"))) {
		close();
		return;
	}
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
		_trade();
	}
}

} // namespace godot
