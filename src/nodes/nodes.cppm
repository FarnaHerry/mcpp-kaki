// mcpp-kaki nodes module — UI classes that bind only primitive/custom-class params.
// Player/Enemy/CameraRoom2D/Portal/ItemPickup/HerbNode/TelemetryPanel stay headers:
// they bind godot engine pointer params (Node2D*/Object*) which trigger a
// make_property_info ADL failure inside module impl units.
module;

#include <vector>
#include <deque>

#include <godot-cpp-m/macros.h>

#include "../utils/text.h"

namespace godot {
class Player; // external (nodes header) — global fragment, no module linkage
class DongtianManager; // external (nodes header)
}

export module mcpp_kaki.nodes;

import godot_cpp;

import mcpp_kaki.utils;
import mcpp_kaki.inventory;
import mcpp_kaki.core;
import mcpp_kaki.cultivation;

namespace godot {

export class DamageNumbers : public Node {
	GDCLASS(DamageNumbers, Node)

	struct Entry {
		Node2D *root = nullptr;
		float t = 0.0f;
		float drift_x = 0.0f;
	};
	std::vector<Entry> _active;
	int _spawn_counter = 0;

	static constexpr float LIFETIME = 0.8f;
	static constexpr float RISE_SPEED = 28.0f;

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;

	void _on_damage_dealt(Vector2 p_world_pos, float p_amount, bool p_is_player_victim);
};
export class GameHUD : public CanvasLayer {
	GDCLASS(GameHUD, CanvasLayer);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _unhandled_input(const Ref<InputEvent> &p_event) override;

	void on_player_health_changed(float p_current, float p_max);
	void on_spiritual_energy_changed(int64_t p_current, int64_t p_max, float p_progress);
	void on_mana_changed(double p_current, double p_max);
	void on_realm_changed(int p_old_realm, int p_new_realm, const String &p_realm_name);
	void on_combo_changed(int p_count);
	void on_combo_ended(int p_final_count);
	void on_interaction_prompt(const String &p_text, bool p_show);
	void on_player_died();
	void on_player_respawned();
	void on_boss_fight_update(const String &p_name, double p_current, double p_max);
	void on_boss_fight_ended(const String &p_name);
	void on_buffs_changed(const Array &p_active);
	void on_continent_changed(const String &p_id, const String &p_name);
	void on_dongtian_entered();
	void on_dongtian_exited();
	void on_ledger_inspect(const Dictionary &p_data, bool p_show);
	void on_fullness_changed(float p_current, float p_max);
	void on_bigu_changed(bool p_bigu);
	void _on_language_changed(const String &p_locale);
	bool is_boss_bar_visible() const {
		for (const BossBarUi &b : _boss_bars) {
			if (b.bg && b.bg->is_visible()) return true;
		}
		return false;
	}
	String get_boss_bar_name() const {
		return _boss_bars.empty() ? String() : _boss_bars[0].name;
	}
	int get_boss_bar_count() const { return (int)_boss_bars.size(); }

	void set_hud_visible(bool p_visible);
	bool is_hud_visible() const { return _hud_visible; }
	void set_all_visible(bool p_visible) { set_visible(p_visible); }

	void set_health_bar_color(const Color &p_color) { _health_color = p_color; }
	Color get_health_bar_color() const { return _health_color; }
	void set_energy_bar_color(const Color &p_color) { _energy_color = p_color; }
	Color get_energy_bar_color() const { return _energy_color; }

protected:
	static void _bind_methods();

private:
	ColorRect *_health_bg = nullptr;
	ColorRect *_health_fill = nullptr;
	Label *_health_label = nullptr;
	float _health_current = 100.0f;
	float _health_max = 100.0f;
	Color _health_color = Color(0.85f, 0.2f, 0.2f, 1.0f);

	ColorRect *_energy_bg = nullptr;
	ColorRect *_energy_fill = nullptr;
	Label *_energy_label = nullptr;
	double _mana_current = 0.0;
	double _mana_max = 0.0;
	String _mana_prefix = LOC("灵力");
	Color _energy_color = Color(0.2f, 0.6f, 1.0f, 1.0f);

	Node *_xp_bg = nullptr;      // 修为经验圆底（Polygon2D 圆形）
	Node *_xp_fill = nullptr;    // 修为经验水位（Polygon2D 圆切片，进度=水位）
	Label *_xp_label = nullptr;
	float _xp_progress = 0.0f;
	String _xp_prefix = LOC("修为");
	Color _xp_color = Color(0.9f, 0.75f, 0.2f, 1.0f);

