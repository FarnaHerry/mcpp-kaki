module;
#include "../nodes/player.h"
#include "../core/enemy_database.h"
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
#include "../core/currency_system.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

module mcpp_kaki.nodes;
import mcpp_kaki.cultivation;
import mcpp_kaki.core;
import mcpp_kaki.combat;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;
namespace godot {

static const char *TAB_NAMES[] = { "个人信息", "背包", "能力", "功法", "技能", "法宝", "宗门", "云游", "熔炼炉", "设置", "图鉴" };

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
		t->set_position(Vector2(42 + i * 40, 12));
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
	_apply_display();

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

	// Q/E 翻页任何行都生效（设置页 ←/→ 音量/语言调节走 _process，互不干扰）。
	// 菜单打开即暂停，Q/E 在此翻页与正常游戏中的 Q/E 技能键分属两态，不冲突。
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
		case PAGE_PROFILE:
			if (_inv_panel) _inv_panel->close();
			_build_profile_page();
			_set_hint(LOC("Q/E 切换页  ESC 关闭"));
			break;
		case PAGE_INVENTORY:
			if (_inv_panel) _inv_panel->open();
			_set_hint(LOC("Q/E 切换页  ↑/↓ 选择  ←/→ 列移  ↑筛选  X 使用/装备  ESC 关闭"));
			break;
		case PAGE_ABILITY:
			if (_inv_panel) _inv_panel->close();
			_build_ability_page();
			_set_hint(LOC("Q/E 切换页  ESC 关闭"));
			break;
		case PAGE_GONGFA:
			if (_inv_panel) _inv_panel->close();
			_build_gongfa_page();
			_set_hint(LOC("Q/E 切换页  ESC 关闭"));
			break;
		case PAGE_SKILL:
			if (_inv_panel) _inv_panel->close();
			_build_skill_page();
			_set_hint(LOC("Q/E 切换页  ESC 关闭"));
			break;
		case PAGE_ARTIFACT:
			if (_inv_panel) _inv_panel->close();
			_build_artifact_page();
			_set_hint(LOC("Q/E 切换页  ↑/↓ 选法宝  X 设本命  A~H 装入对应槽  ESC 关闭"));
			break;
		case PAGE_SECT:
			if (_inv_panel) _inv_panel->close();
			_build_sect_page();
			_set_hint(LOC("Q/E 切换页  ↑/↓ 选宗  X 拜入/叛门  ESC 关闭"));
			break;
		case PAGE_TRAVEL:
			if (_inv_panel) _inv_panel->close();
			_build_travel_page();
			_set_hint(LOC("Q/E 切换页  ↑↓←→ 选洲  X 前往  ESC 关闭"));
			break;
		case PAGE_ALCHEMY:
			if (_inv_panel) _inv_panel->close();
			_build_forge_page();
			_set_hint(LOC("Q/E 切页  ↑/↓←/→ 选配方  X 炼制/铸造  ↑到顶进侧边栏"));
			break;
		case PAGE_SETTINGS:
			if (_inv_panel) _inv_panel->close();
			_build_settings_page();
			_set_hint(LOC("Q/E 切换页  ↑/↓ 选择  ←/→ 调节  X 确认  ESC 关闭"));
			break;
		case PAGE_BESTIARY:
			if (_inv_panel) _inv_panel->close();
			_build_bestiary_page();
			_set_hint(LOC("Q/E 切换页  ↑/↓←/→ 选条目  X 备注  ↑顶行/←左列进分类  ESC 关闭"));
			break;
	}
}

