#pragma once

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/variant/array.hpp>

#include <vector>

namespace godot {

class Label;
class ColorRect;
class CanvasItem;
class InventoryPanel;
class Player;
class ContinentManager;

// ESC 多页管理菜单——背包/能力/功法/技能/法宝/设置的统一入口。
// 专门的管理类（按设计：ESC 各页统一管理，不再各自为政）：
//   - 背包页：托管 InventoryPanel（外部驱动，菜单转发输入）
//   - 能力页：DNF 式技能树总览（主动/被动分区，境界能力解锁状态，v1 只读）
//   - 功法/技能/法宝页：占位（体系落地后填充，见 design/gongfa-skills.md）
//   - 设置页：主音量/保存/退出
// 打开时暂停树（嵌套暂停安全：记录并还原原暂停状态）。
// 层级：主页 105 < InventoryPanel 110 < 页签条 130（背包页时页签条仍可见）。
class GameMenu : public CanvasLayer {
	GDCLASS(GameMenu, CanvasLayer)

	enum Page { PAGE_INVENTORY = 0, PAGE_ABILITY, PAGE_GONGFA, PAGE_SKILL, PAGE_ARTIFACT, PAGE_SECT, PAGE_TRAVEL, PAGE_ALCHEMY, PAGE_SETTINGS, PAGE_COUNT };

	// 页签条（独立高层级 CanvasLayer，背包页压在 InventoryPanel 之上仍可见）
	CanvasLayer *_tabs_layer = nullptr;
	Label *_tab_labels[PAGE_COUNT] = {};
	Label *_hint_label = nullptr;

	// 主页内容
	ColorRect *_dim = nullptr;
	std::vector<CanvasItem *> _page_nodes;      // 当前页的节点（切页时清理）
	InventoryPanel *_inv_panel = nullptr;       // 托管的背包面板
	Player *_player = nullptr;                  // 打开时惰性查找（玩家可能被重挂载进秘境）

	int _page = PAGE_INVENTORY;
	bool _open = false;
	bool _restore_pause = false;

	// 炼丹页状态
	int _alchemy_sel = 0;
	String _alchemy_msg;
	float _alchemy_msg_t = 0.0f;
	void _handle_alchemy_input();

	// 技能页状态（主动装配：↑/↓ 选已学主动，A/S/D/F/T/Y 装入对应槽）
	int _skill_sel = 0;
	String _skill_msg;
	float _skill_msg_t = 0.0f;
	Array _skill_active_knowns() const; // 已学列表过滤掉被动（保持定义表顺序）
	void _handle_skill_input();

	// 宗门页状态（未入门=四宗列表选宗拜入；已入门=信息总览+X叛门）
	int _sect_sel = 0;
	String _sect_msg;
	float _sect_msg_t = 0.0f;
	void _handle_sect_input();

	// 云游页状态（四洲列表：↑/↓ 选洲，X 前往已解锁洲）
	int _travel_sel = 0;
	String _travel_msg;
	float _travel_msg_t = 0.0f;
	ContinentManager *_continent_mgr = nullptr;
	void _handle_travel_input();

	// 设置页状态
	int _settings_sel = 0;
	float _volume = 0.8f;
	float _saved_flash = 0.0f;

	void _open_menu(int p_page);
	void _close_menu();
	void _switch_page(int p_page);
	void _rebuild_page();
	void _refresh_tabs();
	void _set_hint(const String &p_text);

	void _build_ability_page();
	void _build_placeholder_page(const String &p_title, const PackedStringArray &p_lines);
	void _build_gongfa_page();
	void _build_skill_page();
	void _build_artifact_page();
	void _build_sect_page();
	void _build_travel_page();
	void _build_alchemy_page();
	void _build_settings_page();
	void _refresh_settings_page();

	void _handle_settings_input();
	void _apply_volume();
	void _load_settings();
	void _save_settings();

	Player *_find_player();

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;
};

} // namespace godot
