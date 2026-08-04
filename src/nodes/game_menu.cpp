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

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

module mcpp_kaki.nodes;
import mcpp_kaki.cultivation;
import mcpp_kaki.core;
import mcpp_kaki.combat;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;
namespace godot {

static const char *TAB_NAMES[] = { "背包", "能力", "功法", "技能", "法宝", "宗门", "云游", "炼丹", "设置" };

void GameMenu::_bind_methods() {
}

void GameMenu::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	set_layer(105); // InventoryPanel(110) 之下
	set_process_mode(Node::PROCESS_MODE_ALWAYS);
	set_process_input(true);

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
		t->set_text(LOC(TAB_NAMES[i]));
		t->add_theme_font_size_override("font_size", 9);
		t->set_position(Vector2(42 + i * 44, 12));
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

	// 监听语言切换，刷新菜单文字
	SignalBus *sb = SignalBus::get_singleton();
	if (sb) {
		sb->connect("language_changed", Callable(this, "_on_language_changed"));
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
	// 云游页数据源：惰性查找（WorldCommon 中 ContinentManager 在 GameMenu 之后创建，
	// _ready 时还未存在；且旅行换场景后本对象整个重建）
	Node *scene = get_tree()->get_current_scene();
	_continent_mgr = scene ? Object::cast_to<ContinentManager>(scene->find_child("ContinentManager", true, false)) : nullptr;
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

void GameMenu::_input(const Ref<InputEvent> &p_event) {
	if (!_open) return;
	Ref<InputEventKey> k = p_event;
	if (k.is_null() || !k->is_pressed() || k->is_echo()) return;

	// Settings page: left/right adjust volume (row 0) or language (row 1)
	bool lr_for_settings = (_page == PAGE_SETTINGS && (_settings_sel == 0 || _settings_sel == 1));
	if (lr_for_settings) return;

	// Q = prev tab, E = next tab
	if (k->get_keycode() == KEY_Q) {
		_switch_page(_page - 1);
		return;
	}
	if (k->get_keycode() == KEY_E) {
		_switch_page(_page + 1);
		return;
	}
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
			_set_hint(LOC("←/→ 切换页  ↑/↓ 选择  X 使用/装备  ESC 关闭"));
			break;
		case PAGE_ABILITY:
			if (_inv_panel) _inv_panel->close();
			_build_ability_page();
			_set_hint(LOC("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_GONGFA:
			if (_inv_panel) _inv_panel->close();
			_build_gongfa_page();
			_set_hint(LOC("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_SKILL:
			if (_inv_panel) _inv_panel->close();
			_build_skill_page();
			_set_hint(LOC("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_ARTIFACT:
			if (_inv_panel) _inv_panel->close();
			_build_artifact_page();
			_set_hint(LOC("←/→ 切换页  ESC 关闭"));
			break;
		case PAGE_SECT:
			if (_inv_panel) _inv_panel->close();
			_build_sect_page();
			_set_hint(LOC("←/→ 切换页  ↑/↓ 选宗  X 拜入/叛门  ESC 关闭"));
			break;
		case PAGE_TRAVEL:
			if (_inv_panel) _inv_panel->close();
			_build_travel_page();
			_set_hint(LOC("←/→ 切换页  ↑/↓ 选洲  X 前往  ESC 关闭"));
			break;
		case PAGE_ALCHEMY:
			if (_inv_panel) _inv_panel->close();
			_build_alchemy_page();
			_set_hint(LOC("←/→ 切换页  ↑/↓ 选方  X 炼制  ESC 关闭"));
			break;
		case PAGE_SETTINGS:
			if (_inv_panel) _inv_panel->close();
			_build_settings_page();
			_set_hint(LOC("←/→ 切换页  ↑/↓ 选择  ←/→ 调节  X 确认  ESC 关闭"));
			break;
	}
}

void GameMenu::_build_placeholder_page(const String &p_title, const PackedStringArray &p_lines) {
	Label *title = memnew(Label);
	title->set_text(LOC("—— ") + p_title + LOC(" ——"));
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
				l->set_text(LOC("✓ ") + LOC(rows[i].name));
				l->add_theme_color_override("font_color", Color(0.55f, 0.95f, 0.55f));
			} else {
				l->set_text(LOC("✗ ") + LOC(rows[i].name) + LOC(" (") + LOC(rows[i].cond) + LOC(")"));
				l->add_theme_color_override("font_color", Color(0.45f, 0.45f, 0.45f));
			}
			l->add_theme_font_size_override("font_size", 9);
			l->set_position(Vector2(x, 54 + i * 13));
			add_child(l);
			_page_nodes.push_back(l);
		}
	};

	build_column(ACTIVE_ROWS, 15, LOC("— 主动 —"), 60.0f);
	build_column(PASSIVE_ROWS, 7, LOC("— 被动 —"), 280.0f);

	// 威压/灵压（先天战技，不占 AbilityManager 槽；凡人期即可施放，仅灵力门控）
	{
		Label *ph = memnew(Label);
		ph->set_text(LOC("— 战技 —"));
		ph->add_theme_font_size_override("font_size", 9);
		ph->add_theme_color_override("font_color", Color(0.9f, 0.75f, 0.4f));
		ph->set_position(Vector2(60, 225));
		add_child(ph);
		_page_nodes.push_back(ph);

		Label *v = memnew(Label);
		v->set_text(LOC("✓ 威压 V  — 慑服低阶（耗灵30 cd8s）"));
		v->add_theme_font_size_override("font_size", 8);
		v->add_theme_color_override("font_color", Color(0.55f, 0.9f, 0.55f));
		v->set_position(Vector2(60, 238));
		add_child(v);
		_page_nodes.push_back(v);

		Label *r = memnew(Label);
		r->set_text(LOC("✓ 灵压 R  — 法伤低阶/镇杀（耗灵60 cd15s）"));
		r->add_theme_font_size_override("font_size", 8);
		r->add_theme_color_override("font_color", Color(0.55f, 0.9f, 0.55f));
		r->set_position(Vector2(60, 250));
		add_child(r);
		_page_nodes.push_back(r);
	}
}

// ============================================================
// 功法页（炼体/练气双槽 + 熟练进度 + 加成总览）
// ============================================================

void GameMenu::_build_gongfa_page() {
	Label *title = memnew(Label);
	title->set_text(LOC("—— 功法 ——"));
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
			out.append(LOC("（空）"));
			return out;
		}
		const GongfaSystem::Def *def = GongfaSystem::find_def(slot.id);
		if (!def) {
			out.append(LOC("（空）"));
			return out;
		}
		String line1 = LOC(def->name) + LOC("  ") + GongfaSystem::grade_name(def->grade) +
			LOC("  第") + String::num_int64(slot.layer) + LOC("/") + String::num_int64(def->max_layer) + LOC("层");
		out.append(line1);
		if (slot.layer >= def->max_layer) {
			out.append(LOC("熟练: 圆满"));
		} else {
			float pct = slot.prof / gf->prof_threshold(slot.layer) * 100.0f;
			out.append(LOC("熟练: ") + String::num(pct, 0) + LOC("%"));
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

	add_line(LOC("— 炼体 —"), 70.0f, 60.0f, 10, head_c);
	PackedStringArray body_lines = slot_text(GongfaSystem::SCHOOL_BODY);
	for (int i = 0; i < body_lines.size(); i++) {
		add_line(body_lines[i], 70.0f, 76.0f + i * 14, 9, body_c);
	}
	add_line(LOC("— 练气 —"), 280.0f, 60.0f, 10, head_c);
	PackedStringArray qi_lines = slot_text(GongfaSystem::SCHOOL_QI);
	for (int i = 0; i < qi_lines.size(); i++) {
		add_line(qi_lines[i], 280.0f, 76.0f + i * 14, 9, body_c);
	}

	// 加成总览
	if (gf) {
		auto pct = [](float m) { return String::num((m - 1.0f) * 100.0f, 0) + LOC("%"); };
		add_line(LOC("加成总览:"), 70.0f, 130.0f, 9, head_c);
		add_line(LOC("生命 +") + pct(gf->get_hp_mult()) + LOC("   防御 +") + pct(gf->get_def_mult()) +
		         LOC("   物攻 +") + pct(gf->get_atk_mult()), 70.0f, 146.0f, 9, body_c);
		add_line(LOC("灵力 +") + pct(gf->get_mana_mult()) + LOC("   回灵 +") + pct(gf->get_regen_mult()) +
		         LOC("   法强 +") + pct(gf->get_spell_mult()) + LOC("   速度 +") + pct(gf->get_speed_mult()),
		         70.0f, 162.0f, 9, body_c);
	}

	add_line(LOC("最多同修一门炼体 + 一门练气；炼体行为（受击/近战击杀）主养炼体，"), 70.0f, 196.0f, 8, dim_c);
	add_line(LOC("练气行为（耗灵/施法）主养练气，副系亦得两成熟练。切换保留熟练。"), 70.0f, 210.0f, 8, dim_c);
}

// ============================================================
// 技能页（槽位总览 + 主动装配交互 + 被动分区）
// ============================================================

Array GameMenu::_skill_active_knowns() const {
	Array out;
	SkillSystem *skills = _player ? _player->get_skills() : nullptr;
	if (!skills) return out;
	Array known = skills->get_known_list();
	for (int i = 0; i < known.size(); i++) {
		Dictionary k = known[i];
		if (int(k.get("type", -1)) != SkillSystem::TYPE_PASSIVE) {
			out.append(k);
		}
	}
	return out;
}

void GameMenu::_build_skill_page() {
	Label *title = memnew(Label);
	title->set_text(LOC("—— 技能 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 32));
	add_child(title);
	_page_nodes.push_back(title);

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
	Color sel_c(1.0f, 0.95f, 0.6f);
	Color ok_c(0.6f, 1.0f, 0.6f);
	Color bad_c(1.0f, 0.5f, 0.5f);

	SkillSystem *skills = _player ? _player->get_skills() : nullptr;

	// 槽位总览（武技A/S 法术D/F 法宝G/H 神通T 仙法Y）
	static const char *KEYS[SkillSystem::SLOT_COUNT] = { "A", "S", "D", "F", "G", "H", "T", "Y" };
	static const char *GNAMES[5] = { "武技", "法术", "法宝", "神通", "仙法" };
	for (int group = 0; group < 5; group++) {
		add_line(LOC("— ") + LOC(GNAMES[group]) + LOC(" —"), 40.0f + group * 90.0f, 54.0f, 10, head_c);
		int first = group < 3 ? group * 2 : group + 3; // 0/2/4/6/7
		int count = group < 3 ? 2 : 1;
		for (int k = 0; k < count; k++) {
			int slot = first + k;
			String text = String("[") + KEYS[slot] + "] ";
			if (skills) {
				Dictionary info = skills->get_slot_info(slot);
				text += info.is_empty() ? LOC("（空）") : LOC(String(info.get("name", "")));
			} else {
				text += LOC("（空）");
			}
			add_line(text, 40.0f + group * 90.0f, 69.0f + k * 13, 9, body_c);
		}
	}

	// 主动技能列表（↑/↓ 选择，A/S/D/F/T/Y 装配到对应槽）
	add_line(LOC("已学主动:"), 40.0f, 102.0f, 9, head_c);
	Array actives = _skill_active_knowns();
	const int VISIBLE = 9;
	if (actives.is_empty()) {
		add_line(LOC("（尚未习得任何技能）"), 40.0f, 118.0f, 9, dim_c);
	} else {
		_skill_sel = CLAMP(_skill_sel, 0, (int)actives.size() - 1);
		int scroll = actives.size() > VISIBLE ? CLAMP(_skill_sel - VISIBLE / 2, 0, (int)actives.size() - VISIBLE) : 0;
		for (int row = 0; row < VISIBLE && scroll + row < actives.size(); row++) {
			int i = scroll + row;
			Dictionary k = actives[i];
			String line = (i == _skill_sel ? LOC("▶ ") : LOC("  ")) + LOC(String(k.get("name", ""))) +
				LOC(" · ") + String(k.get("type_name", "")) +
				LOC(" ×") + String::num(float(k.get("power", 1.0f)), 1) +
				LOC(" 冷却") + String::num(float(k.get("cooldown", 0.0f)), 1) + LOC("s");
			float mana = float(k.get("mana_cost", 0.0f));
			if (mana > 0.0f) {
				line += LOC(" 灵") + String::num_int64(int64_t(mana));
			}
			float law = float(k.get("law_cost", 0.0f));
			if (law > 0.0f) {
				line += LOC(" 法则") + String::num_int64(int64_t(law));
			}
			add_line(line, 40.0f, 118.0f + row * 13, 9, i == _skill_sel ? sel_c : body_c);
		}
	}

	// 被动分区（学会即常驻，不占槽；数值走乘区）
	add_line(LOC("已悟被动:"), 285.0f, 102.0f, 9, head_c);
	static const char *PAS_NAMES[7] = { "", "攻击", "移速", "防御", "回灵", "飞速", "法则回复" };
	if (skills) {
		Array known = skills->get_known_list();
		float y = 118.0f;
		int shown = 0;
		for (int i = 0; i < known.size(); i++) {
			Dictionary k = known[i];
			if (int(k.get("type", -1)) != SkillSystem::TYPE_PASSIVE) continue;
			int ps = CLAMP(int(k.get("passive_stat", 0)), 0, 6);
			int pct = int(Math::round(float(k.get("passive_value", 0.0f)) * 100.0f));
			String line = LOC(String(k.get("name", ""))) + LOC("  ") + LOC(PAS_NAMES[ps]) +
				LOC("+") + String::num_int64(pct) + LOC("%");
			add_line(line, 285.0f, y, 9, body_c);
			y += 13.0f;
			shown++;
		}
		if (shown == 0) {
			add_line(LOC("（尚未悟得被动）"), 285.0f, 118.0f, 9, dim_c);
		}
	}

	// 装配结果提示 / 操作说明
	if (!_skill_msg.is_empty()) {
		bool ok = _skill_msg.contains(LOC("已装配"));
		add_line(_skill_msg, 40.0f, 248.0f, 9, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓ 选择主动技，按 A/S/D/F/T/Y 装入对应槽；G/H 留给法宝页（B 键切换）。"), 40.0f, 248.0f, 8, dim_c);
	}
}

void GameMenu::_handle_skill_input() {
	SkillSystem *skills = _player ? _player->get_skills() : nullptr;
	if (!skills) return;
	Input *input = Input::get_singleton();
	Array actives = _skill_active_knowns();
	int count = actives.size();
	if (count == 0) return;
	_skill_sel = CLAMP(_skill_sel, 0, count - 1);
	if (input->is_action_just_pressed(LOC("up"))) {
		_skill_sel = (_skill_sel - 1 + count) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_skill_sel = (_skill_sel + 1) % count;
		_rebuild_page();
	}
	// 按槽键装配当前选中技能（G/H 槽留给法宝页，不参与装配）
	static const char *SLOT_ACTIONS[6] = { "skill_a", "skill_s", "skill_d", "skill_f", "skill_t", "skill_y" };
	static const int SLOT_IDX[6] = { 0, 1, 2, 3, 6, 7 };
	static const char *SLOT_KEYS[6] = { "A", "S", "D", "F", "T", "Y" };
	for (int i = 0; i < 6; i++) {
		if (input->is_action_just_pressed(LOC(SLOT_ACTIONS[i]))) {
			Dictionary k = actives[_skill_sel];
			StringName id = StringName(String(k.get("id", "")));
			if (skills->assign(SLOT_IDX[i], id)) {
				_skill_msg = LOC("已装配 [") + SLOT_KEYS[i] + LOC("] ") + LOC(String(k.get("name", "")));
			} else {
				_skill_msg = LOC(String(k.get("name", ""))) + LOC(" 是") + String(k.get("type_name", "")) +
					LOC("，与 [") + SLOT_KEYS[i] + LOC("] 槽类型不符");
			}
			_skill_msg_t = 2.5f;
			_rebuild_page();
			break;
		}
	}
}

// ============================================================
// 法宝页（本命 + 次要槽：系数/温养/祭出参数）
// ============================================================

void GameMenu::_build_artifact_page() {
	Label *title = memnew(Label);
	title->set_text(LOC("—— 法宝 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 36));
	add_child(title);
	_page_nodes.push_back(title);

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

	ArtifactSystem *arts = _player ? _player->get_artifacts() : nullptr;
	int limit = arts ? arts->get_slot_limit() : 3;
	static const char *KEYS[ArtifactSystem::MAX_SLOTS] = { "A", "S", "D", "F", "G", "H" };

	float y = 60.0f;
	for (int i = 0; i < ArtifactSystem::MAX_SLOTS; i++) {
		if (i >= limit) {
			add_line(LOC("—— 飞升后解锁 ——"), 70.0f, y, 8, dim_c);
			break;
		}
		String head = i == 0 ? LOC("— 本命法宝 —") : (i == 1 ? LOC("— 次要法宝 —") : String());
		if (!head.is_empty()) {
			add_line(head, 70.0f, y, 10, head_c);
			y += 15.0f;
		}
		Dictionary info = arts ? arts->get_slot_info(i) : Dictionary();
		String line = String("[") + KEYS[i] + "] ";
		if (info.is_empty()) {
			line += LOC("（空）");
			add_line(line, 70.0f, y, 9, body_c);
			y += 14.0f;
			continue;
		}
		line += LOC(String(info.get("name", ""))) + LOC("  ") + String(info.get("kind_name", "")) +
			LOC("  系数×") + String::num(float(info.get("coeff", 1.0f)), 2);
		add_line(line, 70.0f, y, 9, body_c);
		y += 14.0f;
		String sub;
		if (int(info.get("kind", 0)) == int(ArtifactSystem::KIND_ATTACK)) {
			sub = LOC("    祭出: 倍率×") + String::num(float(info.get("power", 1.0f)), 1) +
				LOC("  灵力") + String::num_int64(int64_t(float(info.get("mana_cost", 0.0f)))) +
				LOC("  冷却") + String::num(float(info.get("cooldown", 0.0f)), 1) + LOC("s");
		} else {
			sub = LOC("    常驻: 防御+") + String::num(float(info.get("passive_def", 0.0f)) * 100.0f, 0) +
				LOC("%×系数");
		}
		if (i == 0) {
			sub += LOC("  温养") + String::num_int64(int64_t(float(info.get("nurture", 0.0f)))) + LOC("/1000");
			if (_player && _player->is_benming_awakened()) {
				sub += LOC(" 已觉醒");
			}
		} else {
			sub += LOC("  温养") + String::num_int64(int64_t(float(info.get("nurture", 0.0f)))) + LOC("/600");
		}
		add_line(sub, 70.0f, y, 8, dim_c);
		y += 13.0f;
	}

	add_line(LOC("战斗中按 B 整页切换法宝页，A~H 即法宝快捷键；祭出复用技能管线，耗灵力。"), 70.0f, 232.0f, 8, dim_c);
	add_line(LOC("本命温养 120%→150%，渡劫觉醒 200% 并锁定；次要 100%→120%→150%。"), 70.0f, 246.0f, 8, dim_c);
}

// ============================================================
// 宗门页（design/sect-pressure.md：未入门=四宗列表拜师；已入门=信息总览+叛门）
// ============================================================

void GameMenu::_build_sect_page() {
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
	Color dim_c(0.5f, 0.5f, 0.5f);
	Color sel_c(1.0f, 0.95f, 0.6f);
	Color ok_c(0.6f, 1.0f, 0.6f);
	Color bad_c(1.0f, 0.5f, 0.5f);

	Label *title = memnew(Label);
	title->set_text(LOC("—— 宗门 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 32));
	add_child(title);
	_page_nodes.push_back(title);

	SectSystem *sect = _player ? _player->get_sect_system() : nullptr;
	auto pct = [](float m) { return String::num((m - 1.0f) * 100.0f, 0) + LOC("%"); };

	if (sect && sect->in_sect()) {
		// 已入门：信息总览
		Dictionary info = sect->get_sect_info();
		add_line(String(info["name"]) + LOC("  ") + String(info["rank_name"]), 70.0f, 58.0f, 11, head_c);
		add_line(String(info["desc"]), 70.0f, 76.0f, 8, dim_c);
		int contrib = info["contribution"];
		int next = info["next_rank_cost"];
		String prog = LOC("贡献 ") + String::num_int64(contrib);
		prog += next >= 0 ? LOC(" / 晋阶需 ") + String::num_int64(next) : LOC("（已至真传）");
		add_line(prog, 70.0f, 92.0f, 9, body_c);

		add_line(LOC("宗门加成:"), 70.0f, 116.0f, 9, head_c);
		PackedStringArray bonus;
		if (sect->get_atk_mult() > 1.0f) bonus.append(LOC("攻击+") + pct(sect->get_atk_mult()));
		if (sect->get_mana_mult() > 1.0f) bonus.append(LOC("灵力+") + pct(sect->get_mana_mult()));
		if (sect->get_regen_mult() > 1.0f) bonus.append(LOC("回灵+") + pct(sect->get_regen_mult()));
		if (sect->get_hp_mult() > 1.0f) bonus.append(LOC("生命+") + pct(sect->get_hp_mult()));
		if (sect->get_def_mult() > 1.0f) bonus.append(LOC("防御+") + pct(sect->get_def_mult()));
		if (sect->get_kill_xp_mult() > 1.0f) bonus.append(LOC("击杀修为+") + pct(sect->get_kill_xp_mult()));
		add_line(bonus.is_empty() ? LOC("（无）") : String("  ").join(bonus), 70.0f, 132.0f, 9, body_c);

		const SectSystem::Def *def = SectSystem::find_def(sect->get_sect_id());
		if (def) {
			add_line(LOC("入门已授专属技（见技能页，可自由装配）"), 70.0f, 156.0f, 8, dim_c);
		}
		add_line(LOC("击杀 +1 贡献，Boss +10；内门 100 / 真传 300。"), 70.0f, 196.0f, 8, dim_c);
		add_line(LOC("叛门贡献清零，已学技能保留（逐出师门不夺修为）。"), 70.0f, 210.0f, 8, dim_c);
		if (!_sect_msg.is_empty()) {
			add_line(_sect_msg, 70.0f, 236.0f, 9, bad_c);
		} else {
			add_line(LOC("按 X 叛出师门"), 70.0f, 236.0f, 9, bad_c);
		}
	} else {
		// 未入门：四宗列表
		add_line(LOC("散修 · 无门无派"), 70.0f, 54.0f, 10, dim_c);
		Array list = sect ? sect->get_sect_list() : Array();
		_sect_sel = CLAMP(_sect_sel, 0, (int)list.size() - 1);
		static const char *BONUS_LINE[4] = {
			"攻 +6/10/15% ｜ 专属：万剑归宗（武技）",
			"灵力+10/16/24% 回灵+10/15/22% ｜ 专属：太清神光（法术）",
			"生命+8/12/18% 防+4/6/10% ｜ 专属：玄龟护体（法术）",
			"击杀修为+15/25/40% 攻+3/5/8% ｜ 专属：血影斩（武技）",
		};
		for (int i = 0; i < list.size(); i++) {
			Dictionary c = list[i];
			bool is_sel = (i == _sect_sel);
			float y = 76.0f + i * 34;
			add_line((is_sel ? LOC("▶ ") : LOC("  ")) + String(c["name"]), 70.0f, y, 10, is_sel ? sel_c : body_c);
			add_line(LOC("    ") + String(c["desc"]) + LOC("  ") + LOC(BONUS_LINE[i]), 70.0f, y + 14, 8, dim_c);
		}
		if (!_sect_msg.is_empty()) {
			bool ok = _sect_msg.contains(LOC("拜入"));
			add_line(_sect_msg, 70.0f, 240.0f, 9, ok ? ok_c : bad_c);
		} else {
			add_line(LOC("炼气起可拜师。加成随职位升：外门→内门(100)→真传(300)。"), 70.0f, 240.0f, 8, dim_c);
		}
	}
}

void GameMenu::_handle_sect_input() {
	SectSystem *sect = _player ? _player->get_sect_system() : nullptr;
	if (!sect) return;
	Input *input = Input::get_singleton();
	if (sect->in_sect()) {
		if (input->is_action_just_pressed(LOC("interact"))) {
			String name = LOC(String(sect->get_sect_info().get("name", "")));
			_player->leave_sect();
			_sect_msg = LOC("已叛出") + name;
			_sect_msg_t = 2.5f;
			_rebuild_page();
		}
		return;
	}
	Array list = sect->get_sect_list();
	int count = list.size();
	if (count == 0) return;
	_sect_sel = CLAMP(_sect_sel, 0, count - 1);
	if (input->is_action_just_pressed(LOC("up"))) {
		_sect_sel = (_sect_sel - 1 + count) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_sect_sel = (_sect_sel + 1) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		Dictionary c = list[_sect_sel];
		StringName id = StringName(String(c["id"]));
		if (_player->join_sect(id)) {
			_sect_msg = LOC("已拜入") + String(c["name"]);
		} else {
			_sect_msg = LOC("炼气期方可拜师");
		}
		_sect_msg_t = 2.5f;
		_rebuild_page();
	}
}

// ============================================================
// 云游页（design/world-map.md：四大部洲列表 → X 前往已解锁洲）
// ============================================================

void GameMenu::_build_travel_page() {
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
	Color dim_c(0.5f, 0.5f, 0.5f);
	Color sel_c(1.0f, 0.95f, 0.6f);
	Color cur_c(0.6f, 1.0f, 0.6f);
	Color lock_c(0.45f, 0.45f, 0.45f);
	Color ok_c(0.6f, 1.0f, 0.6f);
	Color bad_c(1.0f, 0.5f, 0.5f);

	Label *title = memnew(Label);
	title->set_text(LOC("—— 云游图 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(185, 32));
	add_child(title);
	_page_nodes.push_back(title);

	if (!_continent_mgr) {
		add_line(LOC("（云路未通）"), 70.0f, 60.0f, 9, dim_c);
		return;
	}

	Array list = _continent_mgr->get_continent_list();
	_travel_sel = CLAMP(_travel_sel, 0, (int)list.size() - 1);
	for (int i = 0; i < list.size(); i++) {
		Dictionary c = list[i];
		bool unlocked = c["unlocked"];
		bool current = c["current"];
		bool is_sel = (i == _travel_sel);
		float y = 62.0f + i * 34;

		String line1 = (is_sel ? LOC("▶ ") : LOC("  ")) + String(c["name"]);
		if (current) line1 += LOC("  【当前】");
		else if (!unlocked) line1 += LOC("  未解锁");
		Color c1 = is_sel ? sel_c : (current ? cur_c : (unlocked ? body_c : lock_c));
		add_line(line1, 70.0f, y, 10, c1);

		String line2 = LOC("    ") + String(c["desc"]);
		if (!unlocked) {
			line2 += LOC("  ｜ 条件：") + String(c["gate"]);
		}
		add_line(line2, 70.0f, y + 14, 8, dim_c);
	}

	// 云游结果提示 / 说明
	if (!_travel_msg.is_empty()) {
		bool ok = _travel_msg.contains(LOC("已在此洲"));
		add_line(_travel_msg, 70.0f, 250.0f, 9, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("四大部洲，云游四方。境界不足者，云海难渡。"), 70.0f, 250.0f, 8, dim_c);
	}
}

void GameMenu::_handle_travel_input() {
	if (!_continent_mgr) return;
	Input *input = Input::get_singleton();
	Array list = _continent_mgr->get_continent_list();
	int count = list.size();
	if (count == 0) return;
	_travel_sel = CLAMP(_travel_sel, 0, count - 1);
	if (input->is_action_just_pressed(LOC("up"))) {
		_travel_sel = (_travel_sel - 1 + count) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_travel_sel = (_travel_sel + 1) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		Dictionary c = list[_travel_sel];
		String id = c["id"];
		if (bool(c["current"])) {
			_travel_msg = LOC("已在此洲");
			_travel_msg_t = 2.0f;
			_rebuild_page();
		} else if (_continent_mgr->can_travel(id)) {
			// 先关菜单还原暂停，再切场景（本节点随场景释放）
			_close_menu();
			_continent_mgr->travel_to(id);
		} else {
			_travel_msg = String(c["name"]) + LOC(" 未解锁：") + String(c["gate"]);
			_travel_msg_t = 2.5f;
			_rebuild_page();
		}
	}
}

// ============================================================
// 设置页
// ============================================================

// ============================================================
// 炼丹页（design/alchemy.md：丹炉随身，配方列表 → 材料够=亮/不够=灰 → X 炼制）
// ============================================================

void GameMenu::_build_alchemy_page() {
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
	Color dim_c(0.45f, 0.45f, 0.45f);
	Color sel_c(1.0f, 0.95f, 0.6f);
	Color ok_c(0.6f, 1.0f, 0.6f);
	Color bad_c(1.0f, 0.5f, 0.5f);

	Label *title = memnew(Label);
	title->set_text(LOC("—— 炼丹 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 36));
	add_child(title);
	_page_nodes.push_back(title);

	AlchemySystem *al = _player ? _player->get_alchemy() : nullptr;
	if (!al) {
		add_line(LOC("（丹炉未备）"), 70.0f, 60.0f, 9, dim_c);
		return;
	}

	// 顶部：各草药持有数
	static const char *HERB_IDS[] = {
		"zhi_xue_cao", "ju_ling_cao", "bing_xin_lian", "chi_yan_hua",
		"jin_gang_teng", "wu_dao_cha", "qian_nian_ling_zhi"
	};
	Inventory *inv = _player->get_inventory();
	ItemDatabase *db = ItemDatabase::get_singleton();
	String herb_line;
	for (int i = 0; i < 7; i++) {
		const Item *def = db ? db->get_item(StringName(HERB_IDS[i])) : nullptr;
		if (i > 0) herb_line += " ";
		herb_line += (def ? LOC(def->name) : String(HERB_IDS[i])) + LOC("×") +
			String::num_int64(inv ? inv->get_item_count(StringName(HERB_IDS[i])) : 0);
	}
	add_line(herb_line, 40.0f, 58.0f, 8, head_c);

	// 配方行
	Array recipes = al->get_recipe_list();
	int sel = CLAMP(_alchemy_sel, 0, (int)recipes.size() - 1);
	for (int i = 0; i < recipes.size(); i++) {
		Dictionary r = recipes[i];
		float y = 80.0f + i * 24;
		bool locked = r["realm_locked"];
		bool can = r["can_craft"];
		bool is_sel = (i == sel);

		// 主行：选中标记 + 丹名 + 效果 + 门控标注
		String main_line = (is_sel ? LOC("> ") : LOC("  ")) + String(r["name"]) +
			LOC("  ") + String(r["effect"]);
		if (locked) main_line += LOC("  （金丹起）");
		Color mc = is_sel ? sel_c : (can ? body_c : dim_c);
		add_line(main_line, 60.0f, y, 9, mc);

		// 材料行：够=亮 / 不够=红
		String mat_line = LOC("    ");
		Array mats = r["mats"];
		for (int j = 0; j < mats.size(); j++) {
			Dictionary m = mats[j];
			if (j > 0) mat_line += " + ";
			mat_line += String(m["name"]) + "×" + String::num_int64((int)m["need"]) +
				"(" + String::num_int64((int)m["have"]) + ")";
		}
		add_line(mat_line, 60.0f, y + 11, 8, can ? dim_c : bad_c);
	}

	// 炼制结果提示
	if (!_alchemy_msg.is_empty()) {
		bool ok = _alchemy_msg.contains(LOC("炼成"));
		add_line(_alchemy_msg, 60.0f, 250.0f, 10, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("丹炉随身，随时随地可炼。炼制亦修行：每炉喂练气 +5。"), 60.0f, 250.0f, 8, dim_c);
	}
}

void GameMenu::_handle_alchemy_input() {
	AlchemySystem *al = _player ? _player->get_alchemy() : nullptr;
	if (!al) return;
	Input *input = Input::get_singleton();
	int count = al->get_recipe_list().size();
	if (input->is_action_just_pressed(LOC("up"))) {
		_alchemy_sel = (_alchemy_sel - 1 + count) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_alchemy_sel = (_alchemy_sel + 1) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		Array recipes = al->get_recipe_list();
		int sel = CLAMP(_alchemy_sel, 0, (int)recipes.size() - 1);
		if (sel >= 0) {
			StringName id = Dictionary(recipes[sel])["id"];
			al->craft(id);
			_alchemy_msg = al->get_last_message();
			_alchemy_msg_t = 2.5f;
			_rebuild_page();
		}
	}
}

void GameMenu::_build_settings_page() {
	_refresh_settings_page();
}

void GameMenu::_refresh_settings_page() {
	for (CanvasItem *n : _page_nodes) {
		if (n) n->queue_free();
	}
	_page_nodes.clear();

	static const char *ITEM_NAMES[3] = { "保存游戏", "退出游戏", "" };
	bool is_en = Localization::get_singleton() && Localization::get_singleton()->get_language() == "en";
	String lang_label = LOC("语言") + ": < " + LOC(is_en ? "English" : "中文") + " >";
	String vol = LOC("主音量") + "  < " + String::num_int64(int64_t(Math::round(_volume * 100.0f))) + "% >";
	String names[4] = { vol, lang_label, LOC(ITEM_NAMES[0]), LOC(ITEM_NAMES[1]) };

	Label *title = memnew(Label);
	title->set_text(LOC("—— 设置 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 40));
	add_child(title);
	_page_nodes.push_back(title);

	for (int i = 0; i < 4; i++) {
		Label *l = memnew(Label);
		if (_saved_flash > 0.0f && i == 2) {
			l->set_text(LOC("   ") + LOC("已存档！"));
		} else if (i == _settings_sel) {
			l->set_text(LOC("▶ ") + names[i]);
		} else {
			l->set_text(LOC("   ") + names[i]);
		}
		l->add_theme_font_size_override("font_size", 9);
		l->add_theme_color_override("font_color",
			i == _settings_sel ? Color(1.0f, 0.95f, 0.6f) : Color(0.85f, 0.85f, 0.85f));
		l->set_position(Vector2(170, 80 + i * 22));
		add_child(l);
		_page_nodes.push_back(l);
	}
}

void GameMenu::_handle_settings_input() {
	Input *input = Input::get_singleton();

	if (input->is_action_just_pressed(LOC("up"))) {
		_settings_sel = (_settings_sel + 3) % 4;
		_refresh_settings_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_settings_sel = (_settings_sel + 1) % 4;
		_refresh_settings_page();
	}
	if (_settings_sel == 0) {
		if (input->is_action_just_pressed(LOC("left"))) {
			_volume = CLAMP(_volume - 0.1f, 0.0f, 1.0f);
			_apply_volume(); _save_settings(); _refresh_settings_page();
		}
		if (input->is_action_just_pressed(LOC("right"))) {
			_volume = CLAMP(_volume + 0.1f, 0.0f, 1.0f);
			_apply_volume(); _save_settings(); _refresh_settings_page();
		}
	}
	// Row 1: language toggle
	if (_settings_sel == 1) {
		if (input->is_action_just_pressed(LOC("left")) || input->is_action_just_pressed(LOC("right")) ||
			input->is_action_just_pressed(LOC("interact"))) {
			Localization *loc = Localization::get_singleton();
			if (loc) {
				String cur = loc->get_language();
				loc->set_language(cur == "en" ? "zh" : "en");
				_refresh_settings_page();
			}
		}
		return;
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		switch (_settings_sel) {
			case 0:
				_volume = CLAMP(_volume + 0.1f, 0.0f, 1.0f);
				_apply_volume(); _save_settings(); _refresh_settings_page();
				break;
			case 2: {
				Node *gm = get_tree()->get_current_scene()->get_node_or_null(NodePath("GameManager"));
				if (gm) {
					gm->call("save_game", String("auto"));
					_saved_flash = 1.5f;
					_refresh_settings_page();
				}
				break;
			}
			case 3:
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
	if (cfg->load(LOC("user://settings.cfg")) == OK) {
		_volume = CLAMP(float(cfg->get_value(LOC("audio"), LOC("master_volume"), 0.8f)), 0.0f, 1.0f);
	}
}

void GameMenu::_save_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->set_value("audio", "master_volume", _volume);
	cfg->save("user://settings.cfg");
}

void GameMenu::_on_language_changed(const String &p_locale) {
	// 刷新页签条文字
	for (int i = 0; i < PAGE_COUNT; i++) {
		_tab_labels[i]->set_text(LOC(TAB_NAMES[i]));
	}
	// 若菜单打开，重建当前页
	if (_open) {
		_rebuild_page();
	}
}

// ============================================================
// 输入轮询
// ============================================================

void GameMenu::_process(double p_delta) {
	Input *input = Input::get_singleton();

	if (!_open) {
		if (input->is_action_just_pressed(LOC("menu")) || input->is_action_just_pressed(LOC("inventory"))) {
			// 储物面板打开时 ESC/I 归它处理（关面板），菜单不抢
			Node *scene = get_tree()->get_current_scene();
			StoragePanel *sp = scene ? Object::cast_to<StoragePanel>(scene->find_child("StoragePanel", true, false)) : nullptr;
			if (sp && sp->is_open())
				return;
			if (input->is_action_just_pressed(LOC("menu"))) {
				_open_menu(_page); // 记住上次页
			} else {
				_open_menu(PAGE_INVENTORY);
			}
		}
		return;
	}

	if (_saved_flash > 0.0f) {
		_saved_flash -= float(p_delta);
		if (_saved_flash <= 0.0f && _page == PAGE_SETTINGS) {
			_refresh_settings_page();
		}
	}

	if (_alchemy_msg_t > 0.0f) {
		_alchemy_msg_t -= float(p_delta);
		if (_alchemy_msg_t <= 0.0f && _page == PAGE_ALCHEMY) {
			_alchemy_msg = String();
			_rebuild_page();
		}
	}

	if (_skill_msg_t > 0.0f) {
		_skill_msg_t -= float(p_delta);
		if (_skill_msg_t <= 0.0f && _page == PAGE_SKILL) {
			_skill_msg = String();
			_rebuild_page();
		}
	}

	if (_sect_msg_t > 0.0f) {
		_sect_msg_t -= float(p_delta);
		if (_sect_msg_t <= 0.0f && _page == PAGE_SECT) {
			_sect_msg = String();
			_rebuild_page();
		}
	}

	if (_travel_msg_t > 0.0f) {
		_travel_msg_t -= float(p_delta);
		if (_travel_msg_t <= 0.0f && _page == PAGE_TRAVEL) {
			_travel_msg = String();
			_rebuild_page();
		}
	}

	if (input->is_action_just_pressed(LOC("menu"))) {
		_close_menu();
		return;
	}
	if (input->is_action_just_pressed(LOC("inventory"))) {
		if (_page == PAGE_INVENTORY) {
			_close_menu();
		} else {
			_switch_page(PAGE_INVENTORY);
		}
		return;
	}
	bool lr_for_settings = (_page == PAGE_SETTINGS && (_settings_sel == 0 || _settings_sel == 1));
	if (!lr_for_settings) {
		if (input->is_action_just_pressed(LOC("left"))) {
			_switch_page(_page - 1);
			return;
		}
		if (input->is_action_just_pressed(LOC("right"))) {
			_switch_page(_page + 1);
			return;
		}
	}

	switch (_page) {
		case PAGE_INVENTORY:
			if (input->is_action_just_pressed(LOC("up")))   _inv_panel->ext_navigate(-1);
			if (input->is_action_just_pressed(LOC("down"))) _inv_panel->ext_navigate(+1);
			if (input->is_action_just_pressed(LOC("interact"))) _inv_panel->ext_use();
			break;
		case PAGE_ALCHEMY:
			_handle_alchemy_input();
			break;
		case PAGE_SKILL:
			_handle_skill_input();
			break;
		case PAGE_SECT:
			_handle_sect_input();
			break;
		case PAGE_TRAVEL:
			_handle_travel_input();
			break;
		case PAGE_SETTINGS:
			_handle_settings_input();
			break;
		default:
			break;
	}
}

} // namespace godot
