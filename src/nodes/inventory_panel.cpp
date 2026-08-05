module;
#include "../nodes/player.h"
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.nodes;
import mcpp_kaki.cultivation;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;
namespace godot {

static const char *EQUIP_SLOT_NAMES[] = { "武器", "护甲", "饰品" };

// 类型筛选选项：0=全部（显示所有），1..4 映射到 Item::Type
static const char *FILTER_NAMES[] = { "全部", "消耗品", "材料", "装备", "关键物品" };
static const int FILTER_TYPE[] = { Item::CONSUMABLE, Item::MATERIAL, Item::EQUIPMENT, Item::KEY_ITEM };

void InventoryPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_player", "player"), &InventoryPanel::set_player);
	ClassDB::bind_method(D_METHOD("ext_navigate", "dir"), &InventoryPanel::ext_navigate);
	ClassDB::bind_method(D_METHOD("ext_navigate_h", "dir"), &InventoryPanel::ext_navigate_h);
	ClassDB::bind_method(D_METHOD("ext_use"), &InventoryPanel::ext_use);
	ClassDB::bind_method(D_METHOD("set_selected_index", "idx"), &InventoryPanel::set_selected_index);
	ClassDB::bind_method(D_METHOD("toggle"), &InventoryPanel::toggle);
	ClassDB::bind_method(D_METHOD("refresh", "item_id", "qty"), &InventoryPanel::refresh,
		DEFVAL(String()), DEFVAL(0));
}

void InventoryPanel::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_layer(110); // Above GameHUD (layer 100)

	_signal_bus = SignalBus::get_singleton();

	_build_background();
	_build_equipment_section();
	_build_item_list();
	_build_stats();
	_build_close_hint();

	// Hidden by default
	set_visible(false);

	// Listen for inventory-related events to auto-refresh
	if (_signal_bus) {
		_signal_bus->connect("item_picked_up", Callable(this, "refresh"));
		_signal_bus->connect("item_used", Callable(this, "refresh"));
		_signal_bus->connect("language_changed", Callable(this, "_on_language_changed"));
	}

	// Enable input processing
	set_process_input(true);
}

// ============================================================
// Toggle & Refresh
// ============================================================

void InventoryPanel::toggle() {
	_visible = !_visible;
	set_visible(_visible);
	if (_visible && _grid) {
		_grid->set_selected(0);
		_filter = 0; // 重开背包回到「全部」
		_filtering = false;
		_update_filter_label();
		refresh();
	}
}

