#include "inventory_panel.h"

#include "../cultivation/cultivation_system.h"
#include "../inventory/item.h"
#include "../inventory/item_database.h"
#include "../inventory/inventory.h"
#include "../nodes/player.h"
#include "../utils/signal_bus.h"
#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

static const char *EQUIP_SLOT_NAMES[] = { "武器", "护甲", "饰品" };

void InventoryPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_player", "player"), &InventoryPanel::set_player);
	ClassDB::bind_method(D_METHOD("ext_navigate", "dir"), &InventoryPanel::ext_navigate);
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
	if (_visible) {
		_scroll_offset = 0;
		_selected_index = 0;
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
			_equip_names[i]->set_text(TXT("空"));
			_equip_names[i]->add_theme_color_override("font_color",
				Color(0.4f, 0.4f, 0.4f, 1));
		} else {
			const Item *def = ItemDatabase::get_singleton()->get_item(equipped);
			if (def) {
				String txt = def->name;
				if (def->attack_bonus > 0) txt += " +" + String::num(int(def->attack_bonus)) + " ATK";
				if (def->defense_bonus > 0) txt += " +" + String::num(int(def->defense_bonus)) + " DEF";
				if (def->speed_bonus > 0) txt += " +" + String::num(int(def->speed_bonus * 100)) + "% SPD";
				_equip_names[i]->set_text(txt);
			}
			_equip_names[i]->add_theme_color_override("font_color",
				Color(1.0f, 0.95f, 0.5f, 1));
		}
	}

	// ---- Item list ----
	int visible_slots = 8;
	int displayed = 0;

	for (int i = 0; i < visible_slots; i++) {
		int slot_idx = _scroll_offset + i;
		Dictionary slot_data = inv->get_slot(slot_idx);

		if (slot_data.is_empty()) {
			_item_labels[i]->set_text("");
			_item_labels[i]->add_theme_color_override("font_color",
				Color(0.3f, 0.3f, 0.3f, 1));
		} else {
			StringName item_id = slot_data["id"];
			int qty = slot_data["quantity"];
			const Item *def = ItemDatabase::get_singleton()->get_item(item_id);

			String txt = String::num_int64(slot_idx) + ": ";
			if (def) {
				txt += def->name;
			} else {
				txt += String(item_id);
			}
			if (qty > 1) {
				txt += " x" + String::num_int64(qty);
			}

			// Show action hint for selected item
			if (slot_idx == _selected_index) {
				txt = "> " + txt;
				if (def && def->type == Item::CONSUMABLE) {
					txt += TXT("  [E]使用");
				} else if (def && def->type == Item::EQUIPMENT) {
					txt += TXT("  [E]装备");
				}
				_item_labels[i]->add_theme_color_override("font_color",
					Color(1.0f, 1.0f, 0.3f, 1));
			} else {
				_item_labels[i]->add_theme_color_override("font_color",
					Color(0.8f, 0.8f, 0.8f, 1));
			}

			_item_labels[i]->set_text(txt);
		}

		if (!slot_data.is_empty()) {
			displayed++;
		}
	}

	// Scroll hint
	int total_items = inv->get_capacity() - inv->get_free_slot_count();
	if (total_items > visible_slots) {
		_scroll_hint->set_text(TXT("↑↓ 滚动  (") +
			String::num_int64(_scroll_offset + 1) + "-" +
			String::num_int64(Math::min(_scroll_offset + visible_slots, total_items)) +
			"/" + String::num_int64(total_items) + ")");
		_scroll_hint->set_visible(true);
	} else {
		_scroll_hint->set_visible(false);
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
		stats_txt += TXT("  修为: ") +
			String::num_int64(int64_t(_player->get_cultivation()->get_realm_progress() * 100.0f)) + "%";
		stats_txt += "  " + _player->get_cultivation()->get_mana_name() + ": " +
			String::num_int64(int64_t(_player->get_cultivation()->get_mana()));
		stats_txt += "/" + String::num_int64(int64_t(_player->get_cultivation()->get_max_mana()));
	}

	_stats_label->set_text(stats_txt);
}

// ============================================================
// Input handling
// ============================================================

void InventoryPanel::ext_navigate(int p_dir) {
	if (!_player) return;
	Inventory *inv = _player->get_inventory();
	if (!inv) return;

	if (p_dir < 0) {
		if (_selected_index > 0) {
			_selected_index--;
			if (_selected_index < _scroll_offset) {
				_scroll_offset = _selected_index;
			}
			refresh();
		}
	} else {
		if (_selected_index < inv->get_capacity() - 1) {
			_selected_index++;
			if (_selected_index >= _scroll_offset + 8) {
				_scroll_offset = _selected_index - 7;
			}
			refresh();
		}
	}
}

void InventoryPanel::ext_use() {
	if (!_player) return;
	Inventory *inv = _player->get_inventory();
	if (!inv) return;

	Dictionary slot_data = inv->get_slot(_selected_index);
	if (slot_data.is_empty()) return;

	StringName item_id = slot_data["id"];
	const Item *def = ItemDatabase::get_singleton()->get_item(item_id);
	if (!def) return;

	if (def->type == Item::CONSUMABLE) {
		// 消耗品效果统一走 Player::use_consumable（自动用/快捷栏同口径）
		_player->use_consumable(item_id);
	} else if (def->type == Item::EQUIPMENT) {
		_player->equip_item(_selected_index);
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
	_equip_header->set_text(TXT("⚔ 装备"));
	add_child(_equip_header);

	for (int i = 0; i < 3; i++) {
		// Slot label
		_equip_labels[i] = memnew(Label);
		_equip_labels[i]->set_position(Vector2(16.0f + i * 150.0f, EQUIP_Y));
		_equip_labels[i]->add_theme_font_size_override("font_size", FONT_SZ);
		_equip_labels[i]->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f, 1));
		_equip_labels[i]->set_text(TXT(EQUIP_SLOT_NAMES[i]) + ":");
		add_child(_equip_labels[i]);

		// Equipped item name
		_equip_names[i] = memnew(Label);
		_equip_names[i]->set_position(Vector2(56.0f + i * 150.0f, EQUIP_Y));
		_equip_names[i]->add_theme_font_size_override("font_size", FONT_SZ);
		_equip_names[i]->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.5f, 1));
		_equip_names[i]->set_text(TXT("空"));
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
	_inv_header->set_text(TXT("物品"));
	add_child(_inv_header);

	// Item labels
	for (int i = 0; i < 8; i++) {
		_item_labels[i] = memnew(Label);
		_item_labels[i]->set_position(Vector2(20, ITEM_LIST_Y + i * ITEM_ROW_H));
		_item_labels[i]->add_theme_font_size_override("font_size", FONT_SZ);
		add_child(_item_labels[i]);
	}

	// Scroll hint
	_scroll_hint = memnew(Label);
	_scroll_hint->set_position(Vector2(320, ITEM_LIST_Y + 8 * ITEM_ROW_H));
	_scroll_hint->add_theme_font_size_override("font_size", FONT_SZ);
	_scroll_hint->add_theme_color_override("font_color", Color(0.5f, 0.5f, 0.5f, 1));
	_scroll_hint->set_visible(false);
	add_child(_scroll_hint);
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
	_close_hint->set_text("[I/ESC] 关闭");
	add_child(_close_hint);
}

} // namespace godot