	// 饱食度条（食物/辟谷，design/cultivation-realms.md 饮食；辟谷后隐藏）
	ColorRect *_fullness_bg = nullptr;
	ColorRect *_fullness_fill = nullptr;
	Label *_fullness_label = nullptr;
	float _fullness_current = 100.0f;
	float _fullness_max = 100.0f;
	Color _fullness_color = Color(0.85f, 0.55f, 0.2f, 1.0f);
	bool _bigu = false;

	Label *_buff_label = nullptr;
	Array _buffs;
	float _buff_refresh = 0.0f;

	Label *_continent_label = nullptr;
	float _continent_banner_t = 0.0f;

	Label *_realm_label = nullptr;
	Label *_jiyuan_label = nullptr;
	ColorRect *_ledger_overlay = nullptr; // 查簿 overlay 底
	std::vector<Label *> _ledger_lines;   // 查簿 overlay 5 行
	String _realm_name = LOC("凡人");

	Label *_combo_label = nullptr;
	int _combo_count = 0;

	Label *_interact_label = nullptr;
	bool _prompt_showing = false;

	ColorRect *_death_overlay = nullptr;

	// 拾取提示（屏幕右侧中，最多 6 条，自上而下滚动；每条 2.5s 自消）
	std::vector<Label *> _pickup_labels;
	float _pickup_scroll = 0.0f;   // 总滚动偏移（超出可视区向下平移）
	float _pickup_scroll_t = 0.0f; // 自动滚动节拍
	float _pickup_slide_t = 0.0f;  // 新条目滑入计时
	Color _pickup_color = Color(1.0f, 0.92f, 0.55f, 1.0f);
	void _create_pickup_notify();
	void _on_item_picked_up(const String &p_item_id, int p_qty);
	void _layout_pickup_notify();
	Label *_spawn_pickup_label(const String &p_text);
	void _update_pickup(float p_delta);

	std::vector<CanvasItem *> _skill_bar_nodes;
	std::vector<Label *> _skill_name_labels;
	std::vector<Label *> _skill_cd_labels;
	std::vector<Label *> _bar_name_labels;
	std::vector<Label *> _bar_count_labels;
	Player *_player = nullptr;
	Label *_page_badge = nullptr;

	ColorRect *_law_bg = nullptr;
	ColorRect *_law_fill = nullptr;
	Label *_law_label = nullptr;
	bool _law_shown = false; // 法则条显示态（化神解锁；切换时左列重排补位）

	ColorRect *_wei_bg = nullptr;
	Label *_wei_label = nullptr;
	ColorRect *_lin_bg = nullptr;
	Label *_lin_label = nullptr;

	// 多 Boss 血条：按名维护（黑白无常同场各一条，自上而下排列）。
	// bg/fill/名字直接挂 CanvasLayer，名字用 font 精确度量绝对定位居中于血条
	struct BossBarUi {
		String name;
		String realm_tag; // 修为境界名（建条时按 enemies 组同名敌 realm 解析缓存，空=不显示）
		ColorRect *bg = nullptr;
		ColorRect *fill = nullptr;
		Label *name_label = nullptr;
		bool alive = false; // 血条激活（HUD 隐藏后按状态恢复）
	};
	std::deque<BossBarUi> _boss_bars; // deque 元素地址稳定（增删不失效）

	// 渲染比例自适应（content_scale_size 变更 → 右/中/底锚定元素重排；渲染分辨率与窗口解耦）
	float _vw = 480.0f;
	float _vh = 270.0f;

	bool _hud_visible = true;

	void _create_health_bar();
	void _create_energy_bar();
	void _create_xp_bar();
	void _create_realm_label();
	void _create_jiyuan_label();
	void _create_ledger_overlay();
	void _create_fullness_bar();
	void _create_combo_label();
	void _create_interact_prompt();
	void _create_death_overlay();
	void _create_skill_bar();
	void _create_consumable_bar();
	void _update_consumable_bar();
	void _create_law_bar();
	BossBarUi *_find_boss_bar(const String &p_name);
	BossBarUi *_create_boss_bar_item();
	void _relayout_boss_bars();
	String _boss_realm_tag(const String &p_name) const; // 按名找 enemies 组敌 → 境界名（realm<=0/找不到=空）
	void _position_boss_name(BossBarUi *p_bar, float p_x, float p_y);
	void _create_pressure_indicators();
	void _update_pressure_indicators();
	void _sync_viewport();
	void _relayout_hud();
	void _layout_right_side();
	void _layout_left_column(); // 左列动态堆叠（法则条显隐补位）
	void _layout_skill_bar();
	void _layout_consumable_bar();
	void _update_skill_bar();
	void _update_law_bar();
	void _update_buff_label(double p_delta);
	void _update_bar(ColorRect *p_fill, float p_current, float p_max, bool p_horizontal = true);
	void _refresh_mana_label();
	void _refresh_xp_label();
	void _refresh_realm_label();
	void _apply_hud_visibility();
};
export class GridList; // 前置声明：InventoryPanel/StoragePanel 引用

export class InventoryPanel : public CanvasLayer {
	GDCLASS(InventoryPanel, CanvasLayer);

public:
	void _ready() override;
	void _input(const Ref<InputEvent> &p_event) override;

