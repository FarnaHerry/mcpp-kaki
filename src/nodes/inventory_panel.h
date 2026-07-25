#ifndef CPP_KAKI_INVENTORY_PANEL_H
#define CPP_KAKI_INVENTORY_PANEL_H

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class Player;
class Inventory;
class SignalBus;

// Inventory UI overlay — toggled with I key.
// Shows equipment slots, item list with selection cursor, and stat summary.
// Follows the same CanvasLayer + manual layout pattern as GameHUD.
class InventoryPanel : public CanvasLayer {
	GDCLASS(InventoryPanel, CanvasLayer);

public:
	void _ready() override;
	void _input(const Ref<InputEvent> &p_event) override;

	// Dependency injection
	void set_player(Player *p) { _player = p; }

	// Manual toggle from GDScript
	void toggle();
	void open() { if (!_visible) toggle(); }
	void close() { if (_visible) toggle(); }

	// 外部驱动模式（GameMenu 托管）：自身 _input 不再响应，由菜单转发操作
	void set_external_drive(bool p_on) { _external_drive = p_on; }
	void ext_navigate(int p_dir); // +1 下 / -1 上
	void ext_use();               // 使用/装备当前选中
	void set_selected_index(int p_idx) { _selected_index = p_idx; } // 测试/外部驱动

	// Rebuild the item list (called on inventory_changed)
	// item_picked_up/item_used signals carry (item_id, qty) — accepted and ignored
	void refresh(const String &p_item_id = String(), int p_qty = 0);

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	SignalBus *_signal_bus = nullptr;
	bool _visible = false;
	bool _external_drive = false;

	// UI elements
	ColorRect *_background = nullptr;

	// Equipment section
	Label *_equip_header = nullptr;
	Label *_equip_labels[3];   // weapon, armor, accessory
	Label *_equip_names[3];

	// Item list
	Label *_inv_header = nullptr;
	Label *_item_labels[8];    // show 8 items at a time
	int _scroll_offset = 0;
	int _selected_index = 0;
	Label *_scroll_hint = nullptr;

	// Stats
	Label *_stats_label = nullptr;

	// Close hint
	Label *_close_hint = nullptr;

	// Constants
	static constexpr int START_Y = 20;
	static constexpr int EQUIP_Y = 32;
	static constexpr int ITEM_LIST_Y = 70;
	static constexpr int ITEM_ROW_H = 16;
	static constexpr int STATS_Y = 240;
	static constexpr int FONT_SZ = 12;
	static constexpr int FONT_SZ_TITLE = 14;

	void _build_background();
	void _build_equipment_section();
	void _build_item_list();
	void _build_stats();
	void _build_close_hint();
	void _handle_input_action(const String &p_action);
};

} // namespace godot

#endif // CPP_KAKI_INVENTORY_PANEL_H
