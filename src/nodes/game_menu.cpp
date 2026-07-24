#include "game_menu.h"

#include "../cultivation/ability_manager.h"
#include "../cultivation/gongfa_system.h"
#include "../nodes/inventory_panel.h"
#include "../nodes/player.h"
#include "../utils/text.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {

static const char *TAB_NAMES[] = { "背包", "能力", "功法", "技能", "法宝", "设置" };

void GameMenu::_bind_methods() {
}

void GameMenu::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_layer(105); // InventoryPanel(110) 之下
	set_process_mode(Node::PROCESS_MODE_ALWAYS);

	_dim = memnew(ColorRect);
	_dim->set_color(Color(0, 0, 0, 0.78f));
	_dim->set_anchors_preset(Control::PRESET_FULL_RECT);
	add_child(_dim);

	// 页签条：独立高层级，背包页(110)之上仍可见
	_tabs_layer = memnew(CanvasLayer);
	_tabs_layer->set_layer(130);
	add_child(_tabs_layer);

	for (int i = 0; i < PAGE_COUNT; i++) {
		Label *t = memnew(Label);
		t->set_text(TXT(TAB_NAMES[i]));
		t->add_theme_font_size_override("font_size", 10);
		t->set_position(Vector2(110 + i * 48, 12));
		_tabs_layer->add_child(t);
		_tab_labels[i] = t;
	}

	_hint_label = memnew(Label);
	_hint_label->add_theme_font_size_override("font_size", 8);
	_hint_label->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f));
	_hint_label->set_position(Vector2(140, 252));
	_tabs_layer->add_child(_hint_label);

	// 托管背包面板（bootstrap 已创建）
	Node *inv = get_parent()->get_node_or_null(NodePath("InventoryPanel"));
	_inv_panel = Object::cast_to<InventoryPanel>(inv);
	if (_inv_panel) {
		_inv_panel->set_external_drive(true);
	}

	_load_settings();
	_apply_volume();

	_dim->set_visible(false);
	_tabs_layer->set_visible(false);
}

Player *GameMenu::_find_player() {
	Node *scene = get_tree()->get_current_scene();
	if (!scene) return nullptr;
	return Object::cast_to<Player>(scene->find_child("Player", true, false));
}

// ============================================================
// 开关与切页
// ============================================================

void GameMenu::_open_menu(int p_page) {
	_open = true;
	_restore_pause = get_tree()->is_paused();
	get_tree()->set_pause(true);
	_player = _find_player();
	_page = p_page;
	_dim->set_visible(true);
	_tabs_layer->set_visible(true);
	_rebuild_page();
}

void GameMenu::_close_menu() {
	_open = false;
	if (_inv_panel) _inv_panel->close();
	for (CanvasItem *n : _page_nodes) {
		if (n) n->queue_free();
	}
	_page_nodes.clear();
	_dim->set_visible(false);
	_tabs_layer->set_visible(false);
	get_tree()->set_pause(_restore_pause); // 嵌套暂停：叙事 overlay 期间打开则还原为暂停
}

void GameMenu::_switch_page(int p_page) {
	_page = (p_page + PAGE_COUNT) % PAGE_COUNT;
	_rebuild_page();
}

void GameMenu::_refresh_tabs() {
	for (int i = 0; i < PAGE_COUNT; i++) {
		if (i == _page) {
			_tab_labels[i]->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
		} else {
			_tab_labels[i]->add_theme_color_override("font_color", Color(0.65f, 0.65f, 0.65f));
		}
	}
}

void GameMenu::_set_hint(const String &p_text) {
	_hint_label->set_text(p_text);
}

// ============================================================
// 页面构建
// ============================================================