	void set_player(Player *p) { _player = p; }

	void toggle();
	void open() { if (!_visible) toggle(); }
	void close() { if (_visible) toggle(); }

	void set_external_drive(bool p_on) { _external_drive = p_on; }
	void ext_navigate(int p_dir);
	void ext_navigate_h(int p_dir);
	void ext_use();
	void set_selected_index(int p_idx); // 紧凑格子索引（GridList 选中）

	void refresh(const String &p_item_id = String(), int p_qty = 0);

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	SignalBus *_signal_bus = nullptr;
	bool _visible = false;
	bool _external_drive = false;

	ColorRect *_background = nullptr;

	Label *_equip_header = nullptr;
	Label *_equip_labels[3];
	Label *_equip_names[3];

	GridList *_grid = nullptr;       // 物品格子列表（统一 GridList 组件）
	std::vector<int> _slot_map;      // 紧凑格子索引 → 背包真实槽位
	Label *_action_hint = nullptr;   // 选中项操作提示（[X]使用/装备）
	Label *_desc_label = nullptr;    // 选中项说明（物品 desc，网格下方单行）
	Label *_currency_label = nullptr; // 灵石余额（四阶，背包右下角）

	// ---- 类型筛选（全部/消耗品/材料/装备/关键物品）----
	static constexpr int FILTER_COUNT = 5;
	Label *_filter_buttons[FILTER_COUNT] = {}; // 筛选行逐项标签（选中高亮/切换中括起，同图鉴分类行）
	int _filter = 0;                 // 0=全部 1..4=Item::Type
	bool _filtering = false;         // 选中在筛选行（↑ 进入 / ↓或X 返回）

	Label *_stats_label = nullptr;

	Label *_close_hint = nullptr;

	static constexpr int START_Y = 20;
	static constexpr int EQUIP_Y = 32;
	static constexpr int ITEM_LIST_Y = 70;
	static constexpr int STATS_Y = 240;
	static constexpr int FONT_SZ = 12;
	static constexpr int FONT_SZ_TITLE = 14;

	void _build_background();
	void _build_equipment_section();
	void _build_item_list();
	void _build_stats();
	void _build_close_hint();
	void _handle_input_action(const String &p_action);
	void _on_language_changed(const String &p_locale);
	void _update_filter_label();
	bool _filter_matches(const Item *p_def) const;
};
// 统一格子列表：物品/技能/法宝/功法等一切列表条目的通用格子渲染。
// 数据 = Array of {text, color?, dim?}；无图标时代一律名字格子。
// 交互由宿主面板驱动（move_selection/get_selected），本组件只管渲染+网格导航+滚动。
export class GridList : public Control {
	GDCLASS(GridList, Control);

public:
	void _ready() override;

	void set_items(const Array &p_items); // Array of Dictionary {text, color?, dim?}
	int get_item_count() const { return int(_items.size()); }

	void set_columns(int p_cols);
	int get_columns() const { return _columns; }
	void set_cell_size(const Vector2 &p_size);
	void set_active(bool p_active); // 非激活栏（如仓库双栏的另一栏）暗化

	void set_selected(int p_idx);
	int get_selected() const { return _selected; }
	void move_selection(int p_dx, int p_dy); // 网格导航，自动滚动

	void refresh(); // 重建可见格子

protected:
	static void _bind_methods();

private:
	struct Cell {
		ColorRect *frame = nullptr; // 外框（选中=金）
		ColorRect *bg = nullptr;
		Label *label = nullptr;
	};

	Array _items;
	int _columns = 5;
	Vector2 _cell_size = Vector2(76, 22);
	int _selected = 0;
	int _scroll_row = 0;
	bool _active = true;
	bool _built = false;