void InventoryPanel::refresh(const String &p_item_id, int p_qty) {
	if (!_player) return;

	Inventory *inv = _player->get_inventory();
	if (!inv) return;

	// ---- Equipment section ----
	for (int i = 0; i < 3; i++) {
		StringName equipped = _player->get_equipment_in_slot(i);
		if (equipped.is_empty()) {
			_equip_names[i]->set_text(LOC("空"));
			_equip_names[i]->add_theme_color_override("font_color",
				Color(0.4f, 0.4f, 0.4f, 1));
		} else {
			const Item *def = ItemDatabase::get_singleton()->get_item(equipped);
			if (def) {
				String txt = LOC(def->name);
				if (def->attack_bonus > 0) txt += " +" + String::num(int(def->attack_bonus)) + " ATK";
				if (def->defense_bonus > 0) txt += " +" + String::num(int(def->defense_bonus)) + " DEF";
				if (def->speed_bonus > 0) txt += " +" + String::num(int(def->speed_bonus * 100)) + "% SPD";
				_equip_names[i]->set_text(txt);
			}
			_equip_names[i]->add_theme_color_override("font_color",
				Color(1.0f, 0.95f, 0.5f, 1));
		}
	}

	// ---- Item grid（统一 GridList；紧凑非空槽）----
	_slot_map.clear();
	Array items;
	int visible_slots = 8;
	int displayed = 0;

	int cap = inv->get_capacity();
	for (int slot_idx = 0; slot_idx < cap; slot_idx++) {
		Dictionary slot_data = inv->get_slot(slot_idx);
		if (slot_data.is_empty())
			continue;

		StringName item_id = slot_data["id"];
		int qty = slot_data["quantity"];
		const Item *def = ItemDatabase::get_singleton()->get_item(item_id);
		if (!_filter_matches(def))
			continue; // 类型筛选：非当前类型跳过

		Dictionary cell;
		String txt = def ? LOC(def->name) : String(item_id);
		if (qty > 1)
			txt += " ×" + String::num_int64(qty);
		cell["text"] = txt;

		// 类型色（与 ItemPickup 视觉同口径）：消耗品红/材料蓝/装备青/关键物品金
		Color c = Color(0.85f, 0.85f, 0.85f, 1.0f);
		if (def) {
			switch (def->type) {
				case Item::CONSUMABLE: c = Color(1.0f, 0.55f, 0.5f, 1.0f); break;
				case Item::MATERIAL:   c = Color(0.5f, 0.75f, 1.0f, 1.0f); break;
				case Item::EQUIPMENT:  c = Color(0.55f, 0.95f, 0.85f, 1.0f); break;
				case Item::KEY_ITEM:   c = Color(1.0f, 0.85f, 0.4f, 1.0f); break;
			}
		}
		cell["color"] = c;
		items.push_back(cell);
		_slot_map.push_back(slot_idx);
		displayed++;
	}
	_grid->set_items(items);

	// 选中项操作提示（筛选行时显示筛选说明）
	String hint;
	if (_filtering) {
		hint = LOC("←/→ 筛选类型  ↑/↓ 返回");
	} else if (!_slot_map.empty()) {
		int sel = _grid->get_selected();
		Dictionary slot_data = inv->get_slot(_slot_map[sel]);
		const Item *def = ItemDatabase::get_singleton()->get_item(slot_data.get("id", StringName()));
		if (def && def->type == Item::CONSUMABLE)
			hint = LOC("[X] 使用");
		else if (def && def->type == Item::EQUIPMENT)
			hint = LOC("[X] 装备");
		if (def) {
			hint = (def ? LOC(def->name) : String()) + "  " + hint;
		}
	}
	_action_hint->set_text(hint);

	// 选中项说明（物品 desc：效果/来源；筛选行/空槽则给操作提示）
	if (_desc_label) {
		String desc;
		if (_filtering) {
			desc = LOC("←/→ 筛选类型  ↑/↓ 返回");
		} else if (!_slot_map.empty()) {
			int sel = _grid->get_selected();
			Dictionary slot_data = inv->get_slot(_slot_map[sel]);
			const Item *def = ItemDatabase::get_singleton()->get_item(slot_data.get("id", StringName()));
			if (def)
				desc = LOC(def->description);
		}
		_desc_label->set_text(desc);
	}

	// ---- Stats ----
	String stats_txt;
	stats_txt += "HP: " + String::num(int(_player->current_health)) + "/" +
		String::num(int(_player->max_health));
	stats_txt += "  ATK: " + String::num(int(_player->get_effective_attack()));
	float atk_bonus = _player->get_equip_bonus_attack();
	if (atk_bonus > 0) stats_txt += "(+" + String::num(int(atk_bonus)) + ")";
	float def_bonus = _player->get_equip_bonus_defense();
	stats_txt += "  DEF: " + String::num(int(def_bonus));
	float spd_bonus = _player->get_equip_bonus_speed();
	stats_txt += "  SPD: " + String::num(int(_player->move_speed));
	if (spd_bonus > 0) stats_txt += "(+" + String::num(int(spd_bonus * 100)) + "%)";

	if (_player->get_cultivation()) {
		stats_txt += "\n";
		stats_txt += _player->get_cultivation()->get_full_title();
		stats_txt += LOC("  修为: ") +
			String::num_int64(int64_t(_player->get_cultivation()->get_realm_progress() * 100.0f)) + "%";
		stats_txt += "  " + _player->get_cultivation()->get_mana_name() + ": " +
			String::num_int64(int64_t(_player->get_cultivation()->get_mana()));
		stats_txt += "/" + String::num_int64(int64_t(_player->get_cultivation()->get_max_mana()));
	}

	_stats_label->set_text(stats_txt);
	_update_filter_label();
}

// ============================================================
// Input handling
// ============================================================

void InventoryPanel::set_selected_index(int p_idx) {
	if (_grid)
		_grid->set_selected(p_idx);
}

void InventoryPanel::ext_navigate(int p_dir) {
	if (!_player || !_grid) return;
	if (_filtering) {
		// 筛选行 ↑/↓ 均返回网格
		_filtering = false;
		_grid->set_selected(0);
		_update_filter_label();
		refresh();
		return;
	}
	// 顶行再按上 → 进入筛选行
	if (p_dir < 0 && _grid->get_selected() / _grid->get_columns() == 0) {
		_filtering = true;
		_update_filter_label();
		refresh();
		return;
	}
	// 上下 = 网格行移动（±列数由 GridList 处理）
	_grid->move_selection(0, p_dir < 0 ? -1 : +1);
	refresh();
}