void GameMenu::_build_profile_page() {
	Label *title = memnew(Label);
	title->set_text(LOC("—— 个人信息 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 40));
	add_child(title);
	_page_nodes.push_back(title);

	Player *p = _player;
	CultivationSystem *cult = p ? p->get_cultivation() : nullptr;

	// 寿元：从 GameManager 的 SoulLedgerSystem 读取（簿上/实际；负值=无限）
	int ledger_ls = 100, actual_ls = 100;
	Node *scene = get_tree()->get_current_scene();
	Node *gm = scene ? scene->get_node_or_null(NodePath("GameManager")) : nullptr;
	if (gm) {
		Object *ledger = gm->call("get_soul_ledger");
		if (ledger) {
			ledger_ls = int(ledger->call("get_ledger_lifespan"));
			actual_ls = int(ledger->call("get_actual_lifespan"));
		}
	}

	auto fmt = [](double v) -> String { return String::num_int64(int64_t(Math::round(v))); };

	auto add_line = [&](const String &label, const String &value, float y) {
		Label *l = memnew(Label);
		l->set_text(label + TXT("：") + value);
		l->add_theme_font_size_override("font_size", 9);
		l->add_theme_color_override("font_color", Color(0.85f, 0.85f, 0.85f));
		l->set_position(Vector2(90, y));
		add_child(l);
		_page_nodes.push_back(l);
	};

	float y = 70.0f;
	add_line(LOC("境界"), cult ? cult->get_full_title() : LOC("凡人"), y); y += 16.0f;
	add_line(LOC("生命"), p ? fmt(p->get_max_health()) : TXT("0"), y); y += 16.0f;
	add_line(LOC("灵力"), cult ? fmt(cult->get_max_mana()) : TXT("0"), y); y += 16.0f;
	// 回复速度
	{
		double mana_regen_rate = cult ? (cult->get_max_mana() * 0.02 * cult->get_mana_regen_mult()) : 0.0;
		double mana_regen_pct = 2.0 * (cult ? cult->get_mana_regen_mult() : 1.0);
		add_line(LOC("灵力回复"), fmt(mana_regen_rate) + TXT("/s  (") + String::num(mana_regen_pct, 1) + TXT("%)"), y); y += 16.0f;
	}
	// 生命回复：目前无非打坐被动回复，显示 0，预留显示 buff/技能加成
	{
		add_line(LOC("生命回复"), TXT("0/s"), y); y += 16.0f;
	}
	add_line(LOC("攻击"), p ? fmt(p->get_effective_attack()) : TXT("0"), y); y += 16.0f;
	add_line(LOC("防御"), p ? fmt(p->get_effective_defense()) : TXT("0"), y); y += 16.0f;
	add_line(LOC("速度"), p ? fmt(p->move_speed) : TXT("0"), y); y += 16.0f;
	add_line(LOC("饱食"), (p ? fmt(p->get_fullness()) : TXT("0")) + TXT(" / ") + (p ? fmt(p->get_max_fullness()) : TXT("0")), y); y += 16.0f;
	add_line(LOC("寿元"), String::num_int64(ledger_ls) + TXT(" / ") + (actual_ls < 0 ? TXT("∞") : String::num_int64(actual_ls)), y);
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

	auto build_column = [&](const AbilityRow *rows, int count, const String &header, float x, int cols, float w) {
		Label *h = memnew(Label);
		h->set_text(header);
		h->add_theme_font_size_override("font_size", 11);
		h->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
		h->set_position(Vector2(x, 36));
		add_child(h);
		_page_nodes.push_back(h);

		Array items;
		for (int i = 0; i < count; i++) {
			bool unlocked = rows[i].innate || (am && am->has_ability(StringName(rows[i].id)));
			Dictionary cell;
			if (unlocked) {
				cell["text"] = LOC("✓") + LOC(rows[i].name);
				cell["color"] = Color(0.55f, 0.95f, 0.55f, 1.0f);
			} else {
				cell["text"] = LOC("✗") + LOC(rows[i].name) + LOC("·") + LOC(rows[i].cond);
				cell["dim"] = true;
			}
			items.push_back(cell);
		}
		GridList *grid = memnew(GridList);
		grid->set_position(Vector2(x, 54));
		grid->set_size(Vector2(w, 168));
		add_child(grid);
		grid->set_columns(cols);
		grid->set_cell_size(Vector2(w / float(cols), 21));
		grid->set_items(items);
		grid->set_active(false); // 只读总览：无选中高亮
		_page_nodes.push_back(grid);
	};

	build_column(ACTIVE_ROWS, 15, LOC("— 主动 —"), 60.0f, 2, 200.0f);
	build_column(PASSIVE_ROWS, 7, LOC("— 被动 —"), 280.0f, 1, 170.0f);

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
		v->set_text(LOC("✓ 威压 U  — 慑服低阶（耗灵30 cd8s）"));
		v->add_theme_font_size_override("font_size", 8);
		v->add_theme_color_override("font_color", Color(0.55f, 0.9f, 0.55f));
		v->set_position(Vector2(60, 238));
		add_child(v);
		_page_nodes.push_back(v);

		Label *r = memnew(Label);
		r->set_text(LOC("✓ 灵压 I  — 法伤低阶/镇杀（耗灵60 cd15s）"));
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

// 功法品级五色直接用全项目统一 grade_color（inventory.cppm）：
// 黄→0白 / 玄→1蓝 / 地→2紫 / 天→3金 / 仙→4青

void GameMenu::_build_gongfa_page() {
	Label *title = memnew(Label);
	title->set_text(LOC("—— 功法 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 36));
	add_child(title);
	_page_nodes.push_back(title);

	GongfaSystem *gf = _player ? _player->get_gongfa() : nullptr;

	auto slot_text = [&](GongfaSystem::School school, Color &r_line1_color) -> PackedStringArray {
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
		// 仙品显示：先天仙品（grade==GRADE_XIAN）或后天仙化（飞升晋升），名前缀「仙·」
		const bool xian = (def->grade == GongfaSystem::GRADE_XIAN) || gf->is_xian_promoted();
		String disp_name = (xian ? String(LOC("仙·")) : String()) + LOC(def->name);
		String gname = xian ? LOC("仙品") : GongfaSystem::grade_name(def->grade);
		r_line1_color = grade_color(xian ? 4 : (int)def->grade);
		String line1 = disp_name + LOC("  ") + gname +
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
	Color body_line1_c = body_c;
	PackedStringArray body_lines = slot_text(GongfaSystem::SCHOOL_BODY, body_line1_c);
	for (int i = 0; i < body_lines.size(); i++) {
		add_line(body_lines[i], 70.0f, 76.0f + i * 14, 9, i == 0 ? body_line1_c : body_c);
	}
	add_line(LOC("— 练气 —"), 280.0f, 60.0f, 10, head_c);
	Color qi_line1_c = body_c;
	PackedStringArray qi_lines = slot_text(GongfaSystem::SCHOOL_QI, qi_line1_c);
	for (int i = 0; i < qi_lines.size(); i++) {
		add_line(qi_lines[i], 280.0f, 76.0f + i * 14, 9, i == 0 ? qi_line1_c : body_c);
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

	// ---- 元婴分叉：肉身成圣 / 元神修炼（合体「形神合一」汇合）----
	CultivationSystem *cs = _player ? _player->get_cultivation() : nullptr;
	if (cs && cs->get_realm_index() >= CultivationSystem::NASCENT_SOUL) {
		int bl = cs->get_path_body_level();
		int sl = cs->get_path_spirit_level();
		auto path_line = [&](const String &name, int lvl, float exp) -> String {
			if (lvl >= CultivationSystem::PATH_MAX_LEVEL) {
				return name + LOC(" 第") + String::num_int64(lvl) + LOC("/5 级  圆满");
			}
			float pct = exp / CultivationSystem::PATH_EXP_PER_LEVEL * 100.0f;
			// exp 含已升整级部分，取级内进度
			pct = Math::fmod(exp, CultivationSystem::PATH_EXP_PER_LEVEL) / CultivationSystem::PATH_EXP_PER_LEVEL * 100.0f;
			return name + LOC(" 第") + String::num_int64(lvl) + LOC("/5 级  ") + String::num(pct, 0) + LOC("%");
		};
		String head = cs->is_path_merged()
			? LOC("— 形神合一（分叉已汇合，双修同步）—")
			: LOC("— 元婴分叉 —");
		add_line(head, 70.0f, 226.0f, 10, head_c);
		add_line(path_line(LOC("肉身成圣"), bl, cs->get_path_body_exp()) +
		         LOC("  物攻/防御/生命 +") + String::num(bl * 3, 0) + LOC("%"),
		         70.0f, 240.0f, 9, body_c);
		add_line(path_line(LOC("元神修炼"), sl, cs->get_path_spirit_exp()) +
		         LOC("  法强/灵力 +") + String::num(sl * 3, 0) +
		         LOC("%  法则回复 +") + String::num(sl * 5, 0) + LOC("%"),
		         280.0f, 240.0f, 9, body_c);
		add_line(LOC("近战/受击养肉身（渡劫硬抗减伤 ") + String::num(bl * 8, 0) +
		         LOC("%），施法养元神（雷预警+风稳心）。合体弱侧补 80% 汇合。"),
		         70.0f, 254.0f, 8, dim_c);
	}
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

	// 槽位总览（QWERTY 上行 0..5 / ASDFGH 下行 6..11；按类型分两行展示）
	static const char *KEYS[SkillSystem::SLOT_COUNT] = { "Q", "W", "E", "R", "T", "Y", "A", "S", "D", "F", "G", "H" };
	// 每类列出其槽位：武技 Q/W/A/S、法术 E/R/D/F、神通 T/Y/G、仙法 H
	struct TypeGroup { const char *name; int slots[4]; int count; };
	static const TypeGroup GROUPS[4] = {
		{ "武技", { 0, 1, 6, 7 }, 4 },
		{ "法术", { 2, 3, 8, 9 }, 4 },
		{ "神通", { 4, 5, 10, -1 }, 3 },
		{ "仙法", { 11, -1, -1, -1 }, 1 },
	};
	for (int g = 0; g < 4; g++) {
		add_line(LOC("— ") + LOC(GROUPS[g].name) + LOC(" —"), 40.0f + g * 115.0f, 54.0f, 10, head_c);
		for (int k = 0; k < GROUPS[g].count; k++) {
			int slot = GROUPS[g].slots[k];
			String text = String("[") + KEYS[slot] + "] ";
			if (skills) {
				Dictionary info = skills->get_slot_info(slot);
				text += info.is_empty() ? LOC("（空）") : LOC(String(info.get("name", "")));
			} else {
				text += LOC("（空）");
			}
			add_line(text, 40.0f + g * 115.0f, 69.0f + k * 12, 9, body_c);
		}
	}

	// 主动技能格子列表（↑/↓ 行移，A/S/D/F/T/Y 装配到对应槽）
	add_line(LOC("已学主动:"), 40.0f, 100.0f, 9, head_c);
	Array actives = _skill_active_knowns();
	static const int GRID_COLS = 3;
	if (actives.is_empty()) {
		add_line(LOC("（尚未习得任何技能）"), 40.0f, 116.0f, 9, dim_c);
	} else {
		_skill_sel = CLAMP(_skill_sel, 0, (int)actives.size() - 1);

		// 选中项详情并入标题行右侧
		Dictionary selk = actives[_skill_sel];
		String detail = LOC(String(selk.get("name", ""))) + LOC(" ·") + String(selk.get("type_name", "")) +
			LOC(" ×") + String::num(float(selk.get("power", 1.0f)), 1) +
			LOC(" 冷却") + String::num(float(selk.get("cooldown", 0.0f)), 1) + LOC("s");
		float mana = float(selk.get("mana_cost", 0.0f));
		if (mana > 0.0f) detail += LOC(" 灵") + String::num_int64(int64_t(mana));
		float law = float(selk.get("law_cost", 0.0f));
		if (law > 0.0f) detail += LOC(" 法则") + String::num_int64(int64_t(law));
		add_line(detail, 110.0f, 100.0f, 9, sel_c);

		// 类型色：武技白/法术蓝/神通紫/仙法金
		auto type_color = [](int t) {
			switch (t) {
				case SkillSystem::TYPE_SPELL:    return Color(0.55f, 0.75f, 1.0f, 1.0f);
				case SkillSystem::TYPE_SHENTONG: return Color(0.85f, 0.6f, 1.0f, 1.0f);
				case SkillSystem::TYPE_XIANFA:   return Color(1.0f, 0.85f, 0.4f, 1.0f);
				default:                         return Color(0.9f, 0.9f, 0.9f, 1.0f);
			}
		};
		Array items;
		for (int i = 0; i < actives.size(); i++) {
			Dictionary k = actives[i];
			Dictionary cell;
			cell["text"] = LOC(String(k.get("name", "")));
			cell["color"] = type_color(int(k.get("type", 0)));
			items.push_back(cell);
		}
		GridList *grid = memnew(GridList);
		grid->set_position(Vector2(40, 114));
		grid->set_size(Vector2(400, 52)); // 2 行窗口，选中驱动滚动
		add_child(grid);
		grid->set_columns(GRID_COLS);
		grid->set_cell_size(Vector2(133, 26));
		grid->set_items(items);
		grid->set_selected(_skill_sel);
		_page_nodes.push_back(grid);
	}

	// 被动格子列表（学会即常驻，不占槽；数值走乘区）
	add_line(LOC("已悟被动:"), 40.0f, 172.0f, 9, head_c);
	static const char *PAS_NAMES[7] = { "", "攻击", "移速", "防御", "回灵", "飞速", "法则回复" };
	if (skills) {
		Array known = skills->get_known_list();
		Array pas_items;
		for (int i = 0; i < known.size(); i++) {
			Dictionary k = known[i];
			if (int(k.get("type", -1)) != SkillSystem::TYPE_PASSIVE) continue;
			int ps = CLAMP(int(k.get("passive_stat", 0)), 0, 6);
			int pct = int(Math::round(float(k.get("passive_value", 0.0f)) * 100.0f));
			Dictionary cell;
			cell["text"] = LOC(String(k.get("name", ""))) + LOC(" ") + LOC(PAS_NAMES[ps]) +
				LOC("+") + String::num_int64(pct) + LOC("%");
			cell["color"] = Color(0.7f, 0.9f, 0.7f, 1.0f);
			pas_items.push_back(cell);
		}
		if (pas_items.is_empty()) {
			add_line(LOC("（尚未悟得被动）"), 40.0f, 188.0f, 9, dim_c);
		} else {
			GridList *pas_grid = memnew(GridList);
			pas_grid->set_position(Vector2(40, 186));
			pas_grid->set_size(Vector2(400, 52)); // 2 行 × 3 列
			add_child(pas_grid);
			pas_grid->set_columns(3);
			pas_grid->set_cell_size(Vector2(133, 26));
			pas_grid->set_items(pas_items);
			pas_grid->set_active(false); // 只读：无选中高亮
			_page_nodes.push_back(pas_grid);
		}
	}

	// 装配结果提示 / 操作说明
	if (!_skill_msg.is_empty()) {
		bool ok = _skill_msg.contains(LOC("已装配"));
		add_line(_skill_msg, 40.0f, 248.0f, 9, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓ 选择主动技，按 QWERTY/ASDFGH 装入对应槽（类型须匹配）；B 键切法宝页。"), 40.0f, 248.0f, 8, dim_c);
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
	static const int GRID_COLS = 3; // 与 _build_skill_page 一致
	if (input->is_action_just_pressed(LOC("up"))) {
		_skill_sel = Math::max(0, _skill_sel - GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_skill_sel = Math::min(count - 1, _skill_sel + GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		int row = _skill_sel / GRID_COLS;
		int col = Math::max(0, _skill_sel % GRID_COLS - 1);
		_skill_sel = Math::min(count - 1, row * GRID_COLS + col);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("right"))) {
		int row = _skill_sel / GRID_COLS;
		int col = Math::min(GRID_COLS - 1, _skill_sel % GRID_COLS + 1);
		_skill_sel = Math::min(count - 1, row * GRID_COLS + col);
		_rebuild_page();
	}
	// 按槽键装配当前选中技能（QWERTY 上行 0..5 + ASDFGH 下行 6..11，共 12 槽）
	static const char *SLOT_ACTIONS[12] = {
		"skill_q", "skill_w", "skill_e", "skill_r", "skill_t", "skill_y",
		"skill_a", "skill_s", "skill_d", "skill_f", "skill_g", "skill_h"
	};
	static const int SLOT_IDX[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	static const char *SLOT_KEYS[12] = { "Q", "W", "E", "R", "T", "Y", "A", "S", "D", "F", "G", "H" };
	for (int i = 0; i < 12; i++) {
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
// 法宝页（本命 + 次要槽总览；已拥有法宝可选中，X 设本命 / A~H 装入对应槽）
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
	Color sel_c(1.0f, 0.95f, 0.6f);
	Color ok_c(0.6f, 1.0f, 0.6f);
	Color bad_c(1.0f, 0.5f, 0.5f);
	Color benming_c(1.0f, 0.85f, 0.45f);

	ArtifactSystem *arts = _player ? _player->get_artifacts() : nullptr;
	int limit = arts ? arts->get_slot_limit() : 3;
	static const char *KEYS[ArtifactSystem::MAX_SLOTS] = { "A", "S", "D", "F", "G", "H" };

	// 槽位总览（3 列 × 2 行只读格：本命槽金色，锁定槽灰显）
	add_line(LOC("— 槽位（本命A + 次要S~H） —"), 40.0f, 54.0f, 9, head_c);
	{
		Array slot_items;
		for (int i = 0; i < ArtifactSystem::MAX_SLOTS; i++) {
			Dictionary cell;
			if (i >= limit) {
				cell["text"] = String("[") + KEYS[i] + LOC("] 飞升解锁");
				cell["dim"] = true;
			} else {
				Dictionary info = arts ? arts->get_slot_info(i) : Dictionary();
				if (info.is_empty()) {
					cell["text"] = String("[") + KEYS[i] + LOC("] （空）");
					cell["dim"] = true;
				} else {
					cell["text"] = String("[") + KEYS[i] + "] " + LOC(String(info.get("name", ""))) +
						LOC(" ×") + String::num(float(info.get("coeff", 1.0f)), 2);
					cell["color"] = i == 0 ? benming_c : body_c;
				}
			}
			slot_items.push_back(cell);
		}
		GridList *slot_grid = memnew(GridList);
		slot_grid->set_position(Vector2(40, 66));
		slot_grid->set_size(Vector2(400, 52)); // 2 行 × 3 列
		add_child(slot_grid);
		slot_grid->set_columns(3);
		slot_grid->set_cell_size(Vector2(133, 26));
		slot_grid->set_items(slot_items);
		slot_grid->set_active(false); // 只读
		_page_nodes.push_back(slot_grid);
	}

	// 已拥有法宝（可选中列表：↑/↓←/→ 移动光标，X 设本命，A~H 装入对应槽）
	add_line(LOC("已拥有法宝:"), 40.0f, 124.0f, 9, head_c);
	Array owned = arts ? arts->get_owned_list() : Array();
	static const int GRID_COLS = 3;
	if (owned.is_empty()) {
		add_line(LOC("（尚未获得任何法宝；筑基赐飞剑，残篇类物品使用即得）"), 40.0f, 140.0f, 9, dim_c);
	} else {
		_artifact_sel = CLAMP(_artifact_sel, 0, (int)owned.size() - 1);

		// 选中项详情并入标题行右侧
		Dictionary selk = owned[_artifact_sel];
		StringName sel_id = StringName(String(selk.get("id", "")));
		String detail = LOC(String(selk.get("name", ""))) + LOC(" ·") + String(selk.get("kind_name", ""));
		if (arts) {
			const ArtifactSystem::Def *sdef = ArtifactSystem::find_def(sel_id);
			if (sdef) {
				if (sdef->kind == ArtifactSystem::KIND_ATTACK) {
					detail += LOC(" 祭出×") + String::num(sdef->power, 1) +
						LOC(" 灵") + String::num_int64(int64_t(sdef->mana_cost)) +
						LOC(" 冷却") + String::num(sdef->cooldown, 1) + LOC("s");
				} else {
					if (sdef->passive_def > 0.0f)
						detail += LOC(" 防+") + String::num_int64(int64_t(sdef->passive_def * 100.0f)) + LOC("%");
					if (sdef->passive_atk > 0.0f)
						detail += LOC(" 攻+") + String::num_int64(int64_t(sdef->passive_atk * 100.0f)) + LOC("%");
					if (sdef->resist_elem_pct > 0.0f) {
						static const char *ELEM_SHORT[8] = { "", "金", "木", "水", "火", "土", "雷", "风" };
						int el = CLAMP(int(sdef->resist_elem), 0, 7);
						detail += LOC(" ") + LOC(ELEM_SHORT[el]) + LOC("抗+") +
							String::num_int64(int64_t(sdef->resist_elem_pct * 100.0f)) + LOC("%");
					}
				}
			}
		}
		add_line(detail, 130.0f, 124.0f, 9, sel_c);

		// 温养进度行（选中项）
		{
			String nurture_line = _artifact_nurture_line(arts, sel_id);
			if (!nurture_line.is_empty())
				add_line(nurture_line, 130.0f, 136.0f, 9, sel_c);
		}

		// 装配标记：本命=金「·本命」，次要槽=「·[S]」等
		Array items;
		for (int i = 0; i < owned.size(); i++) {
			Dictionary k = owned[i];
			StringName oid = StringName(String(k.get("id", "")));
			String txt = LOC(String(k.get("name", "")));
			if (arts) {
				for (int s = 0; s < limit; s++) {
					if (arts->get_slot_artifact(s) == oid) {
						txt += s == 0 ? LOC(" ·本命") : String(LOC(" ·[")) + KEYS[s] + "]";
						break;
					}
				}
			}
			Dictionary cell;
			cell["text"] = txt;
			cell["color"] = int(k.get("kind", 0)) == int(ArtifactSystem::KIND_ATTACK)
				? Color(0.9f, 0.9f, 0.9f, 1.0f) : Color(0.7f, 0.9f, 0.7f, 1.0f);
			items.push_back(cell);
		}
		GridList *grid = memnew(GridList);
		grid->set_position(Vector2(40, 138));
		grid->set_size(Vector2(400, 52)); // 2 行窗口，选中驱动滚动
		add_child(grid);
		grid->set_columns(GRID_COLS);
		grid->set_cell_size(Vector2(133, 26));
		grid->set_items(items);
		grid->set_selected(_artifact_sel);
		_page_nodes.push_back(grid);
	}

	// 装配结果提示 / 操作说明
	if (!_artifact_msg.is_empty()) {
		bool ok = _artifact_msg.contains(LOC("已设")) || _artifact_msg.contains(LOC("已装配"));
		add_line(_artifact_msg, 40.0f, 206.0f, 9, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓←/→ 选法宝，X 设本命法宝，A/S/D/F/G/H 装入对应槽。"), 40.0f, 206.0f, 8, dim_c);
	}
	add_line(LOC("战斗中按 B 整页切换法宝页，A~H 即法宝快捷键；祭出复用技能管线，耗灵力。"), 40.0f, 232.0f, 8, dim_c);
	add_line(LOC("本命温养 120%→150%，渡劫觉醒 200% 并锁定；次要 100%→120%→150%。"), 40.0f, 246.0f, 8, dim_c);
}

// 温养进度行文案：各族法宝显示温养% + 当前倍率 + 距下一档所需值（本命另显觉醒状态）。
// 幅面固定（催熟/祭出等温养值变化只换文本不换行数，避免每帧重建抖动）。
String GameMenu::_artifact_nurture_line(ArtifactSystem *p_arts, const StringName &p_id) {
	if (!p_arts) return String();
	Dictionary prog = p_arts->get_nurture_progress(p_id);
	int stage = int(prog.get("stage", 0));
	bool benming = bool(prog.get("is_benming", false));
	bool awakened = bool(prog.get("awakened", false));
	float nurture = float(prog.get("nurture", 0.0f));
	float coeff = float(prog.get("coeff", 1.0f));
	float next = float(prog.get("next_need", 0.0f));

	String line = LOC("温养 ") + String::num(nurture, 0) + LOC(" ·×") + String::num(coeff, 2);
	if (benming && awakened) {
		line += LOC("  ｜ 已觉醒 ×2.00 锁定");
	} else if (stage >= 2) {
		line += LOC("  ｜ 已圆满");
	} else if (stage == 1) {
		line += benming
			? LOC("  ｜ 圆满待渡劫觉醒 ×2.00")
			: LOC("  ｜ 圆满 ×1.50");
	} else {
		line += LOC("  ｜ 距 ×1.20 还差 ") + String::num(next, 0);
	}
	return line;
}

void GameMenu::_handle_artifact_input() {
	ArtifactSystem *arts = _player ? _player->get_artifacts() : nullptr;
	if (!arts) return;
	Input *input = Input::get_singleton();
	Array owned = arts->get_owned_list();
	int count = owned.size();
	if (count == 0) return;
	_artifact_sel = CLAMP(_artifact_sel, 0, count - 1);
	static const int GRID_COLS = 3; // 与 _build_artifact_page 一致
	if (input->is_action_just_pressed(LOC("up"))) {
		_artifact_sel = Math::max(0, _artifact_sel - GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_artifact_sel = Math::min(count - 1, _artifact_sel + GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		int row = _artifact_sel / GRID_COLS;
		int col = Math::max(0, _artifact_sel % GRID_COLS - 1);
		_artifact_sel = Math::min(count - 1, row * GRID_COLS + col);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("right"))) {
		int row = _artifact_sel / GRID_COLS;
		int col = Math::min(GRID_COLS - 1, _artifact_sel % GRID_COLS + 1);
		_artifact_sel = Math::min(count - 1, row * GRID_COLS + col);
		_rebuild_page();
	}

	Dictionary k = owned[_artifact_sel];
	StringName id = StringName(String(k.get("id", "")));
	String kname = LOC(String(k.get("name", "")));
	int limit = arts->get_slot_limit();
	static const char *KEYS[ArtifactSystem::MAX_SLOTS] = { "A", "S", "D", "F", "G", "H" };

	// X：设本命法宝（觉醒锁定后拒绝；换本命会重置温养）
	if (input->is_action_just_pressed(LOC("interact"))) {
		StringName cur = arts->get_slot_artifact(0);
		if (cur == id) {
			_artifact_msg = kname + LOC(" 已是本命法宝");
		} else if (_player && _player->is_benming_awakened()) {
			_artifact_msg = LOC("本命已锁定（渡劫觉醒后不可更换）");
		} else if (arts->equip(0, id) && arts->get_slot_artifact(0) == id) {
			_artifact_msg = LOC("已设本命法宝 ") + kname;
		} else {
			_artifact_msg = LOC("本命设定失败");
		}
		_artifact_msg_t = 2.5f;
		_rebuild_page();
		return;
	}

	// A~H：装入对应法宝槽（A=本命槽同 X 规则；次要槽越限提示飞升解锁）
	static const char *SLOT_ACTIONS[6] = { "skill_a", "skill_s", "skill_d", "skill_f", "skill_g", "skill_h" };
	for (int i = 0; i < 6; i++) {
		if (!input->is_action_just_pressed(LOC(SLOT_ACTIONS[i]))) continue;
		if (i >= limit) {
			_artifact_msg = LOC("[") + KEYS[i] + LOC("] 槽飞升后解锁");
		} else if (i == 0) {
			StringName cur = arts->get_slot_artifact(0);
			if (cur == id) {
				_artifact_msg = kname + LOC(" 已是本命法宝");
			} else if (_player && _player->is_benming_awakened()) {
				_artifact_msg = LOC("本命已锁定（渡劫觉醒后不可更换）");
			} else if (arts->equip(0, id) && arts->get_slot_artifact(0) == id) {
				_artifact_msg = LOC("已设本命法宝 ") + kname;
			} else {
				_artifact_msg = LOC("本命设定失败");
			}
		} else if (arts->equip(i, id)) {
			_artifact_msg = LOC("已装配 [") + KEYS[i] + LOC("] ") + kname;
		} else {
			_artifact_msg = LOC("装配失败");
		}
		_artifact_msg_t = 2.5f;
		_rebuild_page();
		break;
	}
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
// 云游图页（design/world-map.md 四）：可视化世界地图——四大部洲按地理方位落位 +
// 云海航线点线 + 天界浮空；▲选中/【当前】/未解锁灰显 + 门槛随节点展示，底部详情栏。
// 文本约定（test_travel.gd 断言依赖）：标题「—— 云游图 ——」；节点名行含 ▶ 前缀（选中）/
// 【当前】/未解锁 字样；锁定节点附属行以「条件：」开头；除节点名外任何文案不得含
// 「未解锁」（页内计数 == 锁定洲数）。
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
	// 近似宽度（CJK 全角≈字号，ASCII≈0.55×）——仅用于居中摆放
	auto text_w = [](const String &s, int size) -> float {
		float w = 0;
		for (int i = 0; i < s.length(); i++) {
			uint32_t ch = (uint32_t)s[i];
			bool wide = ch >= 0x2E80 || ch == 0x25B6;
			w += wide ? size : size * 0.55f;
		}
		return w;
	};
	auto add_rect = [&](float x, float y, float w, float h, const Color &c) {
		ColorRect *r = memnew(ColorRect);
		r->set_position(Vector2(x, y));
		r->set_size(Vector2(w, h));
		r->set_color(c);
		add_child(r);
		_page_nodes.push_back(r);
	};
	// 有机岛屿形色块（固定抖动表，10 边形；size = 外接矩形）
	auto add_blob = [&](Vector2 center, Vector2 size, const Color &c) {
		Polygon2D *p = memnew(Polygon2D);
		static const float jit[10] = { 1.0f, 0.84f, 0.96f, 0.8f, 0.98f, 0.86f, 0.92f, 0.8f, 1.0f, 0.88f };
		PackedVector2Array pts;
		for (int i = 0; i < 10; i++) {
			float a = Math_TAU * i / 10.0f;
			pts.push_back(Vector2(Math::cos(a) * size.x * 0.5f * jit[i],
					Math::sin(a) * size.y * 0.5f * jit[(i + 5) % 10]));
		}
		p->set_polygon(pts);
		p->set_color(c);
		p->set_position(center);
		add_child(p);
		_page_nodes.push_back(p);
	};
	// 云海航线：两节点间铺点（离节点 26px 内不铺，留出洲岸）
	auto add_route = [&](Vector2 a, Vector2 b, const Color &c) {
		Vector2 ab = b - a;
		float len = ab.length();
		if (len < 1.0f) return;
		Vector2 dir = ab / len;
		int n = (int)(len / 14.0f);
		for (int i = 1; i < n; i++) {
			Vector2 p = a + dir * (i * 14.0f);
			if ((p - a).length() < 26.0f || (p - b).length() < 26.0f) continue;
			add_rect(p.x - 2, p.y - 2, 4, 4, c);
		}
	};

	Color sel_c(1.0f, 0.92f, 0.5f);
	Color cur_c(0.55f, 0.95f, 0.55f);
	Color body_c(0.88f, 0.88f, 0.88f);
	Color dim_c(0.5f, 0.5f, 0.5f);
	Color lock_c(0.72f, 0.72f, 0.8f);
	Color ok_c(0.6f, 1.0f, 0.6f);
	Color bad_c(1.0f, 0.5f, 0.5f);
	Color route_c(0.78f, 0.87f, 1.0f, 0.4f);

	Label *title = memnew(Label);
	title->set_text(LOC("—— 云游图 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(185, 26));
	add_child(title);
	_page_nodes.push_back(title);

	if (!_continent_mgr) {
		add_line(LOC("（云路未通）"), 70.0f, 60.0f, 9, dim_c);
		return;
	}

	// ---- 洲节点版式（地理方位：东胜居东、西牛居西、南赡居南、北俱居北，天界浮空九霄）----
	struct NodeLayout {
		const char *id;
		Vector2 pos;
		Vector2 size;
		Color base;
	};
	static const NodeLayout LAYOUTS[] = {
		{ "tianjie",   Vector2(240, 64),  Vector2(66, 26),  Color(0.95f, 0.84f, 0.45f) }, // 天界·金
		{ "beijulu",   Vector2(330, 102), Vector2(96, 34),  Color(0.55f, 0.78f, 0.95f) }, // 北俱芦洲·冰蓝
		{ "xiniuhe",   Vector2(104, 148), Vector2(96, 34),  Color(0.95f, 0.62f, 0.45f) }, // 西牛贺洲·赤土
		{ "dongsheng", Vector2(376, 148), Vector2(96, 34),  Color(0.55f, 0.85f, 0.6f) },  // 东胜神洲·青
		{ "nanzhanbu", Vector2(240, 178), Vector2(96, 34),  Color(0.9f, 0.85f, 0.55f) },  // 南赡部洲·黄土
	};

	// ---- 底图：东海云海 + 天界云带 ----
	add_rect(16, 44, 448, 172, Color(0.1f, 0.17f, 0.3f, 0.45f));
	add_rect(48, 82, 384, 10, Color(1, 1, 1, 0.05f));
	add_rect(90, 88, 300, 7, Color(1, 1, 1, 0.05f));

	// ---- 云海航线（四洲环渡）+ 北俱南天门登天路 ----
	add_route(Vector2(376, 148), Vector2(330, 102), route_c);
	add_route(Vector2(330, 102), Vector2(104, 148), route_c);
	add_route(Vector2(104, 148), Vector2(240, 178), route_c);
	add_route(Vector2(240, 178), Vector2(376, 148), route_c);
	add_route(Vector2(330, 102), Vector2(240, 64), Color(0.8f, 0.8f, 0.85f, 0.3f)); // 登天虚线

	Array list = _continent_mgr->get_continent_list();
	_travel_sel = CLAMP(_travel_sel, 0, (int)list.size() - 1);

	// 两遍绘制：先岛体（描边环→岛影→岛体），后全部文字（门槛/洲名不被后画岛体压住）
	struct NodeDraw {
		String name_txt, gate_txt;
		Color name_c;
		Vector2 pos, size;
		bool locked;
	};
	std::vector<NodeDraw> draws;
	for (int i = 0; i < list.size(); i++) {
		Dictionary c = list[i];
		String id = c["id"];
		String name = c["name"];
		bool unlocked = c["unlocked"];
		bool current = c["current"];
		bool is_sel = (i == _travel_sel);

		// 版式查表；未收录 id 兜底横排（防未来新增洲越界）
		Vector2 pos(80 + i * 80, 120);
		Vector2 size(80, 30);
		Color base(0.6f, 0.7f, 0.7f);
		for (const NodeLayout &nl : LAYOUTS) {
			if (id == nl.id) {
				pos = nl.pos;
				size = nl.size;
				base = nl.base;
				break;
			}
		}
		Color fill = unlocked ? base : base.lerp(Color(0.32f, 0.32f, 0.36f), 0.75f);

		// 选中/当前描边环（大 blob 垫底成边框）：选中金在最外，当前绿在其内
		if (current)
			add_blob(pos, size + Vector2(13, 11), Color(cur_c.r, cur_c.g, cur_c.b, 0.9f));
		if (is_sel)
			add_blob(pos, size + Vector2(26, 21), Color(sel_c.r, sel_c.g, sel_c.b, 0.9f));
		// 岛影 + 岛体
		add_blob(pos + Vector2(0, 3), size, Color(0, 0, 0, 0.25f));
		add_blob(pos, size, fill);

		NodeDraw nd;
		nd.name_txt = (is_sel ? LOC("▶ ") : String()) + name;
		if (current) nd.name_txt += LOC("【当前】");
		if (!unlocked) nd.name_txt += LOC(" 未解锁");
		nd.name_c = is_sel ? sel_c : (current ? cur_c : (unlocked ? body_c : lock_c));
		nd.gate_txt = unlocked ? String() : LOC("条件：") + String(c["gate"]);
		nd.pos = pos;
		nd.size = size;
		nd.locked = !unlocked;
		draws.push_back(nd);
	}
	for (const NodeDraw &nd : draws) {
		float nw = text_w(nd.name_txt, 9);
		// 洲名暗底垫：与岛体/描边环分隔，保证灰字锁定态可读
		add_rect(nd.pos.x - nw * 0.5f - 3, nd.pos.y - 8, nw + 6, 15, Color(0, 0, 0, 0.45f));
		add_line(nd.name_txt, nd.pos.x - nw * 0.5f, nd.pos.y - 6, 9, nd.name_c);
		// 锁定洲：岛下门槛行（页内「条件：」唯一来源）
		if (nd.locked) {
			float gw = text_w(nd.gate_txt, 7);
			add_line(nd.gate_txt, nd.pos.x - gw * 0.5f, nd.pos.y + nd.size.y * 0.5f + 2, 7, Color(0.75f, 0.75f, 0.82f));
		}
	}

	// ---- 底部详情栏（当前选中洲）----
	Dictionary sel = list[_travel_sel];
	add_line(String(sel["name"]), 70.0f, 216.0f, 10, sel_c);
	add_line(String(sel["desc"]), 70.0f, 231.0f, 8, body_c);
	String state;
	if (bool(sel["current"])) state = LOC("当前所在洲");
	else if (bool(sel["unlocked"])) state = LOC("已解锁 · X 前往");
	else state = LOC("境界门槛：") + String(sel["gate"]);
	add_line(state, 280.0f, 231.0f, 8, bool(sel["current"]) ? cur_c : (bool(sel["unlocked"]) ? ok_c : lock_c));
	add_line(LOC("↑↓←→ 选择  X 前往"), 350.0f, 216.0f, 8, dim_c);

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
	if (input->is_action_just_pressed(LOC("up")) || input->is_action_just_pressed(LOC("left"))) {
		_travel_sel = (_travel_sel - 1 + count) % count;
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down")) || input->is_action_just_pressed(LOC("right"))) {
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
// 熔炼炉页（design：左侧侧边栏 4 子页 + 右侧主区域）
// 子页 0=炼丹 1=装备铸造 2=法宝铸造 3=装备强化
// ============================================================

void GameMenu::_build_forge_page() {
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

	// 标题
	Label *title = memnew(Label);
	title->set_text(LOC("—— 熔炼炉 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(185, 36));
	add_child(title);
	_page_nodes.push_back(title);

	// 左侧侧边栏（4 子页标签竖排，同背包筛选行：非焦点仅高亮色，焦点时选中项方括号括起）
	static const char *SUB_NAMES[4] = { "炼丹", "装备铸造", "法宝铸造", "装备强化" };
	for (int i = 0; i < 4; i++) {
		bool is_sel = (i == _forge_sub);
		Label *sl = memnew(Label);
		if (_forge_sidebar_focus) {
			sl->set_text(is_sel ? LOC("[") + LOC(SUB_NAMES[i]) + LOC("]") : LOC(SUB_NAMES[i]));
		} else {
			sl->set_text(LOC(SUB_NAMES[i]));
		}
		sl->add_theme_color_override("font_color", is_sel ? sel_c : dim_c);
		sl->add_theme_font_size_override("font_size", 9);
		sl->set_position(Vector2(12, 60 + i * 22));
		add_child(sl);
		_page_nodes.push_back(sl);
	}

	// 右侧主区域
	switch (_forge_sub) {
		case 0: _build_forge_alchemy(); break;
		case 1: _build_forge_equip(); break;
		case 2: _build_forge_artifact(); break;
		case 3: _build_forge_upgrade(); break;
	}
}

// 子页 0：炼丹（调用 AlchemySystem 既有 API，与旧 _build_alchemy_page 一致）
void GameMenu::_build_forge_alchemy() {
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

	AlchemySystem *al = _player ? _player->get_alchemy() : nullptr;
	if (!al) {
		add_line(LOC("（丹炉未备）"), 80.0f, 60.0f, 9, dim_c);
		return;
	}

	// 顶部草药持有数
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
	add_line(herb_line, 80.0f, 58.0f, 8, head_c);

	// 丹方卡片（GridList，3 列）
	Array recipes = al->get_recipe_list();
	static const int GRID_COLS = 3;
	_forge_sel = CLAMP(_forge_sel, 0, (int)recipes.size() - 1);
	Array items;
	for (int i = 0; i < recipes.size(); i++) {
		Dictionary r = recipes[i];
		Dictionary cell;
		cell["text"] = String(r["name"]);
		bool locked = bool(r["realm_locked"]);
		bool can = bool(r["can_craft"]);
		if (locked) {
			cell["dim"] = true;
			cell["color"] = Color(0.5f, 0.5f, 0.5f, 1.0f);
		} else if (can) {
			cell["color"] = ok_c;
		} else {
			cell["color"] = bad_c;
		}
		items.push_back(cell);
	}
	GridList *grid = memnew(GridList);
	grid->set_position(Vector2(80, 78));
	grid->set_size(Vector2(360, 84)); // 3 行窗口
	add_child(grid);
	grid->set_columns(GRID_COLS);
	grid->set_cell_size(Vector2(120, 28));
	grid->set_items(items);
	grid->set_selected(_forge_sel);
	_page_nodes.push_back(grid);

	// 选中丹方详情
	Dictionary selr = recipes[_forge_sel];
	String detail = LOC(String(selr["name"])) + LOC("  ") + LOC(String(selr["effect"]));
	if (bool(selr["realm_locked"])) detail += LOC("  （金丹起）");
	add_line(detail, 80.0f, 170.0f, 9, sel_c);
	String mat_line = LOC("材料 ");
	Array mats = selr["mats"];
	for (int j = 0; j < mats.size(); j++) {
		Dictionary m = mats[j];
		if (j > 0) mat_line += LOC(" + ");
		mat_line += String(m["name"]) + LOC("×") + String::num_int64((int)m["need"]) +
			LOC("(") + String::num_int64((int)m["have"]) + LOC(")");
	}
	add_line(mat_line, 80.0f, 184.0f, 8, bool(selr["can_craft"]) ? dim_c : bad_c);

	// 炼制结果提示
	if (!_forge_msg.is_empty()) {
		bool ok = _forge_msg.contains(LOC("炼成"));
		add_line(_forge_msg, 80.0f, 250.0f, 10, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓←/→ 选方  X 炼制。炼制亦修行：每炉喂练气 +5。↑顶行/←左列进侧边栏"), 80.0f, 250.0f, 8, dim_c);
	}
}

// 子页 1：装备铸造（硬编码配方 5 条，扣材料给装备）
void GameMenu::_build_forge_equip() {
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

	// 硬编码铸造配方：{result_id, result_name, mat1_id, mat1_name, mat1_qty, mat2_id, mat2_name, mat2_qty}
	struct ForgeRecipe {
		const char *result_id, *result_name;
		const char *mat1_id, *mat1_name; int mat1_qty;
		const char *mat2_id, *mat2_name; int mat2_qty;
	};
	static const ForgeRecipe RECIPES[] = {
		{ "iron_sword", "铁剑", "zhi_xue_cao", "止血草", 3, "ju_ling_cao", "聚灵草", 2 },
		{ "protect_robe", "护体法衣", "bing_xin_lian", "冰心莲", 2, "chi_yan_hua", "赤焰花", 2 },
		{ "qing_feng_gu_jian", "青锋古剑", "long_gu", "龙骨", 2, "xuan_bing_sui", "玄冰髓", 1 },
		{ "bi_shui_zhu", "避水珠", "xuan_bing_shen", "玄冰参", 3, "long_gu", "龙骨", 2 },
		{ "she_li_zi", "舍利子", "jin_gang_teng", "金刚藤", 3, "xuan_bing_sui", "玄冰髓", 1 },
	};
	static const int RECIPE_COUNT = 5;

	Inventory *inv = _player ? _player->get_inventory() : nullptr;
	ItemDatabase *db = ItemDatabase::get_singleton();

	_forge_sel = CLAMP(_forge_sel, 0, RECIPE_COUNT - 1);

	// 铸造列表
	add_line(LOC("— 装备铸造 —"), 80.0f, 56.0f, 10, head_c);
	for (int i = 0; i < RECIPE_COUNT; i++) {
		bool is_sel = (i == _forge_sel);
		int have1 = inv ? inv->get_item_count(StringName(RECIPES[i].mat1_id)) : 0;
		int have2 = inv ? inv->get_item_count(StringName(RECIPES[i].mat2_id)) : 0;
		bool enough = have1 >= RECIPES[i].mat1_qty && have2 >= RECIPES[i].mat2_qty;
		float y = 76.0f + i * 22;
		String prefix = is_sel ? LOC("▶ ") : LOC("  ");
		add_line(prefix + LOC(RECIPES[i].result_name), 80.0f, y, 9, is_sel ? sel_c : (enough ? body_c : dim_c));
		String mat = LOC(RECIPES[i].mat1_name) + LOC("×") + String::num_int64(RECIPES[i].mat1_qty) +
			LOC("(") + String::num_int64(have1) + LOC(") + ") +
			LOC(RECIPES[i].mat2_name) + LOC("×") + String::num_int64(RECIPES[i].mat2_qty) +
			LOC("(") + String::num_int64(have2) + LOC(")");
		add_line(mat, 100.0f, y + 10, 8, is_sel ? (enough ? ok_c : bad_c) : dim_c);
	}

	// 结果提示
	if (!_forge_msg.is_empty()) {
		bool ok = _forge_msg.contains(LOC("铸造"));
		add_line(_forge_msg, 80.0f, 250.0f, 10, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓ 选配方  X 铸造（材料够绿色，不够红色）  ↑顶行/←左列进侧边栏"), 80.0f, 250.0f, 8, dim_c);
	}
}

// 子页 2：法宝铸造（硬编码配方 2~3 条，扣材料→ArtifactSystem::acquire）
void GameMenu::_build_forge_artifact() {
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

	struct ForgeRecipe {
		const char *result_id, *result_name;
		const char *mat1_id, *mat1_name; int mat1_qty;
		const char *mat2_id, *mat2_name; int mat2_qty;
	};
	static const ForgeRecipe RECIPES[] = {
		{ "fei_jian", "飞剑", "iron_sword", "铁剑", 1, "zhi_xue_cao", "止血草", 5 },
		{ "zhao_yao_hu", "照妖葫", "long_gu", "龙骨", 2, "xuan_bing_sui", "玄冰髓", 2 },
		{ "xuan_tie_ta", "玄铁塔", "xuan_bing_shen", "玄冰参", 3, "jin_gang_teng", "金刚藤", 3 },
	};
	static const int RECIPE_COUNT = 3;

	Inventory *inv = _player ? _player->get_inventory() : nullptr;
	ArtifactSystem *arts = _player ? _player->get_artifacts() : nullptr;

	_forge_sel = CLAMP(_forge_sel, 0, RECIPE_COUNT - 1);

	add_line(LOC("— 法宝铸造 —"), 80.0f, 56.0f, 10, head_c);
	for (int i = 0; i < RECIPE_COUNT; i++) {
		bool is_sel = (i == _forge_sel);
		int have1 = inv ? inv->get_item_count(StringName(RECIPES[i].mat1_id)) : 0;
		int have2 = inv ? inv->get_item_count(StringName(RECIPES[i].mat2_id)) : 0;
		bool enough = have1 >= RECIPES[i].mat1_qty && have2 >= RECIPES[i].mat2_qty;
		bool already_owned = arts && arts->is_owned(StringName(RECIPES[i].result_id));
		float y = 76.0f + i * 22;
		String prefix = is_sel ? LOC("▶ ") : LOC("  ");
		String name = LOC(RECIPES[i].result_name);
		if (already_owned) name += LOC("（已拥有）");
		add_line(prefix + name, 80.0f, y, 9, is_sel ? sel_c : (already_owned ? dim_c : (enough ? ok_c : bad_c)));
		String mat = LOC(RECIPES[i].mat1_name) + LOC("×") + String::num_int64(RECIPES[i].mat1_qty) +
			LOC("(") + String::num_int64(have1) + LOC(") + ") +
			LOC(RECIPES[i].mat2_name) + LOC("×") + String::num_int64(RECIPES[i].mat2_qty) +
			LOC("(") + String::num_int64(have2) + LOC(")");
		add_line(mat, 100.0f, y + 10, 8, is_sel ? (enough ? ok_c : bad_c) : dim_c);
	}

	if (!_forge_msg.is_empty()) {
		bool ok = _forge_msg.contains(LOC("铸造")) || _forge_msg.contains(LOC("习得"));
		add_line(_forge_msg, 80.0f, 250.0f, 10, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓ 选配方  X 铸造法宝（材料够绿色，不够红色）  ↑顶行/←左列进侧边栏"), 80.0f, 250.0f, 8, dim_c);
	}
}

// 子页 3：装备强化（选中装备，消耗材料提升 attack_bonus/defense_bonus，上限 +10）
void GameMenu::_build_forge_upgrade() {
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

	add_line(LOC("— 装备强化 —"), 80.0f, 56.0f, 10, head_c);

	Inventory *inv = _player ? _player->get_inventory() : nullptr;
	ItemDatabase *db = ItemDatabase::get_singleton();

	// 收集所有背包中的装备（type=3）
	struct EquipEntry {
		StringName id;
		String name;
		int qty;
		int extra_atk;
		int extra_def;
	};
	std::vector<EquipEntry> equips;
	if (inv && db) {
		for (int i = 0; i < inv->get_capacity(); i++) {
			Dictionary slot = inv->get_slot(i);
			if (slot.is_empty()) continue;
			StringName sid = StringName(String(slot["id"]));
			const Item *def = db->get_item(sid);
			if (!def || def->type != Item::EQUIPMENT) continue;
			// 去重：如果已有同 id 条目，加数量
			bool found = false;
			for (auto &e : equips) {
				if (e.id == sid) { e.qty += int(slot["quantity"]); found = true; break; }
			}
			if (!found) {
				EquipEntry ee;
				ee.id = sid;
				ee.name = LOC(def->name);
				ee.qty = int(slot["quantity"]);
				ee.extra_atk = inv->get_item_extra_atk(sid);
				ee.extra_def = inv->get_item_extra_def(sid);
				equips.push_back(ee);
			}
		}
		// 也检查已装备的
		for (int i = 0; i < 3; i++) {
			StringName eid = _player->get_equipment_in_slot(i);
			if (eid.is_empty()) continue;
			const Item *def = db->get_item(eid);
			if (!def) continue;
			bool found = false;
			for (auto &e : equips) {
				if (e.id == eid) { found = true; break; }
			}
			if (!found) {
				EquipEntry ee;
				ee.id = eid;
				ee.name = LOC(def->name);
				ee.qty = 1;
				ee.extra_atk = inv ? inv->get_item_extra_atk(eid) : 0;
				ee.extra_def = inv ? inv->get_item_extra_def(eid) : 0;
				equips.push_back(ee);
			}
		}
	}

	int count = (int)equips.size();
	if (count == 0) {
		add_line(LOC("（背包中无装备）"), 80.0f, 76.0f, 9, dim_c);
		return;
	}

	_forge_sel = CLAMP(_forge_sel, 0, count - 1);

	// 检查强化材料：5 中品灵石 + 1 玄冰髓
	CurrencySystem *cs = CurrencySystem::get_singleton();
	bool can_upgrade = false;
	if (cs && inv) {
		int mid_cost = 5 * CurrencySystem::tier_value(CurrencySystem::TIER_MID); // 5 中品 = 50 下品
		can_upgrade = cs->can_afford(mid_cost) && inv->get_item_count(StringName("xuan_bing_sui")) >= 1;
	}

	for (int i = 0; i < count; i++) {
		bool is_sel = (i == _forge_sel);
		float y = 76.0f + i * 20;
		String prefix = is_sel ? LOC("▶ ") : LOC("  ");
		int bonus = equips[i].extra_atk + equips[i].extra_def;
		bool capped = bonus >= 10;
		String line = prefix + equips[i].name + LOC(" ×") + String::num_int64(equips[i].qty);
		String bonus_str;
		if (equips[i].extra_atk > 0) bonus_str += LOC(" 攻+") + String::num_int64(equips[i].extra_atk);
		if (equips[i].extra_def > 0) bonus_str += LOC(" 防+") + String::num_int64(equips[i].extra_def);
		if (bonus_str.is_empty()) bonus_str = LOC(" 未强化");
		add_line(line + bonus_str, 80.0f, y, 9, is_sel ? sel_c : body_c);
	}

	// 详情行
	add_line(LOC("选中: ") + equips[_forge_sel].name + LOC("  强化 ") +
		String::num_int64(equips[_forge_sel].extra_atk + equips[_forge_sel].extra_def) + LOC("/10"),
		80.0f, 180.0f, 9, sel_c);
	add_line(LOC("消耗: 5 中品灵石 + 1 玄冰髓  → 攻/防 +1（上限 +10）"),
		80.0f, 194.0f, 8, dim_c);

	if (!_forge_msg.is_empty()) {
		bool ok = _forge_msg.contains(LOC("强化"));
		add_line(_forge_msg, 80.0f, 250.0f, 10, ok ? ok_c : bad_c);
	} else {
		add_line(LOC("↑/↓ 选装备  X 强化（材料够绿色，不够红色）  ↑顶行/←左列进侧边栏"), 80.0f, 250.0f, 8, dim_c);
	}
}

void GameMenu::_handle_forge_input() {
	Input *input = Input::get_singleton();
	static const int SUB_COUNT = 4;

	if (_forge_sidebar_focus) {
		// 侧边栏焦点模式：↑/↓ 切子页，X/↓ 确认返回内容
		if (input->is_action_just_pressed(LOC("up"))) {
			_forge_sub = (_forge_sub - 1 + SUB_COUNT) % SUB_COUNT;
			_forge_sel = 0;
			_forge_msg = String();
			_rebuild_page();
			return;
		}
		if (input->is_action_just_pressed(LOC("down"))) {
			_forge_sub = (_forge_sub + 1) % SUB_COUNT;
			_forge_sel = 0;
			_forge_msg = String();
			_rebuild_page();
			return;
		}
		if (input->is_action_just_pressed(LOC("interact"))) {
			// X 确认 → 返回内容区
			_forge_sidebar_focus = false;
			_forge_sel = 0;
			_forge_msg = String();
			_rebuild_page();
			return;
		}
		return;
	}

	// 内容区模式：各子页处理 ↑/↓←/→ 导航内容，↑ 顶行进侧边栏
	switch (_forge_sub) {
		case 0: _handle_forge_alchemy_input(); break;
		case 1: _handle_forge_equip_input(); break;
		case 2: _handle_forge_artifact_input(); break;
		case 3: _handle_forge_upgrade_input(); break;
	}
}

void GameMenu::_handle_forge_alchemy_input() {
	AlchemySystem *al = _player ? _player->get_alchemy() : nullptr;
	if (!al) return;
	Input *input = Input::get_singleton();
	Array recipes = al->get_recipe_list();
	int count = recipes.size();
	if (count == 0) return;
	_forge_sel = CLAMP(_forge_sel, 0, count - 1);
	static const int GRID_COLS = 3;
	if (input->is_action_just_pressed(LOC("up"))) {
		// 顶行 ↑ 进侧边栏子页选择
		if (_forge_sel < GRID_COLS) {
			_forge_sidebar_focus = true;
			_rebuild_page();
			return;
		}
		_forge_sel = Math::max(0, _forge_sel - GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_forge_sel = Math::min(count - 1, _forge_sel + GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		if (_forge_sel % GRID_COLS > 0) {
			_forge_sel = Math::max(0, _forge_sel - 1);
			_rebuild_page();
		} else {
			// 最左列 ← 进侧边栏子页选择
			_forge_sidebar_focus = true;
			_rebuild_page();
		}
	}
	if (input->is_action_just_pressed(LOC("right"))) {
		if (_forge_sel % GRID_COLS < GRID_COLS - 1) {
			_forge_sel = Math::min(count - 1, _forge_sel + 1);
			_rebuild_page();
		}
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		int sel = CLAMP(_forge_sel, 0, (int)recipes.size() - 1);
		if (sel >= 0) {
			StringName id = Dictionary(recipes[sel])["id"];
			al->craft(id);
			_forge_msg = al->get_last_message();
			_forge_msg_t = 2.5f;
			_rebuild_page();
		}
	}
}

void GameMenu::_handle_forge_equip_input() {
	Input *input = Input::get_singleton();
	struct ForgeRecipe {
		const char *result_id, *result_name;
		const char *mat1_id, *mat1_name; int mat1_qty;
		const char *mat2_id, *mat2_name; int mat2_qty;
	};
	static const ForgeRecipe RECIPES[] = {
		{ "iron_sword", "铁剑", "zhi_xue_cao", "止血草", 3, "ju_ling_cao", "聚灵草", 2 },
		{ "protect_robe", "护体法衣", "bing_xin_lian", "冰心莲", 2, "chi_yan_hua", "赤焰花", 2 },
		{ "qing_feng_gu_jian", "青锋古剑", "long_gu", "龙骨", 2, "xuan_bing_sui", "玄冰髓", 1 },
		{ "bi_shui_zhu", "避水珠", "xuan_bing_shen", "玄冰参", 3, "long_gu", "龙骨", 2 },
		{ "she_li_zi", "舍利子", "jin_gang_teng", "金刚藤", 3, "xuan_bing_sui", "玄冰髓", 1 },
	};
	static const int RECIPE_COUNT = 5;
	_forge_sel = CLAMP(_forge_sel, 0, RECIPE_COUNT - 1);
	if (input->is_action_just_pressed(LOC("up"))) {
		// 首行 ↑ 进侧边栏子页选择
		if (_forge_sel == 0) {
			_forge_sidebar_focus = true;
			_rebuild_page();
			return;
		}
		_forge_sel = Math::max(0, _forge_sel - 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_forge_sel = Math::min(RECIPE_COUNT - 1, _forge_sel + 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		if (_forge_sel == 0) {
			_forge_sidebar_focus = true;
			_rebuild_page();
			return;
		}
		_forge_sel = Math::max(0, _forge_sel - 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("right"))) {
		_forge_sel = Math::min(RECIPE_COUNT - 1, _forge_sel + 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		Inventory *inv = _player ? _player->get_inventory() : nullptr;
		if (!inv) return;
		int idx = _forge_sel;
		int have1 = inv->get_item_count(StringName(RECIPES[idx].mat1_id));
		int have2 = inv->get_item_count(StringName(RECIPES[idx].mat2_id));
		if (have1 < RECIPES[idx].mat1_qty || have2 < RECIPES[idx].mat2_qty) {
			_forge_msg = LOC("材料不足，无法铸造");
			_forge_msg_t = 2.5f;
			_rebuild_page();
			return;
		}
		inv->remove_item(StringName(RECIPES[idx].mat1_id), RECIPES[idx].mat1_qty);
		inv->remove_item(StringName(RECIPES[idx].mat2_id), RECIPES[idx].mat2_qty);
		inv->add_item(StringName(RECIPES[idx].result_id), 1);
		_forge_msg = LOC("铸造成功！获得 ") + LOC(RECIPES[idx].result_name);
		_forge_msg_t = 2.5f;
		_rebuild_page();
	}
}

void GameMenu::_handle_forge_artifact_input() {
	Input *input = Input::get_singleton();
	struct ForgeRecipe {
		const char *result_id, *result_name;
		const char *mat1_id, *mat1_name; int mat1_qty;
		const char *mat2_id, *mat2_name; int mat2_qty;
	};
	static const ForgeRecipe RECIPES[] = {
		{ "fei_jian", "飞剑", "iron_sword", "铁剑", 1, "zhi_xue_cao", "止血草", 5 },
		{ "zhao_yao_hu", "照妖葫", "long_gu", "龙骨", 2, "xuan_bing_sui", "玄冰髓", 2 },
		{ "xuan_tie_ta", "玄铁塔", "xuan_bing_shen", "玄冰参", 3, "jin_gang_teng", "金刚藤", 3 },
	};
	static const int RECIPE_COUNT = 3;
	_forge_sel = CLAMP(_forge_sel, 0, RECIPE_COUNT - 1);
	if (input->is_action_just_pressed(LOC("up"))) {
		// 首行 ↑ 进侧边栏子页选择
		if (_forge_sel == 0) {
			_forge_sidebar_focus = true;
			_rebuild_page();
			return;
		}
		_forge_sel = Math::max(0, _forge_sel - 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_forge_sel = Math::min(RECIPE_COUNT - 1, _forge_sel + 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		if (_forge_sel == 0) {
			_forge_sidebar_focus = true;
			_rebuild_page();
			return;
		}
		_forge_sel = Math::max(0, _forge_sel - 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("right"))) {
		_forge_sel = Math::min(RECIPE_COUNT - 1, _forge_sel + 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		Inventory *inv = _player ? _player->get_inventory() : nullptr;
		ArtifactSystem *arts = _player ? _player->get_artifacts() : nullptr;
		if (!inv || !arts) return;
		int idx = _forge_sel;
		StringName res_id = StringName(RECIPES[idx].result_id);
		if (arts->is_owned(res_id)) {
			_forge_msg = LOC("已拥有该法宝");
			_forge_msg_t = 2.5f;
			_rebuild_page();
			return;
		}
		int have1 = inv->get_item_count(StringName(RECIPES[idx].mat1_id));
		int have2 = inv->get_item_count(StringName(RECIPES[idx].mat2_id));
		if (have1 < RECIPES[idx].mat1_qty || have2 < RECIPES[idx].mat2_qty) {
			_forge_msg = LOC("材料不足，无法铸造法宝");
			_forge_msg_t = 2.5f;
			_rebuild_page();
			return;
		}
		inv->remove_item(StringName(RECIPES[idx].mat1_id), RECIPES[idx].mat1_qty);
		inv->remove_item(StringName(RECIPES[idx].mat2_id), RECIPES[idx].mat2_qty);
		arts->acquire(res_id);
		_forge_msg = LOC("铸造成功！习得法宝 ") + LOC(RECIPES[idx].result_name);
		_forge_msg_t = 2.5f;
		_rebuild_page();
	}
}

void GameMenu::_handle_forge_upgrade_input() {
	Input *input = Input::get_singleton();
	// 先收集背包+已装备列表
	Inventory *inv = _player ? _player->get_inventory() : nullptr;
	ItemDatabase *db = ItemDatabase::get_singleton();
	struct EquipEntry {
		StringName id;
		String name;
		int qty;
		int extra_atk;
		int extra_def;
	};
	std::vector<EquipEntry> equips;
	if (inv && db) {
		for (int i = 0; i < inv->get_capacity(); i++) {
			Dictionary slot = inv->get_slot(i);
			if (slot.is_empty()) continue;
			StringName sid = StringName(String(slot["id"]));
			const Item *def = db->get_item(sid);
			if (!def || def->type != Item::EQUIPMENT) continue;
			bool found = false;
			for (auto &e : equips) { if (e.id == sid) { e.qty += int(slot["quantity"]); found = true; break; } }
			if (!found) {
				EquipEntry ee;
				ee.id = sid; ee.name = LOC(def->name); ee.qty = int(slot["quantity"]);
				ee.extra_atk = inv->get_item_extra_atk(sid); ee.extra_def = inv->get_item_extra_def(sid);
				equips.push_back(ee);
			}
		}
		for (int i = 0; i < 3; i++) {
			StringName eid = _player->get_equipment_in_slot(i);
			if (eid.is_empty()) continue;
			const Item *def = db->get_item(eid);
			if (!def) continue;
			bool found = false;
			for (auto &e : equips) { if (e.id == eid) { found = true; break; } }
			if (!found) {
				EquipEntry ee;
				ee.id = eid; ee.name = LOC(def->name); ee.qty = 1;
				ee.extra_atk = inv ? inv->get_item_extra_atk(eid) : 0;
				ee.extra_def = inv ? inv->get_item_extra_def(eid) : 0;
				equips.push_back(ee);
			}
		}
	}
	int count = (int)equips.size();
	if (count == 0) return;
	_forge_sel = CLAMP(_forge_sel, 0, count - 1);

	if (input->is_action_just_pressed(LOC("up"))) {
		if (_forge_sel == 0) {
			_forge_sidebar_focus = true;
			_rebuild_page();
			return;
		}
		_forge_sel = Math::max(0, _forge_sel - 1);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_forge_sel = Math::min(count - 1, _forge_sel + 1);
		_rebuild_page();
	}

	if (input->is_action_just_pressed(LOC("interact"))) {
		StringName eid = equips[_forge_sel].id;
		int bonus = equips[_forge_sel].extra_atk + equips[_forge_sel].extra_def;
		if (bonus >= 10) {
			_forge_msg = LOC("已达强化上限 +10");
			_forge_msg_t = 2.5f;
			_rebuild_page();
			return;
		}
		CurrencySystem *cs = CurrencySystem::get_singleton();
		if (!cs || !inv) return;
		int mid_cost = 5 * CurrencySystem::tier_value(CurrencySystem::TIER_MID);
		if (!cs->can_afford(mid_cost) || inv->get_item_count(StringName("xuan_bing_sui")) < 1) {
			_forge_msg = LOC("材料不足：需要 5 中品灵石 + 1 玄冰髓");
			_forge_msg_t = 2.5f;
			_rebuild_page();
			return;
		}
		cs->spend(mid_cost);
		inv->remove_item(StringName("xuan_bing_sui"), 1);
		inv->upgrade_item(eid, 1, 1);
		_forge_msg = LOC("强化成功！") + equips[_forge_sel].name + LOC(" 攻防+1");
		_forge_msg_t = 2.5f;
		_rebuild_page();
	}
}

// ============================================================
// 图鉴页（Bestiary）：ESC 第 11 页（设置页之后），←/→ 切分类（物品/敌人/装备），
// ↑/↓ 选条目，X 循环备注标记（无 → ★重要 → 待收集 → 已收集 → 无），详情行显示名称/描述/来源。
// 备注保存在 Player._notes Dictionary，持久化到存档 pd["bestiary"]["notes"]。
// ============================================================

static const char *BESTIARY_CAT_NAMES[3] = { "物品", "敌人", "装备" };

void GameMenu::_bestiary_entry_detail(const String &p_id, int p_cat, String &r_name, String &r_desc) {
	// 从 ItemDatabase 或 EnemyDatabase 读取定义；无定义则 fallback
	if (p_cat == 1) {
		// 敌人
		const EnemyDef *edef = EnemyDatabase::get_def(p_id);
		if (edef) {
			r_name = LOC(edef->name.utf8().get_data());
			r_desc = LOC("击杀可获取修为与掉落物。");
			return;
		}
	}
	// 物品（含装备）
	const Item *item = ItemDatabase::get_singleton()->get_item(StringName(p_id));
	if (item) {
		r_name = LOC(item->name.utf8().get_data());
		r_desc = LOC(item->description.utf8().get_data());
		return;
	}
	// Fallback
	r_name = p_id;
	r_desc = LOC("未知条目");
}

// 备注循环档（X 循环切换；index 0 = 无备注）
static const char *BESTIARY_NOTE_CYCLE[4] = { "", "★重要", "待收集", "已收集" };

// 备注档中文名 → 序号（未命中返回 -1）
static int _bestiary_note_index(const String &p_note) {
	for (int i = 0; i < 4; i++) {
		if (LOC(BESTIARY_NOTE_CYCLE[i]) == p_note) {
			return i;
		}
	}
	return -1;
}

void GameMenu::_build_bestiary_page() {
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
	Color note_c(0.6f, 1.0f, 0.6f);

	Label *title = memnew(Label);
	title->set_text(LOC("—— 图鉴 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 36));
	add_child(title);
	_page_nodes.push_back(title);

	Player *p = _player;
	if (!p) {
		add_line(LOC("（玩家未就绪）"), 70.0f, 60.0f, 9, dim_c);
		return;
	}

	// 分类标签行：物品 | 敌人 | 装备（同背包筛选行：非焦点仅高亮色，焦点时选中项方括号括起）
	{
		float x = 60.0f;
		for (int i = 0; i < 3; i++) {
			String cat_text;
			if (_bestiary_cat_focus) {
				cat_text = i == _bestiary_cat ? LOC("[") + LOC(BESTIARY_CAT_NAMES[i]) + LOC("]") : LOC(BESTIARY_CAT_NAMES[i]);
			} else {
				cat_text = LOC(BESTIARY_CAT_NAMES[i]);
			}
			Color c = i == _bestiary_cat ? sel_c : dim_c;
			add_line(cat_text, x, 56.0f, 10, c);
			x += 80.0f;
		}
	}

	// 获取当前分类的已见过 id 列表
	Array seen_ids;
	switch (_bestiary_cat) {
		case 0: seen_ids = p->get_seen_items(); break;
		case 1: seen_ids = p->get_seen_enemies(); break;
		case 2: seen_ids = p->get_seen_equipment(); break;
	}
	_bestiary_sel = CLAMP(_bestiary_sel, 0, MAX(0, (int)seen_ids.size() - 1));

	if (seen_ids.is_empty()) {
		add_line(LOC("（尚未收录任何条目，拾取物品/击杀敌人后自动记录）"), 70.0f, 80.0f, 9, dim_c);
		return;
	}

	// 图鉴条目 GridList（3 列卡片）
	static const int GRID_COLS = 3;
	Array items;
	for (int i = 0; i < seen_ids.size(); i++) {
		String id = seen_ids[i];
		String name, desc;
		_bestiary_entry_detail(id, _bestiary_cat, name, desc);
		Dictionary cell;
		cell["text"] = name;
		// 有备注标记的条目加前缀色
		String note = p->get_note(id);
		if (!note.is_empty()) {
			cell["color"] = note_c;
		} else {
			cell["color"] = body_c;
		}
		items.push_back(cell);
	}
	GridList *grid = memnew(GridList);
	grid->set_position(Vector2(40, 76));
	grid->set_size(Vector2(400, 84)); // 3 行窗口
	add_child(grid);
	grid->set_columns(GRID_COLS);
	grid->set_cell_size(Vector2(133, 28));
	grid->set_items(items);
	grid->set_selected(_bestiary_sel);
	_page_nodes.push_back(grid);

	// 选中条目详情行
	{
		String id = seen_ids[_bestiary_sel];
		String name, desc;
		_bestiary_entry_detail(id, _bestiary_cat, name, desc);
		String detail = name + LOC("  ") + desc;
		add_line(detail, 40.0f, 170.0f, 9, sel_c);
	}

	// 备注行（选中条目）
	{
		String id = seen_ids[_bestiary_sel];
		String note = p->get_note(id);
		String note_line = LOC("备注: ");
		if (note.is_empty()) {
			note_line += LOC("（无，X 循环标记）");
		} else {
			note_line += note;
		}
		add_line(note_line, 40.0f, 184.0f, 8, dim_c);
	}

	// 提示信息
	if (!_bestiary_msg.is_empty()) {
		add_line(_bestiary_msg, 60.0f, 250.0f, 9, note_c);
	} else {
		add_line(LOC("↑/↓←/→ 选条目  X 备注  ←最左列/↑顶行 进分类  ESC 关闭"), 60.0f, 250.0f, 8, dim_c);
	}
}

void GameMenu::_handle_bestiary_input() {
	Player *p = _player;
	if (!p) return;
	Input *input = Input::get_singleton();

	if (_bestiary_cat_focus) {
		// 分类焦点模式：←/→ 切分类，↓/X 返回内容
		if (input->is_action_just_pressed(LOC("left"))) {
			_bestiary_cat = (_bestiary_cat - 1 + 3) % 3;
			_bestiary_sel = 0;
			_rebuild_page();
			return;
		}
		if (input->is_action_just_pressed(LOC("right"))) {
			_bestiary_cat = (_bestiary_cat + 1) % 3;
			_bestiary_sel = 0;
			_rebuild_page();
			return;
		}
		if (input->is_action_just_pressed(LOC("down"))) {
			_bestiary_cat_focus = false;
			_bestiary_sel = 0;
			_rebuild_page();
			return;
		}
		if (input->is_action_just_pressed(LOC("interact"))) {
			_bestiary_cat_focus = false;
			_bestiary_sel = 0;
			_rebuild_page();
			return;
		}
		return;
	}

	// 获取当前分类的已见过 id 列表
	Array seen_ids;
	switch (_bestiary_cat) {
		case 0: seen_ids = p->get_seen_items(); break;
		case 1: seen_ids = p->get_seen_enemies(); break;
		case 2: seen_ids = p->get_seen_equipment(); break;
	}
	// 内容区 ↑ 顶行 → 进分类焦点模式
	if (input->is_action_just_pressed(LOC("up"))) {
		if (_bestiary_sel < 3 || seen_ids.is_empty()) {
			_bestiary_cat_focus = true;
			_rebuild_page();
			return;
		}
	}
	// ← 最左列 → 进分类焦点模式（空分类也能进）
	if (input->is_action_just_pressed(LOC("left"))) {
		if (_bestiary_sel % 3 == 0 || seen_ids.is_empty()) {
			_bestiary_cat_focus = true;
			_rebuild_page();
			return;
		}
		// 不是最左列，正常左移
		if (_bestiary_sel % 3 > 0) {
			_bestiary_sel = Math::max(0, _bestiary_sel - 1);
			_rebuild_page();
			return;
		}
	}

	// 空分类下其余交互无意义，提前返回
	int count = seen_ids.size();
	if (count == 0) return;
	_bestiary_sel = CLAMP(_bestiary_sel, 0, count - 1);

	static const int GRID_COLS = 3;
	if (input->is_action_just_pressed(LOC("up"))) {
		_bestiary_sel = Math::max(0, _bestiary_sel - GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_bestiary_sel = Math::min(count - 1, _bestiary_sel + GRID_COLS);
		_rebuild_page();
	}
	if (input->is_action_just_pressed(LOC("left"))) {
		if (_bestiary_sel % GRID_COLS > 0) {
			_bestiary_sel = Math::max(0, _bestiary_sel - 1);
			_rebuild_page();
		}
	}
	if (input->is_action_just_pressed(LOC("right"))) {
		if (_bestiary_sel % GRID_COLS < GRID_COLS - 1) {
			_bestiary_sel = Math::min(count - 1, _bestiary_sel + 1);
			_rebuild_page();
		}
	}

	// X：循环备注标记 无 → ★重要 → 待收集 → 已收集 → 无
	if (input->is_action_just_pressed(LOC("interact"))) {
		String id = seen_ids[_bestiary_sel];
		String cur = p->get_note(id);
		int idx = _bestiary_note_index(cur);
		if (idx < 0) idx = 0;
		int next_idx = (idx + 1) % 4;
		String nv = LOC(BESTIARY_NOTE_CYCLE[next_idx]);
		p->set_note(id, nv);
		_bestiary_msg = LOC("标记: ") + nv;
		_bestiary_msg_t = 2.0f;
		_rebuild_page();
	}
}

void GameMenu::_build_settings_page() {
	_refresh_settings_page();
}

// 设置页行序：0主音量 1语言 2窗口模式 3分辨率 4帧率 5垂直同步 6保存 7退出
static const int SETTINGS_ROWS = 8;
// 窗口模式 3 档：窗口 / 无边框全屏 / 独占全屏（Godot 4 WINDOW_MODE_FULLSCREEN 即无边框全屏）
static const char *WMODE_NAMES[3] = { "窗口", "无边框全屏", "独占全屏" };
// 渲染比例固定 16:9（480×270）基准——stretch aspect=keep：非 16:9 比例（16:10/3:2/4:3/21:9）
// 由引擎居中黑边（letterbox）兼容，像素游戏标准（Celeste/HK 式）。不自动匹配比例、不延伸视野。
// 分辨率档（原生分辨率，常规游戏语义，全窗口模式可调）：
// 窗口/无边框全屏=窗口尺寸、独占全屏=显示模式（影响渲染精度）。
static const int FS_RES_PRESETS[][2] = {
	{ 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }, { 3120, 2080 }, { 3840, 2160 },
};
static const int FS_RES_COUNT = 6;
// 垂直同步 2 档：关/开（Godot 原生支持 window_set_vsync_mode）
static const char *VSYNC_NAMES[2] = { "关", "开" };
// 帧率上限档动态生成（_build_fps_options）：按系统最高刷新率（screen_get_refresh_rate）裁剪，
// 常见档 + 系统上限 + 末尾 0（无限不锁帧）。无静态 FPS_PRESETS。

void GameMenu::_refresh_settings_page() {
	for (CanvasItem *n : _page_nodes) {
		if (n) n->queue_free();
	}
	_page_nodes.clear();

	bool is_en = Localization::get_singleton() && Localization::get_singleton()->get_language() == "en";
	String lang_label = LOC("语言") + ": < " + LOC(is_en ? "English" : "中文") + " >";
	String vol = LOC("主音量") + "  < " + String::num_int64(int64_t(Math::round(_volume * 100.0f))) + "% >";
	String wmode = LOC("窗口模式") + "  < " + LOC(WMODE_NAMES[_window_mode_opt]) + " >";
	// 分辨率（原生分辨率，常规游戏语义，全窗口模式可调）：
	// 窗口/无边框全屏=窗口尺寸、独占全屏=显示模式（影响渲染精度）。
	String res = LOC("分辨率") + LOC("  < ") +
		String::num_int64(FS_RES_PRESETS[_fs_res_idx][0]) + LOC("×") + String::num_int64(FS_RES_PRESETS[_fs_res_idx][1]) + LOC(" >");
	String fps = LOC("帧率") + LOC("  < ") + (_max_fps == 0 ? LOC("无限") : String::num_int64(int64_t(_max_fps))) + LOC(" >");
	String vsync = LOC("垂直同步") + LOC("  < ") + LOC(VSYNC_NAMES[_vsync]) + LOC(" >");
	String names[SETTINGS_ROWS] = { vol, lang_label, wmode, res, fps, vsync, LOC("保存游戏"), LOC("退出游戏") };

	Label *title = memnew(Label);
	title->set_text(LOC("—— 设置 ——"));
	title->add_theme_font_size_override("font_size", 13);
	title->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.5f));
	title->set_position(Vector2(195, 34));
	add_child(title);
	_page_nodes.push_back(title);

	for (int i = 0; i < SETTINGS_ROWS; i++) {
		Label *l = memnew(Label);
		if (_saved_flash > 0.0f && i == 6) {
			l->set_text(LOC("   ") + LOC("已存档！"));
		} else if (i == _settings_sel) {
			l->set_text(LOC("▶ ") + names[i]);
		} else {
			l->set_text(LOC("   ") + names[i]);
		}
		l->add_theme_font_size_override("font_size", 9);
		Color c = i == _settings_sel ? Color(1.0f, 0.95f, 0.6f) : Color(0.85f, 0.85f, 0.85f);
		l->add_theme_color_override("font_color", c);
		l->set_position(Vector2(150, 66 + i * 24));
		add_child(l);
		_page_nodes.push_back(l);
	}

	// 分辨率行说明（全窗口模式可调）
	if (_settings_sel == 3) {
		Label *hint = memnew(Label);
		hint->set_text(LOC("←/→ 选择分辨率（独占全屏=显示模式；窗口/无边框全屏=窗口尺寸）"));
		hint->add_theme_font_size_override("font_size", 8);
		hint->add_theme_color_override("font_color", Color(0.55f, 0.6f, 0.7f));
		hint->set_position(Vector2(150, 66 + SETTINGS_ROWS * 24));
		add_child(hint);
		_page_nodes.push_back(hint);
	}
	// 帧率行说明
	if (_settings_sel == 4) {
		Label *hint = memnew(Label);
		hint->set_text(LOC("←/→ 选择帧率上限（按系统最高刷新率，无限=不锁帧）"));
		hint->add_theme_font_size_override("font_size", 8);
		hint->add_theme_color_override("font_color", Color(0.55f, 0.6f, 0.7f));
		hint->set_position(Vector2(150, 66 + SETTINGS_ROWS * 24));
		add_child(hint);
		_page_nodes.push_back(hint);
	}
	// 垂直同步行说明
	if (_settings_sel == 5) {
		Label *hint = memnew(Label);
		hint->set_text(LOC("←/→ 切换垂直同步（锁定显示器刷新率，消除画面撕裂）"));
		hint->add_theme_font_size_override("font_size", 8);
		hint->add_theme_color_override("font_color", Color(0.55f, 0.6f, 0.7f));
		hint->set_position(Vector2(150, 66 + SETTINGS_ROWS * 24));
		add_child(hint);
		_page_nodes.push_back(hint);
	}
}

void GameMenu::_handle_settings_input() {
	Input *input = Input::get_singleton();

	if (input->is_action_just_pressed(LOC("up"))) {
		_settings_sel = (_settings_sel + SETTINGS_ROWS - 1) % SETTINGS_ROWS;
		_refresh_settings_page();
	}
	if (input->is_action_just_pressed(LOC("down"))) {
		_settings_sel = (_settings_sel + 1) % SETTINGS_ROWS;
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
	// Row 2: window mode cycle
	if (_settings_sel == 2) {
		if (input->is_action_just_pressed(LOC("left")) || input->is_action_just_pressed(LOC("right"))) {
			int dir = input->is_action_just_pressed(LOC("left")) ? -1 : 1;
			_window_mode_opt = (_window_mode_opt + dir + 3) % 3;
			_apply_display(); _save_settings(); _refresh_settings_page();
		}
		return;
	}

	// Row 3: 分辨率（原生分辨率，全窗口模式可调）
	if (_settings_sel == 3) {
		if (input->is_action_just_pressed(LOC("left")) || input->is_action_just_pressed(LOC("right"))) {
			int dir = input->is_action_just_pressed(LOC("left")) ? -1 : 1;
			_fs_res_idx = (_fs_res_idx + dir + FS_RES_COUNT) % FS_RES_COUNT;
			_apply_display(); _save_settings(); _refresh_settings_page();
		}
		return;
	}

	// Row 4: 帧率上限（动态档：≤ 系统刷新率 + 无限）
	if (_settings_sel == 4) {
		if (input->is_action_just_pressed(LOC("left")) || input->is_action_just_pressed(LOC("right"))) {
			int dir = input->is_action_just_pressed(LOC("left")) ? -1 : 1;
			int n = (int)_fps_opts.size();
			if (n > 0) {
				int idx = 0;
				for (int i = 0; i < n; i++) if (_fps_opts[i] == _max_fps) idx = i;
				idx = (idx + dir + n) % n;
				_max_fps = _fps_opts[idx];
			}
			_apply_fps(); _save_settings(); _refresh_settings_page();
		}
		return;
	}

	// Row 5: 垂直同步（关/开）
	if (_settings_sel == 5) {
		if (input->is_action_just_pressed(LOC("left")) || input->is_action_just_pressed(LOC("right"))) {
			_vsync = (_vsync + 1) % 2;
			_apply_vsync(); _save_settings(); _refresh_settings_page();
		}
		return;
	}
	if (input->is_action_just_pressed(LOC("interact"))) {
		switch (_settings_sel) {
			case 0:
				_volume = CLAMP(_volume + 0.1f, 0.0f, 1.0f);
				_apply_volume(); _save_settings(); _refresh_settings_page();
				break;
			case 6: {
				Node *gm = get_tree()->get_current_scene()->get_node_or_null(NodePath("GameManager"));
				if (gm) {
					gm->call("save_game", String("auto"));
					_saved_flash = 1.5f;
					_refresh_settings_page();
				}
				break;
			}
			case 7:
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

void GameMenu::_apply_fps() {
	Engine::get_singleton()->set_max_fps(_max_fps);
}

void GameMenu::_apply_vsync() {
	DisplayServer *ds = DisplayServer::get_singleton();
	if (!ds) return;
	ds->window_set_vsync_mode(_vsync == 0 ? DisplayServer::VSYNC_DISABLED : DisplayServer::VSYNC_ENABLED);
}

// 帧率上限档按系统最高刷新率动态生成：常见档（60 起，≤ 刷新率）+ 系统上限 + 末尾 0（无限）。
void GameMenu::_build_fps_options() {
	_fps_opts.clear();
	int R = 60;
	DisplayServer *ds = DisplayServer::get_singleton();
	if (ds) {
		float rr = ds->screen_get_refresh_rate(0);
		if (rr > 1.0f) R = int(Math::round(rr));
	}
	// 默认 = 系统最高刷新率（未设置时）
	if (_max_fps < 0) _max_fps = R;
	static const int LADDER[] = { 60, 120, 144, 240, 360 };
	for (int f : LADDER) if (f <= R) _fps_opts.push_back(f);
	if (_fps_opts.empty() || _fps_opts.back() != R) _fps_opts.push_back(R);
	_fps_opts.push_back(0); // 无限（不锁帧）
	// 换显示器后旧上限可能不在档内 → 钳到系统上限
	if (_max_fps > 0) {
		bool found = false;
		for (int f : _fps_opts) if (f == _max_fps) found = true;
		if (!found) _max_fps = R;
	}
}

void GameMenu::_apply_display() {
	DisplayServer *ds = DisplayServer::get_singleton();
	if (!ds) return;
	// 原生分辨率档全窗口模式生效：窗口/无边框全屏=窗口尺寸（用户此后仍可自由拉伸）、
	// 独占全屏=显示模式（影响渲染精度）
	Vector2i r(FS_RES_PRESETS[_fs_res_idx][0], FS_RES_PRESETS[_fs_res_idx][1]);
	switch (_window_mode_opt) {
		case 0: // 窗口
			ds->window_set_mode(DisplayServer::WINDOW_MODE_WINDOWED);
			ds->window_set_flag(DisplayServer::WINDOW_FLAG_BORDERLESS, false);
			ds->window_set_size(r);
			break;
		case 1: // 无边框全屏（Godot 4 FULLSCREEN 即无边框全屏）
			ds->window_set_mode(DisplayServer::WINDOW_MODE_FULLSCREEN);
			ds->window_set_size(r);
			break;
		case 2: // 独占全屏：window size = 显示模式（原生分辨率，影响渲染精度）
			ds->window_set_mode(DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN);
			ds->window_set_size(r);
			break;
	}
	Window *wnd = get_tree() ? get_tree()->get_root() : nullptr;
	if (wnd) wnd->set_size(r);
	// 内部渲染比例固定 16:9（480×270），非 16:9 由 aspect=keep 居中黑边
	_apply_render_scale();
}

// 内部渲染分辨率固定 16:9（480×270）基准——与窗口大小完全解耦：
// 窗口只决定显示区域与分数放大倍率；非 16:9 比例由 project.godot aspect=keep 居中黑边。
void GameMenu::_apply_render_scale() {
	Window *wnd = get_tree() ? get_tree()->get_root() : nullptr;
	if (!wnd) return;
	wnd->set_content_scale_size(Vector2i(480, 270));
	// 缩放固定分数倍（fractional）——窗口/全屏填满无黑边（窗口尺寸=所选分辨率一致）；
	// 非 16:9 比例（3:2/4:3/16:10/21:9）仍由 aspect=keep 居中黑边兼容。
	wnd->set_content_scale_stretch(Window::CONTENT_SCALE_STRETCH_FRACTIONAL);
}


void GameMenu::_load_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	if (cfg->load(LOC("user://settings.cfg")) == OK) {
		_volume = CLAMP(float(cfg->get_value(LOC("audio"), LOC("master_volume"), 0.8f)), 0.0f, 1.0f);
		// 窗口模式档位迁移（旧 4 档→新 3 档）：0窗口 1无边框窗口→无边框全屏 2全屏→无边框全屏 3独占→独占
		int wm = int(cfg->get_value(LOC("display"), LOC("window_mode"), 0));
		_window_mode_opt = wm <= 0 ? 0 : (wm >= 3 ? 2 : 1);
		_fs_res_idx = CLAMP(int(cfg->get_value(LOC("display"), LOC("res_idx"), 2)), 0, FS_RES_COUNT - 1);
		_max_fps = CLAMP(int(cfg->get_value(LOC("display"), LOC("max_fps"), -1)), -1, 1000);
		_vsync = CLAMP(int(cfg->get_value(LOC("display"), LOC("vsync"), 1)), 0, 1);
		// 旧档 resolution_idx/resolution_custom/custom_w/custom_h/scale_mode/aspect_idx/fps_idx
		// 不再读取（存档重写时随新 ConfigFile 自动消失）
	}
}

void GameMenu::_save_settings() {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->set_value("audio", "master_volume", _volume);
	cfg->set_value("display", "window_mode", _window_mode_opt);
	cfg->set_value("display", "res_idx", _fs_res_idx);
	cfg->set_value("display", "max_fps", _max_fps);
	cfg->set_value("display", "vsync", _vsync);
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

	// 启动首帧（主循环窗口就绪后）应用显示设置——_ready 阶段窗口可能未完全创建，
	// window_set_size/mode 会被引擎初始化覆盖，导致启动时分辨率/窗口模式不生效
	if (!_startup_applied) {
		_startup_applied = true;
		_build_fps_options();
		_apply_display();
		_apply_fps();
		_apply_vsync();
	}
	if (!_open) {
		if (input->is_action_just_pressed(LOC("menu"))) {
			// 储物面板打开时 ESC 归它处理（关面板），菜单不抢
			Node *scene = get_tree()->get_current_scene();
			StoragePanel *sp = scene ? Object::cast_to<StoragePanel>(scene->find_child("StoragePanel", true, false)) : nullptr;
			if (sp && sp->is_open())
				return;
			// 丹房面板（洞天 GDScript 面板）打开时 ESC 同样归它处理
			Node *pp = scene ? scene->find_child("PillLabPanel", true, false) : nullptr;
			if (pp && pp->has_method("is_open") && bool(pp->call("is_open")))
				return;
			_open_menu(_page); // 记住上次页
			return;
		}
		return;
	}

	if (_saved_flash > 0.0f) {
		_saved_flash -= float(p_delta);
		if (_saved_flash <= 0.0f && _page == PAGE_SETTINGS) {
			_refresh_settings_page();
		}
	}

	if (_forge_msg_t > 0.0f) {
			_forge_msg_t -= float(p_delta);
			if (_forge_msg_t <= 0.0f && _page == PAGE_ALCHEMY) {
				_forge_msg = String();
				_rebuild_page();
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

	if (_artifact_msg_t > 0.0f) {
		_artifact_msg_t -= float(p_delta);
		if (_artifact_msg_t <= 0.0f && _page == PAGE_ARTIFACT) {
			_artifact_msg = String();
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

	if (_bestiary_msg_t > 0.0f) {
		_bestiary_msg_t -= float(p_delta);
		if (_bestiary_msg_t <= 0.0f && _page == PAGE_BESTIARY) {
			_bestiary_msg = String();
			_rebuild_page();
		}
	}

	if (input->is_action_just_pressed(LOC("menu"))) {
		_close_menu();
		return;
	}
	// 翻页严格只用 Q/E（_input 处理）；←/→ 保留给各页内部横向导航，不被顶部拦截

	switch (_page) {
		case PAGE_INVENTORY:
			if (input->is_action_just_pressed(LOC("up")))    _inv_panel->ext_navigate(-1);
			if (input->is_action_just_pressed(LOC("down")))  _inv_panel->ext_navigate(+1);
			if (input->is_action_just_pressed(LOC("left")))  _inv_panel->ext_navigate_h(-1);
			if (input->is_action_just_pressed(LOC("right"))) _inv_panel->ext_navigate_h(+1);
			if (input->is_action_just_pressed(LOC("interact"))) _inv_panel->ext_use();
			break;
		case PAGE_ALCHEMY:
			_handle_forge_input();
			break;
		case PAGE_SKILL:
			_handle_skill_input();
			break;
		case PAGE_ARTIFACT:
			_handle_artifact_input();
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
		case PAGE_BESTIARY:
			_handle_bestiary_input();
			break;
		default:
			break;
	}
}

} // namespace godot