	std::vector<Cell> _cells; // 可见窗口对象池
	int _pool_rows = 0;

	void _build_pool();
	void _ensure_visible();
};

// 洞天仓库面板（双栏：背包|仓库，X 移送整堆）——数据在 DongtianManager。
export class StoragePanel : public CanvasLayer {
	GDCLASS(StoragePanel, CanvasLayer);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _input(const Ref<InputEvent> &p_event) override;

	void set_player(Player *p) { _player = p; }
	void open();
	void close();
	bool is_open() const { return _visible; }

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	DongtianManager *_manager = nullptr;
	bool _visible = false;
	bool _restore_pause = false;

	int _pane = 0; // 0=背包 1=仓库
	GridList *_grids[2] = {};       // 双栏格子列表（统一 GridList 组件）
	std::vector<int> _slots[2];     // 紧凑非空列表 → 真实槽位索引

	ColorRect *_background = nullptr;
	Label *_title = nullptr;
	Label *_headers[2] = {};
	Label *_hint = nullptr;
	Label *_msg = nullptr;
	float _msg_t = 0.0f;

	DongtianManager *_find_manager();
	void _rebuild_lists();
	void _refresh();
	void _transfer();
	void _set_msg(const String &p_text);
};

// 长安坊市商店面板（双栏：商店货架|玩家背包，Q/E 切栏 X 购买/卖出）——灵石货币。
export class ShopPanel : public CanvasLayer {
	GDCLASS(ShopPanel, CanvasLayer);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _input(const Ref<InputEvent> &p_event) override;

	void set_player(Player *p) { _player = p; }
	void open();
	void close();
	bool is_open() const { return _visible; }

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	bool _visible = false;
	bool _restore_pause = false;

	int _pane = 0; // 0=商店货架 1=玩家背包 2=灵石兑换（Q/E 三栏循环）
	GridList *_grids[3] = {};   // 货架|背包|兑换 三格
	std::vector<int> _slots[1]; // 背包栏：紧凑非空→真实槽位（仅 pane1）
	Array _stock;               // 商店货架（{id,name,price}）

	ColorRect *_background = nullptr;
	Label *_title = nullptr;
	Label *_balance = nullptr; // 灵石余额（四阶）
	Label *_headers[3] = {};
	Label *_hint = nullptr;
	Label *_msg = nullptr;
	float _msg_t = 0.0f;

	class ShopSystem *_shop = nullptr;
	class ShopSystem *_find_shop();
	void _refresh();
	void _trade();
	void _set_msg(const String &p_text);
};
export class GameMenu : public CanvasLayer {
	GDCLASS(GameMenu, CanvasLayer)

	enum Page { PAGE_PROFILE = 0, PAGE_INVENTORY, PAGE_ABILITY, PAGE_GONGFA, PAGE_SKILL, PAGE_ARTIFACT, PAGE_SECT, PAGE_TRAVEL, PAGE_ALCHEMY, PAGE_SETTINGS, PAGE_BESTIARY, PAGE_COUNT };

	CanvasLayer *_tabs_layer = nullptr;
	Label *_tab_labels[PAGE_COUNT] = {};
	Label *_hint_label = nullptr;

	ColorRect *_dim = nullptr;
	std::vector<CanvasItem *> _page_nodes;
	InventoryPanel *_inv_panel = nullptr;
	Player *_player = nullptr;

	int _page = PAGE_INVENTORY;
	bool _open = false;
	bool _restore_pause = false;

	int _alchemy_sel = 0;
	String _alchemy_msg;
	float _alchemy_msg_t = 0.0f;
	void _handle_alchemy_input();

	int _forge_sub = 0; // 熔炼炉子页：0炼丹 1装备铸造 2法宝铸造 3装备强化
	int _forge_sel = 0; // 子页内选中索引
	bool _forge_sidebar_focus = false; // 侧边栏焦点：内容区顶行 ↑ 进入，↑/↓ 选子页 X 确认（背包筛选行同款）
	String _forge_msg;
	float _forge_msg_t = 0.0f;
	void _handle_forge_input();
	void _build_forge_page();
	void _build_forge_alchemy();
	void _build_forge_equip();
	void _build_forge_artifact();
	void _build_forge_upgrade();
	void _handle_forge_alchemy_input();
	void _handle_forge_equip_input();
	void _handle_forge_artifact_input();
	void _handle_forge_upgrade_input();

	int _skill_sel = 0;
	String _skill_msg;
	float _skill_msg_t = 0.0f;
	Array _skill_active_knowns() const;
	void _handle_skill_input();