void InventoryPanel::ext_navigate_h(int p_dir) {
	if (!_player || !_grid) return;
	if (_filtering) {
		// 筛选行：←/→ 循环切换类型
		_filter = CLAMP(_filter + (p_dir < 0 ? -1 : +1), 0, FILTER_COUNT - 1);
		_update_filter_label();
		refresh();
		return;
	}
	// 左右 = 网格列移动（行内钳制，不跨行回卷）
	_grid->move_selection(p_dir < 0 ? -1 : +1, 0);
	refresh();
}

void InventoryPanel::ext_use() {
	if (!_player || !_grid) return;
	if (_filtering) {
		// X 确认筛选 → 返回网格
		_filtering = false;
		_grid->set_selected(0);
		_update_filter_label();
		refresh();
		return;
	}
	Inventory *inv = _player->get_inventory();
	if (!inv || _slot_map.empty()) return;

	int sel = _grid->get_selected();
	if (sel < 0 || sel >= int(_slot_map.size())) return;
	int slot_idx = _slot_map[sel];

	Dictionary slot_data = inv->get_slot(slot_idx);
	if (slot_data.is_empty()) return;

	StringName item_id = slot_data["id"];
	const Item *def = ItemDatabase::get_singleton()->get_item(item_id);
	if (!def) return;

	if (def->type == Item::CONSUMABLE) {
		// 消耗品效果统一走 Player::use_consumable（自动用/快捷栏同口径）
		_player->use_consumable(item_id);
	} else if (def->type == Item::EQUIPMENT) {
		_player->equip_item(slot_idx);
		if (_signal_bus) {
			_signal_bus->emit_signal("player_health_changed",
				_player->current_health, _player->max_health);
		}
	}

	refresh();
}

void InventoryPanel::_input(const Ref<InputEvent> &p_event) {
	if (!_visible || !_player || _external_drive) return;

	Ref<InputEventKey> key = p_event;
	if (key.is_null() || !key->is_pressed() || key->is_echo()) return;

	int keycode = key->get_keycode();

	switch (keycode) {
		case KEY_I:  // Toggle off
		case KEY_ESCAPE:
			toggle();
			return;

		case KEY_UP:
		case KEY_W:
			ext_navigate(-1);
			return;

		case KEY_DOWN:
		case KEY_S:
			ext_navigate(+1);
			return;

		case KEY_LEFT:
			ext_navigate_h(-1);
			return;

		case KEY_RIGHT:
			ext_navigate_h(+1);
			return;

		case KEY_E:
			ext_use();
			return;
	}
}

// ============================================================
// UI Building
// ============================================================

void InventoryPanel::_build_background() {
	_background = memnew(ColorRect);
	_background->set_name("InvBg");
	_background->set_position(Vector2(0, 0));
	_background->set_size(Vector2(480, 270));
	_background->set_color(Color(0.05f, 0.05f, 0.1f, 0.92f));
	add_child(_background);
}

void InventoryPanel::_build_equipment_section() {
	// Header
	_equip_header = memnew(Label);
	_equip_header->set_name("EquipHeader");
	_equip_header->set_position(Vector2(16, START_Y));
	_equip_header->add_theme_font_size_override("font_size", FONT_SZ_TITLE);
	_equip_header->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
	_equip_header->set_text(LOC("⚔ 装备"));
	add_child(_equip_header);

	for (int i = 0; i < 3; i++) {
		// Slot label
		_equip_labels[i] = memnew(Label);
		_equip_labels[i]->set_position(Vector2(16.0f + i * 150.0f, EQUIP_Y));
		_equip_labels[i]->add_theme_font_size_override("font_size", FONT_SZ);
		_equip_labels[i]->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f, 1));
		_equip_labels[i]->set_text(LOC(EQUIP_SLOT_NAMES[i]) + ":");
		add_child(_equip_labels[i]);

		// Equipped item name
		_equip_names[i] = memnew(Label);
		_equip_names[i]->set_position(Vector2(56.0f + i * 150.0f, EQUIP_Y));
		_equip_names[i]->add_theme_font_size_override("font_size", FONT_SZ);
		_equip_names[i]->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.5f, 1));
		_equip_names[i]->set_text(LOC("空"));
		add_child(_equip_names[i]);
	}
}