void GameMenu::_rebuild_page() {
	for (CanvasItem *n : _page_nodes) {
		if (n) n->queue_free();
	}
	_page_nodes.clear();
	_refresh_tabs();

	switch (_page) {
		case PAGE_INVENTORY:
			if (_inv_panel) _inv_panel->open();
			_set_hint(TXT("←/→ 切换页  ↑/↓ 选择  E 使用/装备  ESC 关闭"));
			break;
		case PAGE_ABILITY:
			if (_inv_panel) _inv_panel->close();
			_build_ability_page();
			_set_hint(TXT("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_GONGFA:
			if (_inv_panel) _inv_panel->close();
			_build_gongfa_page();
			_set_hint(TXT("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_SKILL:
			if (_inv_panel) _inv_panel->close();
			_build_placeholder_page(TXT("技能"), {
				TXT("武技: [A]（空） [S]（空）"),
				TXT("法术: [D]（空） [F]（空）"),
				TXT(""),
				TXT("武技/法术/神通/仙法 统一技能管线落地后在此装配。"),
				TXT("B 键整页切换（法宝页/技能多页）随法宝系统开放。"),
			});
			_set_hint(TXT("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_ARTIFACT:
			if (_inv_panel) _inv_panel->close();
			_build_placeholder_page(TXT("法宝"), {
				TXT("本命法宝:（未定）"),
				TXT("次要法宝:（空） （空）"),
				TXT(""),
				TXT("本命法宝温养 120%→150%，渡劫觉醒 200%。"),
				TXT("法宝系统落地后在此管理，B 键战斗页/法宝页切换。"),
			});
			_set_hint(TXT("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_SETTINGS:
			if (_inv_panel) _inv_panel->close();
			_build_settings_page();
			_set_hint(TXT("←/→ 切换页  ↑/↓ 选择  ←/→ 调节  F 确认  ESC 关闭"));
			break;
	}
}

void GameMenu::_build_placeholder_page(const String &p_title, const PackedStringArray &p_lines) {
	Label *title = memnew(Label);
	title->set_text(TXT("—— ") + p_title + TXT(" ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 40));
	add_child(title);
	_page_nodes.push_back(title);

	for (int i = 0; i < p_lines.size(); i++) {
		Label *l = memnew(Label);
		l->set_text(p_lines[i]);
		l->add_theme_font_size_override("font_size", 9);
		l->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.85f));
		l->set_position(Vector2(90, 70 + i * 16));
		add_child(l);
		_page_nodes.push_back(l);
	}
}

// ============================================================
// 能力页（DNF 式技能树总览 v1：主动/被动分区，只读）
// ============================================================

void GameMenu::_build_ability_page() {
	struct AbilityRow {
		const char *id;     // AbilityManager 常量
		const char *name;
		bool innate;        // 初始即会（不入 AbilityManager 解锁表）
		const char *cond;   // 解锁条件显示文本
	};
	// 与 AbilityManager::check_realm_unlocks 对齐
	static const AbilityRow ACTIVE_ROWS[] = {
		{ AbilityManager::ABILITY_DASH,            "冲刺",     true,  "初始" },
		{ AbilityManager::ABILITY_DOUBLE_JUMP,     "二段跳",   false, "炼气" },
		{ AbilityManager::ABILITY_AIR_DASH,        "空中冲刺", false, "筑基" },
		{ AbilityManager::ABILITY_SHORT_FLIGHT,    "短暂飞行", false, "筑基" },
		{ AbilityManager::ABILITY_GLIDE,           "滑翔",     false, "金丹" },
		{ AbilityManager::ABILITY_FREE_FLIGHT,     "自主飞行", false, "金丹" },
		{ AbilityManager::ABILITY_SOUL_EXIT,       "元婴出窍", false, "元婴" },
		{ AbilityManager::ABILITY_DOMAIN,          "领域展开", false, "元婴" },
		{ AbilityManager::ABILITY_SPIRIT_TRAVEL,   "神游太虚", false, "化神" },
		{ AbilityManager::ABILITY_SPIRIT_SENSE,    "神识扫描", false, "化神" },
		{ AbilityManager::ABILITY_VOID_SHIFT,      "虚实转换", false, "炼虚" },
		{ AbilityManager::ABILITY_CLOUD_FLIGHT,    "腾云驾雾", false, "真仙" },
		{ AbilityManager::ABILITY_GIANT_FORM,      "法天象地", false, "金仙" },
		{ AbilityManager::ABILITY_DAO_DOMAIN,      "道域展开", false, "混元" },
		{ AbilityManager::ABILITY_MYRIAD_AVATARS,  "化身千万", false, "混元" },
	};
	static const AbilityRow PASSIVE_ROWS[] = {
		{ AbilityManager::ABILITY_WALL_CLING,     "攀墙",     true,  "初始" },
		{ AbilityManager::ABILITY_STORAGE_RING,   "纳戒",     false, "炼气" },
		{ AbilityManager::ABILITY_SPIRIT_VISION,  "灵视",     false, "筑基" },
		{ AbilityManager::ABILITY_UNITY_FORM,     "形神合一", false, "合体" },
		{ AbilityManager::ABILITY_MERIT_HALO,     "功德金光", false, "大乘" },
		{ AbilityManager::ABILITY_TRIBULATION_IMMUNITY,  "三灾免疫", false, "真仙" },
		{ AbilityManager::ABILITY_GOLDEN_BODY,    "金身护体", false, "金仙" },
	};

	AbilityManager *am = _player ? _player->get_ability_manager() : nullptr;

	auto build_column = [&](const AbilityRow *rows, int count, const String &header, float x) {
		Label *h = memnew(Label);
		h->set_text(header);
		h->add_theme_font_size_override("font_size", 11);
		h->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
		h->set_position(Vector2(x, 36));
		add_child(h);
		_page_nodes.push_back(h);

		for (int i = 0; i < count; i++) {
			bool unlocked = rows[i].innate || (am && am->has_ability(StringName(rows[i].id)));
			Label *l = memnew(Label);
			if (unlocked) {
				l->set_text(TXT("✓ ") + TXT(rows[i].name));
				l->add_theme_color_override("font_color", Color(0.55f, 0.95f, 0.55f));
			} else {
				l->set_text(TXT("✗ ") + TXT(rows[i].name) + TXT(" (") + TXT(rows[i].cond) + TXT(")"));
				l->add_theme_color_override("font_color", Color(0.45f, 0.45f, 0.45f));
			}
			l->add_theme_font_size_override("font_size", 9);
			l->set_position(Vector2(x, 54 + i * 13));
			add_child(l);
			_page_nodes.push_back(l);
		}
	};

	build_column(ACTIVE_ROWS, 15, TXT("— 主动 —"), 60.0f);
	build_column(PASSIVE_ROWS, 7, TXT("— 被动 —"), 280.0f);

	Label *note = memnew(Label);
	note->set_text(TXT("功法/武技/法术/神通/仙法 体系落地后入树"));
	note->add_theme_font_size_override("font_size", 7);
	note->add_theme_color_override("font_color", Color(0.5f, 0.5f, 0.5f));
	note->set_position(Vector2(60, 240));
	add_child(note);
	_page_nodes.push_back(note);
}

// ============================================================
// 功法页（炼体/练气双槽 + 熟练进度 + 加成总览）
// ============================================================

void GameMenu::_build_gongfa_page() {
	Label *title = memnew(Label);
	title->set_text(TXT("—— 功法 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 36));
	add_child(title);
	_page_nodes.push_back(title);

	GongfaSystem *gf = _player ? _player->get_gongfa() : nullptr;

	auto slot_text = [&](GongfaSystem::School school) -> PackedStringArray {
		PackedStringArray out;
		const GongfaSystem::SlotState &slot = gf ? gf->get_slot(school) : GongfaSystem::SlotState();
		if (!gf || slot.empty()) {
			out.append(TXT("（空）"));
			return out;
		}
		const GongfaSystem::Def *def = GongfaSystem::find_def(slot.id);
		if (!def) {
			out.append(TXT("（空）"));
			return out;
		}
		String line1 = TXT(def->name) + TXT("  ") + GongfaSystem::grade_name(def->grade) +
			TXT("  第") + String::num_int64(slot.layer) + TXT("/") + String::num_int64(def->max_layer) + TXT("层");
		out.append(line1);
		if (slot.layer >= def->max_layer) {
			out.append(TXT("熟练: 圆满"));
		} else {
			float pct = slot.prof / gf->prof_threshold(slot.layer) * 100.0f;
			out.append(TXT("熟练: ") + String::num(pct, 0) + TXT("%"));
		}
		return out;
	};

	auto add_line = [&](const String &text, float x, float y, int size, const Color &c) {
		Label *l = memnew(Label);
		l->set_text(text);
		l->add_theme_font_size_override("font_size", size);
		l->add_theme_color_override("font_color", c);
		l->set_position(Vector2(x, y));
		add_child(l);
		_page_nodes.push_back(l);
	};

	Color head_c(1.0f, 0.85f, 0.5f);
	Color body_c(0.85f, 0.85f, 0.85f);
	Color dim_c(0.55f, 0.55f, 0.55f);

	add_line(TXT("— 炼体 —"), 70.0f, 60.0f, 10, head_c);
	PackedStringArray body_lines = slot_text(GongfaSystem::SCHOOL_BODY);
	for (int i = 0; i < body_lines.size(); i++) {
		add_line(body_lines[i], 70.0f, 76.0f + i * 14, 9, body_c);
	}
	add_line(TXT("— 练气 —"), 280.0f, 60.0f, 10, head_c);
	PackedStringArray qi_lines = slot_text(GongfaSystem::SCHOOL_QI);
	for (int i = 0; i < qi_lines.size(); i++) {
		add_line(qi_lines[i], 280.0f, 76.0f + i * 14, 9, body_c);
	}

	// 加成总览
	if (gf) {
		auto pct = [](float m) { return String::num((m - 1.0f) * 100.0f, 0) + TXT("%"); };
		add_line(TXT("加成总览:"), 70.0f, 130.0f, 9, head_c);
		add_line(TXT("生命 +") + pct(gf->get_hp_mult()) + TXT("   防御 +") + pct(gf->get_def_mult()) +
		         TXT("   物攻 +") + pct(gf->get_atk_mult()), 70.0f, 146.0f, 9, body_c);
		add_line(TXT("灵力 +") + pct(gf->get_mana_mult()) + TXT("   回灵 +") + pct(gf->get_regen_mult()) +
		         TXT("   法强 +") + pct(gf->get_spell_mult()) + TXT("   速度 +") + pct(gf->get_speed_mult()),
		         70.0f, 162.0f, 9, body_c);
	}

	add_line(TXT("最多同修一门炼体 + 一门练气；炼体行为（受击/近战击杀）主养炼体，"), 70.0f, 196.0f, 8, dim_c);
	add_line(TXT("练气行为（耗灵/施法）主养练气，副系亦得两成熟练。切换保留熟练。"), 70.0f, 210.0f, 8, dim_c);
}

// ============================================================
// 设置页
// ============================================================

void GameMenu::_build_settings_page() {
	_refresh_settings_page();
}

void GameMenu::_refresh_settings_page() {
	for (CanvasItem *n : _page_nodes) {
		if (n) n->queue_free();
	}
	_page_nodes.clear();

	static const char *ITEM_NAMES[3] = { "保存游戏", "退出游戏", "" };
	String vol = TXT("主音量  < ") + String::num_int64(int64_t(Math::round(_volume * 100.0f))) + TXT("% >");
	String names[3] = { vol, TXT(ITEM_NAMES[0]), TXT(ITEM_NAMES[1]) };

	Label *title = memnew(Label);
	title->set_text(TXT("—— 设置 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 40));
	add_child(title);
	_page_nodes.push_back(title);

	for (int i = 0; i < 3; i++) {
		Label *l = memnew(Label);
		if (_saved_flash > 0.0f && i == 1) {
			l->set_text(TXT("   已存档！"));
		} else if (i == _settings_sel) {
			l->set_text(TXT("▶ ") + names[i]);
		} else {
			l->set_text(TXT("   ") + names[i]);
		}
		l->add_theme_font_size_override("font_size", 10);
		l->add_theme_color_override("font_color",
			i == _settings_sel ? Color(1.0f, 0.95f, 0.6f) : Color(0.85f, 0.85f, 0.85f));
		l->set_position(Vector2(170, 80 + i * 22));
		add_child(l);
		_page_nodes.push_back(l);
	}
}

void GameMenu::_handle_settings_input() {
	Input *input = Input::get_singleton();

	if (input->is_action_just_pressed(TXT("up"))) {
		_settings_sel = (_settings_sel + 2) % 3;
		_refresh_settings_page();
	}
	if (input->is_action_just_pressed(TXT("down"))) {
		_settings_sel = (_settings_sel + 1) % 3;
		_refresh_settings_page();
	}
	if (_settings_sel == 0) {
		if (input->is_action_just_pressed(TXT("left"))) {
			_volume = CLAMP(_volume - 0.1f, 0.0f, 1.0f);
			_apply_volume(); _save_settings(); _refresh_settings_page();
		}
		if (input->is_action_just_pressed(TXT("right"))) {
			_volume = CLAMP(_volume + 0.1f, 0.0f, 1.0f);
			_apply_volume(); _save_settings(); _refresh_settings_page();
		}
	}
	if (input->is_action_just_pressed(TXT("interact")) || input->is_action_just_pressed(TXT("action"))) {
		switch (_settings_sel) {
			case 0:
				_volume = CLAMP(_volume + 0.1f, 0.0f, 1.0f);
				_apply_volume(); _save_settings(); _refresh_settings_page();
				break;
			case 1: {
				Node *gm = get_tree()->get_current_scene()->get_node_or_null(NodePath("GameManager"));
				if (gm) {
					gm->call("save_game", String("auto"));
					_saved_flash = 1.5f;
					_refresh_settings_page();
				}
				break;
			}
			case 2:
				get_tree()->quit();
				break;
		}
	}
}

void GameMenu::_apply_volume() {
	AudioServer *as = AudioServer::get_singleton();
	if (!as) return;
	as->set_bus_mute(0, _volume <= 0.001f);
	if (_volume > 0.001f) {
		as->set_bus_volume_db(0, Math::linear2db(_volume));
	}
}

void GameMenu::_load_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	if (cfg->load(TXT("user://settings.cfg")) == OK) {
		_volume = CLAMP(float(cfg->get_value(TXT("audio"), TXT("master_volume"), 0.8f)), 0.0f, 1.0f);
	}
}

void GameMenu::_save_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->set_value(TXT("audio"), TXT("master_volume"), _volume);
	cfg->save(TXT("user://settings.cfg"));
}

// ============================================================
// 输入轮询
// ============================================================

void GameMenu::_process(double p_delta) {
	Input *input = Input::get_singleton();

	if (!_open) {
		if (input->is_action_just_pressed(TXT("menu"))) {
			_open_menu(_page); // 记住上次页
		} else if (input->is_action_just_pressed(TXT("inventory"))) {
			_open_menu(PAGE_INVENTORY);
		}
		return;
	}

	if (_saved_flash > 0.0f) {
		_saved_flash -= float(p_delta);
		if (_saved_flash <= 0.0f && _page == PAGE_SETTINGS) {
			_refresh_settings_page();
		}
	}

	if (input->is_action_just_pressed(TXT("menu"))) {
		_close_menu();
		return;
	}
	if (input->is_action_just_pressed(TXT("inventory"))) {
		if (_page == PAGE_INVENTORY) {
			_close_menu();
		} else {
			_switch_page(PAGE_INVENTORY);
		}
		return;
	}
	bool lr_for_volume = (_page == PAGE_SETTINGS && _settings_sel == 0);
	if (input->is_action_just_pressed(TXT("left")) && !lr_for_volume) {
		_switch_page(_page - 1);
		return;
	}
	if (input->is_action_just_pressed(TXT("right")) && !lr_for_volume) {
		_switch_page(_page + 1);
		return;
	}

	switch (_page) {
		case PAGE_INVENTORY:
			if (input->is_action_just_pressed(TXT("up")))   _inv_panel->ext_navigate(-1);
			if (input->is_action_just_pressed(TXT("down"))) _inv_panel->ext_navigate(+1);
			if (input->is_action_just_pressed(TXT("action"))) _inv_panel->ext_use();
			break;
		case PAGE_SETTINGS:
			_handle_settings_input();
			break;
		default:
			break;
	}
}

} // namespace godot