	int _artifact_sel = 0;
	String _artifact_msg;
	float _artifact_msg_t = 0.0f;
	void _handle_artifact_input();
	// 温养进度条（选中项 + 开关变化后刷新，幅面固定不抖动）——详见 game_menu.cpp 法宝页
	String _artifact_nurture_line(ArtifactSystem *p_arts, const StringName &p_id);

	int _sect_sel = 0;
	String _sect_msg;
	float _sect_msg_t = 0.0f;
	void _handle_sect_input();

	int _travel_sel = 0;
	String _travel_msg;
	float _travel_msg_t = 0.0f;
	ContinentManager *_continent_mgr = nullptr;
	void _handle_travel_input();

	int _settings_sel = 0;
	float _volume = 0.8f;
	float _saved_flash = 0.0f;
	// 显示设置：窗口模式 3 档（0窗口/1无边框全屏/2独占全屏）+ 分辨率（原生分辨率档，
	// 常规语义：窗口/无边框全屏=窗口尺寸、独占全屏=显示模式影响渲染精度）。
	// 内部渲染比例固定 16:9（480×270），非 16:9 由 aspect=keep 居中黑边；缩放固定整数倍。
	int _window_mode_opt = 0;
	int _fs_res_idx = 2;    // FS_RES_PRESETS 下标（原生分辨率档），默认 1920×1080
	int _max_fps = -1;      // 帧率上限值（-1=未设置→默认系统最高刷新率；0=无限不锁帧）
	int _vsync = 1;         // 垂直同步 0关/1开，默认开
	std::vector<int> _fps_opts; // 帧率上限档（启动按系统最高刷新率动态生成，末尾 0=无限）
	bool _startup_applied = false; // 启动窗口就绪后应用一次显示设置（_ready 时窗口未完全就绪）

	// 键位配置子页（设置页「键位」行 X 进入）：运行时 InputMap 改绑 + 冲突检测（占用拒绑）
	// + user://keybinds.cfg 持久化（仅存与默认不同的覆盖项）+ 恢复默认。
	// 菜单自身 Q/E 翻页、ESC 关闭走 _input 原始键码，不纳入可改绑范围。
	bool _keybind_open = false;
	int _keybind_sel = 0;
	int _keybind_capture = -1;          // >=0 = 等待新键（KEYBIND_ROWS 下标）
	uint64_t _keybind_cancel_frame = 0; // ESC 取消捕获当帧：同帧 menu 轮询不再退层
	String _keybind_msg;
	float _keybind_msg_t = 0.0f;
	void _enter_keybinds();
	void _build_keybinds_page();
	void _handle_keybinds_input();
	void _capture_keybind(int32_t p_row, int32_t p_physical);
	int _keybind_conflict_row(int p_row, int32_t p_physical) const;
	bool _keybind_is_default(const String &p_action) const;
	int32_t _keybind_first_physical(const String &p_action) const;
	void _apply_keybind(const String &p_action, int32_t p_physical);
	void _reset_keybinds();
	void _load_keybinds();
	void _save_keybinds();
	String _keybind_key_text(const String &p_action) const;

	// 图鉴页（ESC 第 11 页，设置页之后）：分类 0物品 1敌人 2装备
	int _bestiary_cat = 0;
	int _bestiary_sel = 0;
	bool _bestiary_cat_focus = false; // 分类焦点模式：↑ 顶行进，←/→ 切分类，↓/X 返回内容
	String _bestiary_msg;
	float _bestiary_msg_t = 0.0f;
	void _build_bestiary_page();
	void _handle_bestiary_input();
	// 图鉴条目详情构建：返回 [名称, 描述]（从 items/enemies 定义读取，兜底 fallback）
	void _bestiary_entry_detail(const String &p_id, int p_cat, String &r_name, String &r_desc);

	void _open_menu(int p_page);
	void _close_menu();
	void _switch_page(int p_page);
	void _rebuild_page();
	void _refresh_tabs();
	void _set_hint(const String &p_text);

	void _build_profile_page();
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
	void _apply_fps();
	void _apply_vsync();
	void _build_fps_options();
	void _apply_display();
	void _apply_render_scale();
	void _load_settings();
	void _save_settings();
	void _on_language_changed(const String &p_locale);

	Player *_find_player();

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _input(const Ref<InputEvent> &p_event) override;

	void reload_keybinds(); // 重读 user://keybinds.cfg 应用到 InputMap（测试挂钩）
};
} // namespace godot