void InventoryPanel::_build_item_list() {
	// Header
	_inv_header = memnew(Label);
	_inv_header->set_name("InvHeader");
	_inv_header->set_position(Vector2(16, ITEM_LIST_Y - 16));
	_inv_header->add_theme_font_size_override("font_size", FONT_SZ_TITLE);
	_inv_header->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
	_inv_header->set_text(LOC("物品"));
	add_child(_inv_header);

	// 统一格子列表：6 列 × 6 行窗口（480×270 内 75×22 格；底行留说明区）
	_grid = memnew(GridList);
	_grid->set_name("ItemGrid");
	_grid->set_position(Vector2(16, ITEM_LIST_Y));
	_grid->set_size(Vector2(452, 132));
	add_child(_grid);
	_grid->set_columns(6);
	_grid->set_cell_size(Vector2(75, 22));

	// 类型筛选行（物品标题右侧）：[全部] 消耗品 材料 装备 关键物品，[活动项] 括起
	_filter_label = memnew(Label);
	_filter_label->set_position(Vector2(50, 56));
	_filter_label->add_theme_font_size_override("font_size", 11);
	add_child(_filter_label);
	_update_filter_label();

	// 选中项操作提示（网格下方一行）
	_action_hint = memnew(Label);
	_action_hint->set_position(Vector2(16, 206));
	_action_hint->add_theme_font_size_override("font_size", FONT_SZ);
	_action_hint->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.4f, 1));
	add_child(_action_hint);

	// 选中项说明（物品 desc：效果/来源，单行截断）
	_desc_label = memnew(Label);
	_desc_label->set_name("ItemDesc");
	_desc_label->set_position(Vector2(16, 222));
	_desc_label->set_size(Vector2(452, 16));
	_desc_label->add_theme_font_size_override("font_size", 11);
	_desc_label->add_theme_color_override("font_color", Color(0.75f, 0.8f, 0.88f, 1));
	_desc_label->set_clip_text(true);
	add_child(_desc_label);
}

void InventoryPanel::_build_stats() {
	_stats_label = memnew(Label);
	_stats_label->set_name("StatsLabel");
	_stats_label->set_position(Vector2(16, STATS_Y));
	_stats_label->add_theme_font_size_override("font_size", FONT_SZ);
	_stats_label->add_theme_color_override("font_color", Color(0.8f, 0.8f, 0.8f, 1));
	add_child(_stats_label);
}

void InventoryPanel::_build_close_hint() {
	_close_hint = memnew(Label);
	_close_hint->set_position(Vector2(350, START_Y));
	_close_hint->add_theme_font_size_override("font_size", FONT_SZ);
	_close_hint->add_theme_color_override("font_color", Color(0.5f, 0.5f, 0.5f, 1));
	_close_hint->set_text(LOC("[I/ESC] 关闭"));
	add_child(_close_hint);
}

void InventoryPanel::_on_language_changed(const String &p_locale) {
	// Update static section headers
	if (_equip_header) _equip_header->set_text(LOC("⚔ 装备"));
	for (int i = 0; i < 3; i++) {
		if (_equip_labels[i]) _equip_labels[i]->set_text(LOC(EQUIP_SLOT_NAMES[i]) + ":");
	}
	if (_inv_header) _inv_header->set_text(LOC("物品"));
	if (_close_hint) _close_hint->set_text(LOC("[I/ESC] 关闭"));
	// Refresh dynamic content (item names, stats, filter, etc.)
	refresh();
}

void InventoryPanel::_update_filter_label() {
	if (!_filter_label)
		return;
	String txt;
	for (int i = 0; i < FILTER_COUNT; i++) {
		if (i > 0)
			txt += " ";
		String name = LOC(FILTER_NAMES[i]);
		txt += (i == _filter) ? "[" + name + "]" : name;
	}
	_filter_label->set_text(txt);
	_filter_label->add_theme_color_override("font_color",
			_filtering ? Color(1.0f, 0.85f, 0.35f, 1.0f) : Color(0.72f, 0.72f, 0.72f, 1.0f));
}

bool InventoryPanel::_filter_matches(const Item *p_def) const {
	if (_filter == 0)
		return true; // 全部
	if (!p_def)
		return false;
	return (int)p_def->type == FILTER_TYPE[_filter - 1];
}

} // namespace godot
