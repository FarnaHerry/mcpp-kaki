module;
#include "../utils/text.h"
#include "../nodes/player.h"
#include "../nodes/enemy.h"
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
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


#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

module mcpp_kaki.nodes;
import mcpp_kaki.combat;
import mcpp_kaki.core;
import mcpp_kaki.cultivation;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;
namespace godot {

// 寿元显示：负值 = 无限（天尊，跳出五行）→ "∞"（查簿 overlay 仍用）
static String _lifespan_txt(int p_lifespan) {
    if (p_lifespan < 0)
        return TXT("∞");
    return String::num_int64(p_lifespan);
}

// Layout constants (480×270 viewport)
static constexpr float BAR_WIDTH = 140.0f;
static constexpr float BAR_HEIGHT = 16.0f;
static constexpr float BAR_X = 8.0f;
// 左下角竖排（_layout_left_column）：生命→灵力→[法则·化神起显示]→修为→饱食→境界（寿元移个人信息页）
static constexpr float HEALTH_BAR_Y = 6.0f;
static constexpr float ENERGY_BAR_Y = 24.0f; // 灵力（法力）
static constexpr float LAW_BAR_Y = 42.0f;    // 法则（化神解锁后显示；隐藏时下方元素上移补位）
static constexpr float XP_BAR_Y = 42.0f;     // 修为经验（百分比，法则隐藏时紧贴灵力）
static constexpr float FULLNESS_BAR_Y = 60.0f;
static constexpr float REALM_LABEL_Y = 78.0f;
static constexpr float ROW_STEP = 16.0f;     // 条高16，紧贴无间距
static constexpr int FONT_SIZE_XS = 9;
static constexpr int FONT_SIZE_MD = 14;
static constexpr int FONT_SIZE_LG = 20;

void GameHUD::_bind_methods() {
    ClassDB::bind_method(D_METHOD("on_player_health_changed", "current", "max"),
                         &GameHUD::on_player_health_changed);
    ClassDB::bind_method(D_METHOD("on_buffs_changed", "active"), &GameHUD::on_buffs_changed);
    ClassDB::bind_method(D_METHOD("on_continent_changed", "id", "name"), &GameHUD::on_continent_changed);
    ClassDB::bind_method(D_METHOD("on_dongtian_entered"), &GameHUD::on_dongtian_entered);
    ClassDB::bind_method(D_METHOD("on_dongtian_exited"), &GameHUD::on_dongtian_exited);
    ClassDB::bind_method(D_METHOD("on_ledger_inspect", "data", "show"), &GameHUD::on_ledger_inspect);
    ClassDB::bind_method(D_METHOD("on_fullness_changed", "current", "max"), &GameHUD::on_fullness_changed);
    ClassDB::bind_method(D_METHOD("on_bigu_changed", "bigu"), &GameHUD::on_bigu_changed);
    ClassDB::bind_method(D_METHOD("on_spiritual_energy_changed", "current", "max", "progress"),
                         &GameHUD::on_spiritual_energy_changed);
    ClassDB::bind_method(D_METHOD("on_mana_changed", "current", "max"),
                         &GameHUD::on_mana_changed);
    ClassDB::bind_method(D_METHOD("on_realm_changed", "old_realm", "new_realm", "realm_name"),
                         &GameHUD::on_realm_changed);
    ClassDB::bind_method(D_METHOD("on_combo_changed", "count"), &GameHUD::on_combo_changed);
    ClassDB::bind_method(D_METHOD("on_combo_ended", "final_count"), &GameHUD::on_combo_ended);
    ClassDB::bind_method(D_METHOD("on_interaction_prompt", "text", "show"),
                         &GameHUD::on_interaction_prompt);
    ClassDB::bind_method(D_METHOD("on_player_died"), &GameHUD::on_player_died);
    ClassDB::bind_method(D_METHOD("on_player_respawned"), &GameHUD::on_player_respawned);
    ClassDB::bind_method(D_METHOD("on_boss_fight_update", "name", "current", "max"),
                         &GameHUD::on_boss_fight_update);
    ClassDB::bind_method(D_METHOD("on_boss_fight_ended", "name"), &GameHUD::on_boss_fight_ended);
    ClassDB::bind_method(D_METHOD("is_boss_bar_visible"), &GameHUD::is_boss_bar_visible);
    ClassDB::bind_method(D_METHOD("get_boss_bar_name"), &GameHUD::get_boss_bar_name);
    ClassDB::bind_method(D_METHOD("get_boss_bar_count"), &GameHUD::get_boss_bar_count);
    ClassDB::bind_method(D_METHOD("set_hud_visible", "visible"), &GameHUD::set_hud_visible);
    ClassDB::bind_method(D_METHOD("is_hud_visible"), &GameHUD::is_hud_visible);
    ClassDB::bind_method(D_METHOD("set_all_visible", "visible"), &GameHUD::set_all_visible);
}

void GameHUD::_ready() {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    set_layer(100); // Topmost UI layer

    _create_health_bar();
    _create_energy_bar();
    _create_xp_bar();
    _create_realm_label();
    _create_jiyuan_label();
    _create_combo_label();
    _create_interact_prompt();
    _create_death_overlay();
    _create_skill_bar();
    _create_consumable_bar();
    _create_law_bar();
	_create_pressure_indicators();
	_create_ledger_overlay();
	_create_fullness_bar();

    // Buff 行（生命条下方小行）
    _buff_label = memnew(Label);
    _buff_label->set_name("BuffLabel");
    _buff_label->set_position(Vector2(8.0f, XP_BAR_Y + BAR_HEIGHT + 2.0f)); // y 由 _layout_left_column 接管
    _buff_label->add_theme_font_size_override("font_size", 8);
    _buff_label->add_theme_color_override("font_color", Color(0.7f, 0.9f, 1.0f, 1.0f));
    _buff_label->set_visible(false);
    add_child(_buff_label);
    // Boss 血条惰性创建（多 Boss 同场时按名动态增删）

    // 首帧布局：不等 _process → _sync_viewport（_vw/_vh 初始为 0，首帧必定触发重排）
    _vw = 0.0f;
    _vh = 0.0f;

    // 洲名横幅（进入新洲时大字淡入淡出，不随 _hud_visible 隐藏——过场也要看得到）
    _continent_label = memnew(Label);
    _continent_label->set_position(Vector2(0, 78));
    _continent_label->set_size(Vector2(480, 30));
    _continent_label->add_theme_font_size_override("font_size", 18);
    _continent_label->add_theme_color_override("font_color", Color(1.0f, 0.92f, 0.6f, 1.0f));
    _continent_label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9f));
    _continent_label->add_theme_constant_override("outline_size", 4);
    _continent_label->set_visible(false);
    add_child(_continent_label);

    set_process_unhandled_input(true);
    set_process(true); // 技能栏冷却轮询

    // 首帧立即布局（left_column 依赖 _vw/_vh，先设好再排；其余靠 _sync_viewport 首次触发）
    _vw = 480.0f;
    _vh = 270.0f;
    _layout_left_column();

    // Connect to SignalBus
    SignalBus *bus = SignalBus::get_singleton();
    if (bus) {
        bus->connect("player_health_changed", Callable(this, "on_player_health_changed"));
        bus->connect("spiritual_energy_changed", Callable(this, "on_spiritual_energy_changed"));
        bus->connect("mana_changed", Callable(this, "on_mana_changed"));
        bus->connect("realm_changed", Callable(this, "on_realm_changed"));
        bus->connect("combo_changed", Callable(this, "on_combo_changed"));
        bus->connect("combo_ended", Callable(this, "on_combo_ended"));
        bus->connect("interaction_prompt", Callable(this, "on_interaction_prompt"));
        bus->connect("player_died", Callable(this, "on_player_died"));
        bus->connect("boss_fight_update", Callable(this, "on_boss_fight_update"));
        bus->connect("boss_fight_ended", Callable(this, "on_boss_fight_ended"));
        bus->connect("player_respawned", Callable(this, "on_player_respawned"));
        bus->connect("buffs_changed", Callable(this, "on_buffs_changed"));
        bus->connect("continent_changed", Callable(this, "on_continent_changed"));
        bus->connect("dongtian_entered", Callable(this, "on_dongtian_entered"));
        bus->connect("dongtian_exited", Callable(this, "on_dongtian_exited"));
        bus->connect("ledger_inspect_requested", Callable(this, "on_ledger_inspect"));
        bus->connect("fullness_changed", Callable(this, "on_fullness_changed"));
        bus->connect("bigu_changed", Callable(this, "on_bigu_changed"));
	        bus->connect("language_changed", Callable(this, "_on_language_changed"));
    }
}

// ============================================================
// Health Bar
// ============================================================

// Shared builder: one bar (bg + fill + centered in-bar value text)
static void _build_bar(CanvasLayer *p_parent, float p_y, const Color &p_fill_color,
                       ColorRect *&r_bg, ColorRect *&r_fill, Label *&r_label,
                       const String &p_initial_text) {
    r_bg = memnew(ColorRect);
    r_bg->set_position(Vector2(BAR_X, p_y));
    r_bg->set_size(Vector2(BAR_WIDTH, BAR_HEIGHT));
    r_bg->set_color(Color(0.15f, 0.15f, 0.15f, 0.8f));
    p_parent->add_child(r_bg);

    r_fill = memnew(ColorRect);
    r_fill->set_position(Vector2(BAR_X, p_y));
    r_fill->set_size(Vector2(BAR_WIDTH, BAR_HEIGHT));
    r_fill->set_color(p_fill_color);
    p_parent->add_child(r_fill);

    // Value text drawn inside the bar (nudged up: font metrics sit low otherwise)
    r_label = memnew(Label);
    r_label->set_position(Vector2(BAR_X, p_y - 2.0f));
    r_label->set_size(Vector2(BAR_WIDTH, BAR_HEIGHT));
    r_label->add_theme_font_size_override("font_size", FONT_SIZE_XS);
    r_label->add_theme_color_override("font_color", Color(1, 1, 1, 1));
    r_label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9f));
    r_label->add_theme_constant_override("outline_size", 2);
    r_label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
    r_label->set_vertical_alignment(VerticalAlignment::VERTICAL_ALIGNMENT_CENTER);
    r_label->set_clip_text(true);
    r_label->set_text(p_initial_text);
    p_parent->add_child(r_label);
}

void GameHUD::_create_health_bar() {
    _build_bar(this, HEALTH_BAR_Y, _health_color,
               _health_bg, _health_fill, _health_label, LOC("生命 100/100"));
    _health_bg->set_name("HealthBg");
    _health_fill->set_name("HealthFill");
    _health_label->set_name("HealthLabel");
}

void GameHUD::_create_energy_bar() {
    _build_bar(this, ENERGY_BAR_Y, _energy_color,
               _energy_bg, _energy_fill, _energy_label, LOC("灵力 0/0"));
    _energy_bg->set_name("ManaBg");
    _energy_fill->set_name("ManaFill");
    _energy_label->set_name("ManaLabel");
    _energy_fill->set_size(Vector2(0, BAR_HEIGHT)); // starts empty
}

void GameHUD::_create_xp_bar() {
    _build_bar(this, XP_BAR_Y, _xp_color,
               _xp_bg, _xp_fill, _xp_label, LOC("修为 0%"));
    _xp_bg->set_name("XpBg");
    _xp_fill->set_name("XpFill");
    _xp_label->set_name("XpLabel");
    _xp_fill->set_size(Vector2(0, BAR_HEIGHT)); // starts empty
}

void GameHUD::_create_fullness_bar() {
    _build_bar(this, FULLNESS_BAR_Y, _fullness_color,
               _fullness_bg, _fullness_fill, _fullness_label, TXT("饱食 100/100"));
    _fullness_bg->set_name("FullnessBg");
    _fullness_fill->set_name("FullnessFill");
    _fullness_label->set_name("FullnessLabel");
}

// ============================================================
// Realm Label
// ============================================================

void GameHUD::_create_realm_label() {
    _realm_label = memnew(Label);
    _realm_label->set_name("RealmLabel");
    _realm_label->set_position(Vector2(BAR_X, REALM_LABEL_Y));
    _realm_label->add_theme_font_size_override("font_size", FONT_SIZE_MD);
    _realm_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
    _realm_label->set_text(LOC("凡人"));
    add_child(_realm_label);
}

void GameHUD::_create_jiyuan_label() {
    _jiyuan_label = memnew(Label);
    _jiyuan_label->set_name("JiyuanLabel");
    _jiyuan_label->set_position(Vector2(BAR_X + 170.0f, REALM_LABEL_Y + 4.0f));
    _jiyuan_label->add_theme_font_size_override("font_size", FONT_SIZE_XS);
    _jiyuan_label->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.4f, 1));
    _jiyuan_label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9f));
    _jiyuan_label->add_theme_constant_override("outline_size", 2);
    _jiyuan_label->set_text(LOC("机缘已至 [L]"));
    _jiyuan_label->set_visible(false);
    add_child(_jiyuan_label);
}

void GameHUD::_create_ledger_overlay() {
    // 查生死簿 overlay（判官 X 触发）：半透明底 + 5 行
    _ledger_overlay = memnew(ColorRect);
    _ledger_overlay->set_name("LedgerOverlay");
    _ledger_overlay->set_position(Vector2(120, 58));
    _ledger_overlay->set_size(Vector2(240, 116));
    _ledger_overlay->set_color(Color(0.05f, 0.05f, 0.12f, 0.92f));
    _ledger_overlay->set_visible(false);
    add_child(_ledger_overlay);

    static const float LINE_Y[5] = { 10.0f, 26.0f, 42.0f, 58.0f, 74.0f };
    for (int i = 0; i < 5; i++) {
        Label *l = memnew(Label);
        l->set_position(Vector2(8, LINE_Y[i]));
        l->add_theme_font_size_override("font_size", FONT_SIZE_XS);
        l->add_theme_color_override("font_color", Color(0.9f, 0.9f, 0.95f, 1));
        _ledger_overlay->add_child(l);
        _ledger_lines.push_back(l);
    }
}

// ============================================================
// Combo Counter
// ============================================================

void GameHUD::_create_combo_label() {
    _combo_label = memnew(Label);
    _combo_label->set_name("ComboLabel");
    _combo_label->set_position(Vector2(380, 130)); // mid-right, clear of telemetry
    _combo_label->add_theme_font_size_override("font_size", FONT_SIZE_LG);
    _combo_label->add_theme_color_override("font_color", Color(1.0f, 0.6f, 0.1f, 1));
    _combo_label->set_visible(false);
    _combo_label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
    add_child(_combo_label);
}

// ============================================================
// Interaction Prompt
// ============================================================

void GameHUD::_create_interact_prompt() {
    _interact_label = memnew(Label);
    _interact_label->set_name("InteractLabel");
    _interact_label->set_position(Vector2(0, 230));
    _interact_label->set_size(Vector2(480, 30));
    _interact_label->add_theme_font_size_override("font_size", FONT_SIZE_MD);
    _interact_label->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.5f, 1));
    _interact_label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
    _interact_label->set_visible(false);
    add_child(_interact_label);
}

// ============================================================
// Death Overlay
// ============================================================

void GameHUD::_create_death_overlay() {
    _death_overlay = memnew(ColorRect);
    _death_overlay->set_name("DeathOverlay");
    _death_overlay->set_position(Vector2(0, 0));
    _death_overlay->set_size(Vector2(480, 270));
    _death_overlay->set_color(Color(1, 0.05f, 0.05f, 0.3f));
    _death_overlay->set_visible(false);
    add_child(_death_overlay);
}

// ============================================================
// 底部技能/法宝栏（占位）
// ============================================================

void GameHUD::_create_skill_bar() {
    // QWERTY+ASDFGH 12 技能槽，两排紧凑居中：上行 QWERTY(slot 0..5) / 下行 ASDFGH(slot 6..11)。
    // 槽内填技能名首字 + 冷却秒数；空槽显 ·，锁定显 ×，渡劫次要法宝灰显。
    struct SlotSpec { const char *key; Color tint; };
    static const Color WU(0.55f, 0.25f, 0.20f, 0.85f);   // 武技 红棕
    static const Color FA(0.20f, 0.35f, 0.60f, 0.85f);   // 法术 蓝
    static const Color SHEN(0.40f, 0.20f, 0.55f, 0.85f); // 神通 紫
    static const Color XIAN(0.60f, 0.55f, 0.25f, 0.85f); // 仙法 金
    // 12 槽按 slot 索引：Q W E R T Y / A S D F G H
    static const SlotSpec SLOTS[12] = {
        { "Q", WU }, { "W", WU }, { "E", FA }, { "R", FA }, { "T", SHEN }, { "Y", XIAN }, // 上行
        { "A", WU }, { "S", WU }, { "D", FA }, { "F", FA }, { "G", SHEN }, { "H", SHEN }, // 下行
    };
    const float SLOT_W = 20.0f; // 槽尺寸（位置由 _layout_skill_bar() 统一锚定，渲染比例自适应）
    const int N = 12;

    for (int i = 0; i < N; i++) {
        float x = 0.0f, y = 0.0f;

        ColorRect *slot = memnew(ColorRect);
        slot->set_position(Vector2(x, y));
        slot->set_size(Vector2(SLOT_W, SLOT_W));
        slot->set_color(SLOTS[i].tint);
        add_child(slot);
        _skill_bar_nodes.push_back(slot);

        Label *key = memnew(Label);
        key->set_text(SLOTS[i].key); // ASCII 键名无需 TXT
        key->add_theme_font_size_override("font_size", 8);
        key->add_theme_color_override("font_color", Color(0.75f, 0.75f, 0.75f, 0.9f));
        key->set_position(Vector2(x + 1, y + 1));
        add_child(key);
        _skill_bar_nodes.push_back(key);

        // 技能名（槽中央，空槽显示 ·；名字取首字，像素屏宽所限）
        Label *name = memnew(Label);
        name->set_text(LOC("·"));
        name->add_theme_font_size_override("font_size", 8);
        name->add_theme_color_override("font_color", Color(1.0f, 1.0f, 1.0f, 0.95f));
        name->set_position(Vector2(x + 6, y + 7));
        add_child(name);
        _skill_bar_nodes.push_back(name);
        _skill_name_labels.push_back(name);

        // 冷却剩余秒（槽右下角，仅冷却中显示）
        Label *cd = memnew(Label);
        cd->set_text("");
        cd->add_theme_font_size_override("font_size", 7);
        cd->add_theme_color_override("font_color", Color(1.0f, 0.9f, 0.4f, 0.95f));
        cd->set_position(Vector2(x + 10, y + 12));
        add_child(cd);
        _skill_bar_nodes.push_back(cd);
        _skill_cd_labels.push_back(cd);
    }

    // 法宝页徽标（B 切换；仅法宝页显示）
    _page_badge = memnew(Label);
    _page_badge->set_text(LOC("法宝页 [B 返回]"));
    _page_badge->add_theme_font_size_override("font_size", 8);
    _page_badge->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.4f, 0.95f));
    _page_badge->set_position(Vector2(0, 0)); // _layout_skill_bar() 接管
    _page_badge->set_visible(false);
    add_child(_page_badge);
    _skill_bar_nodes.push_back(_page_badge);
    _layout_skill_bar();
}

// ============================================================
// 技能栏刷新（轮询 Player->SkillSystem；冷却走 Player._time 时基）
// ============================================================

// ============================================================
// 渲染比例自适应布局（16:9 基准 480×270；其他比例单边延伸视野，
// 右/中/底锚定元素跟随 content_scale_size 重排——渲染分辨率与窗口解耦）
// ============================================================

void GameHUD::_sync_viewport() {
    Vector2i cs;
    if (get_tree() && get_tree()->get_root()) cs = get_tree()->get_root()->get_content_scale_size();
    float vw = cs.x > 0 ? float(cs.x) : 480.0f;
    float vh = cs.y > 0 ? float(cs.y) : 270.0f;
    if (vw == _vw && vh == _vh)
        return;
    _vw = vw;
    _vh = vh;
    _relayout_hud();
}

void GameHUD::_relayout_hud() {
    _layout_right_side();
    _layout_left_column();
    _layout_skill_bar();
    _layout_consumable_bar();
    // 全宽居中标签/全屏遮罩
    if (_continent_label) _continent_label->set_size(Vector2(_vw, 30));
    if (_interact_label) {
        _interact_label->set_position(Vector2(0, _vh - 40.0f));
        _interact_label->set_size(Vector2(_vw, 30));
    }
    if (_death_overlay) _death_overlay->set_size(Vector2(_vw, _vh));
    if (_ledger_overlay) _ledger_overlay->set_position(Vector2((_vw - 240.0f) * 0.5f, 58));
    if (_combo_label) _combo_label->set_position(Vector2(_vw - 100.0f, 130));
    _relayout_boss_bars();
}

// 右上：威压/灵压指示器（法则条已移左列生命/灵力之下）
void GameHUD::_layout_right_side() {
    const float rx = _vw - BAR_WIDTH - 8.0f;
    if (_wei_bg) {
        _wei_bg->set_position(Vector2(rx, 6.0f));
        _wei_label->set_position(Vector2(rx + 2, 5.0f));
    }
    if (_lin_bg) {
        _lin_bg->set_position(Vector2(rx + 70.0f, 6.0f));
        _lin_label->set_position(Vector2(rx + 72.0f, 5.0f));
    }
}

// 左下角：数值条竖排（生命→灵力→[法则]→修为→饱食→境界），自下而上堆叠于左下角。
// 法则条化神解锁才显示——隐藏时下方元素上移补位，不留空槽。寿元已移入 ESC 个人信息页。
void GameHUD::_layout_left_column() {
    const float bx = BAR_X; // 贴左
    // 自下而上：境界 → 饱食 → buff → 修为 → [法则] → 灵力 → 生命
    float y = _vh - 48.0f - ROW_STEP; // 境界行（技能栏顶行上方一格）
    if (_realm_label) _realm_label->set_position(Vector2(bx, y));
    if (_jiyuan_label) _jiyuan_label->set_position(Vector2(bx + BAR_WIDTH + 8.0f, y + 4.0f));
    y -= ROW_STEP;
    if (_fullness_bg) {
        _fullness_bg->set_position(Vector2(bx, y));
        _fullness_fill->set_position(Vector2(bx, y));
        _fullness_label->set_position(Vector2(bx, y - 2.0f));
    }
    y -= ROW_STEP;
    if (_buff_label) _buff_label->set_position(Vector2(bx, y + 4.0f));
    y -= ROW_STEP;
    if (_xp_bg) {
        _xp_bg->set_position(Vector2(bx, y));
        _xp_fill->set_position(Vector2(bx, y));
        _xp_label->set_position(Vector2(bx, y - 2.0f));
    }
    y -= ROW_STEP;
    if (_law_shown && _law_bg) {
        _law_bg->set_position(Vector2(bx, y));
        _law_fill->set_position(Vector2(bx, y));
        _law_label->set_position(Vector2(bx, y - 2.0f));
        y -= ROW_STEP;
    }
    if (_energy_bg) {
        _energy_bg->set_position(Vector2(bx, y));
        _energy_fill->set_position(Vector2(bx, y));
        _energy_label->set_position(Vector2(bx, y - 2.0f));
    }
    y -= ROW_STEP;
    if (_health_bg) {
        _health_bg->set_position(Vector2(bx, y));
        _health_fill->set_position(Vector2(bx, y));
        _health_label->set_position(Vector2(bx, y - 2.0f));
    }
}

// 底部居中：12 技能槽两排（节点顺序 = 创建顺序，4 节点/槽：slot/key/name/cd）
void GameHUD::_layout_skill_bar() {
    if (_skill_bar_nodes.size() < 12 * 4)
        return;
    const float SLOT_W = 20.0f, SLOT_GAP = 2.0f;
    const int COLS = 6;
    const float row_w = COLS * SLOT_W + (COLS - 1) * SLOT_GAP; // 130
    const float x0 = (_vw - row_w) * 0.5f;
    const float y_top = _vh - 48.0f;
    const float y_bot = _vh - 24.0f;
    for (int i = 0; i < 12; i++) {
        int row = i / COLS, col = i % COLS;
        float x = x0 + col * (SLOT_W + SLOT_GAP);
        float y = (row == 0) ? y_top : y_bot;
        if (Control *slot = Object::cast_to<Control>(_skill_bar_nodes[i * 4 + 0]))
            slot->set_position(Vector2(x, y));
        if (Control *key = Object::cast_to<Control>(_skill_bar_nodes[i * 4 + 1]))
            key->set_position(Vector2(x + 1, y + 1));
        _skill_name_labels[i]->set_position(Vector2(x + 6, y + 7));
        _skill_cd_labels[i]->set_position(Vector2(x + 10, y + 12));
    }
    if (_page_badge)
        _page_badge->set_position(Vector2(x0 + row_w + 8.0f, y_bot + 6.0f));
}

// 左下角：消耗品栏（贴左贴底；_skill_bar_nodes 中下标 49 起 = 12 技能槽×4 + page_badge）
void GameHUD::_layout_consumable_bar() {
    const int base = 12 * 4 + 1;
    if (_bar_name_labels.empty() || (int)_skill_bar_nodes.size() < base + 6 * 4)
        return;
    const float SLOT_W = 20.0f, SLOT_GAP = 2.0f;
    const float x0 = 8.0f, y = _vh - 24.0f;
    for (int i = 0; i < 6; i++) {
        float x = x0 + i * (SLOT_W + SLOT_GAP);
        int b = base + i * 4;
        if (Control *slot = Object::cast_to<Control>(_skill_bar_nodes[b + 0]))
            slot->set_position(Vector2(x, y));
        if (Control *key = Object::cast_to<Control>(_skill_bar_nodes[b + 1]))
            key->set_position(Vector2(x + 1, y + 1));
        _bar_name_labels[i]->set_position(Vector2(x + 6, y + 4));
        _bar_count_labels[i]->set_position(Vector2(x + 12, y + 12));
    }
}

void GameHUD::_process(double p_delta) {
    _sync_viewport(); // 渲染比例变更时重排锚定元素
    if (Engine::get_singleton()->is_editor_hint())
        return;
    _update_skill_bar();
    _update_consumable_bar();
    _update_law_bar();
	_update_pressure_indicators();
    _update_buff_label(p_delta);

    // 洲名横幅：2.8s 展示，末 0.8s 淡出
    if (_continent_banner_t > 0.0f && _continent_label) {
        _continent_banner_t -= (float)p_delta;
        if (_continent_banner_t <= 0.0f) {
            _continent_banner_t = 0.0f;
            _continent_label->set_visible(false);
        } else if (_continent_banner_t < 0.8f) {
            Color c = _continent_label->get_theme_color("font_color");
            c.a = _continent_banner_t / 0.8f;
            _continent_label->add_theme_color_override("font_color", c);
        }
    }
}

void GameHUD::on_continent_changed(const String &p_id, const String &p_name) {
    if (!_continent_label) return;
    _continent_label->set_text(LOC("—— ") + LOC(p_name) + LOC(" ——"));
    Color c = _continent_label->get_theme_color("font_color");
    c.a = 1.0f;
    _continent_label->add_theme_color_override("font_color", c);
    _continent_label->set_visible(true);
    _continent_banner_t = 2.8f;
}

void GameHUD::on_dongtian_entered() {
    if (!_continent_label) return;
    _continent_label->set_text(LOC("—— 洞天 · 灵地一隅 ——"));
    Color c = _continent_label->get_theme_color("font_color");
    c.a = 1.0f;
    _continent_label->add_theme_color_override("font_color", c);
    _continent_label->set_visible(true);
    _continent_banner_t = 2.8f;
}

void GameHUD::on_dongtian_exited() {
    // 回到外界：重播当前洲横幅（惰性查找，同 GameMenu 的 ContinentManager 模式）
    Node *root = get_tree()->get_current_scene();
    ContinentManager *cm = root ? Object::cast_to<ContinentManager>(
            root->find_child("ContinentManager", false, false)) : nullptr;
    if (cm && !cm->get_current_id().is_empty()) {
        on_continent_changed(cm->get_current_id(), cm->get_current_name());
    }
}

void GameHUD::_on_language_changed(const String &p_locale) {
    // Refresh static labels that are set once in _create_* methods
    if (_realm_label) _realm_label->set_text(LOC("凡人"));
    if (_jiyuan_label) _jiyuan_label->set_text(LOC("机缘已至 [L]"));
    if (_page_badge) _page_badge->set_text(LOC("法宝页 [B 返回]"));
    // 灵力/修为条前缀随境界（凡尘=灵力/修为，仙级=仙元）重取本地化
    bool immortal = false;
    if (!_player && get_tree()) {
        _player = Object::cast_to<Player>(get_tree()->get_root()->find_child("Player", true, false));
    }
    if (_player && _player->get_cultivation()) {
        immortal = _player->get_cultivation()->get_realm_index() >= CultivationSystem::TRUE_IMMORTAL;
    }
    _mana_prefix = immortal ? LOC("仙元") : LOC("灵力");
    _xp_prefix = immortal ? LOC("仙元") : LOC("修为");
    _realm_name = LOC("凡人");
    _refresh_mana_label();
    _refresh_xp_label();
    _refresh_realm_label();
    _update_buff_label(0.0f);
}

void GameHUD::on_buffs_changed(const Array &p_active) {
    _buffs = p_active.duplicate(true);
    _buff_refresh = 0.0f; // 立即重绘
    if (_buffs.is_empty() && _buff_label) {
        _buff_label->set_visible(false);
    }
}

void GameHUD::_update_buff_label(double p_delta) {
    if (!_buff_label) return;
    if (_buffs.is_empty()) return;

    // 本地倒计时（apply/到期时 buffs_changed 会重新同步，漂移可接受）
    for (int i = (int)_buffs.size() - 1; i >= 0; i--) {
        Dictionary d = _buffs[i];
        float rem = float(d["remaining"]) - (float)p_delta;
        if (rem <= 0.0f) {
            _buffs.remove_at(i); // 本地先消失；服务端到期信号随后清场
        } else {
            d["remaining"] = rem;
            _buffs[i] = d;
        }
    }
    if (_buffs.is_empty()) {
        _buff_label->set_visible(false);
        return;
    }

    _buff_refresh -= (float)p_delta;
    if (_buff_refresh > 0.0f) return;
    _buff_refresh = 0.5f; // 0.5s 重绘一次足够

    String text;
    for (int i = 0; i < _buffs.size(); i++) {
        Dictionary d = _buffs[i];
        if (i > 0) text += "  ";
        text += LOC(String(d["name"])) + " " + String::num_int64((int64_t)Math::ceil(float(d["remaining"]))) + "s";
    }
    _buff_label->set_text(text);
    _buff_label->set_visible(_hud_visible);
}

// ============================================================
// 法则之力条（右上角；化神解锁后显示）
// ============================================================

void GameHUD::_create_law_bar() {
    // 法则之力：化神解锁后显示在左列（生命/灵力之下，与修为/饱食同列动态堆叠）
    _build_bar(this, LAW_BAR_Y, Color(0.55f, 0.35f, 0.9f, 1.0f),
               _law_bg, _law_fill, _law_label, LOC("法则 0/100"));
    _law_bg->set_name("LawBg");
    _law_fill->set_name("LawFill");
    _law_label->set_name("LawLabel");
    _law_fill->set_size(Vector2(0, BAR_HEIGHT));
    _law_bg->set_visible(false);
    _law_fill->set_visible(false);
    _law_label->set_visible(false);
}

void GameHUD::_update_law_bar() {
    if (!_law_bg)
        return;
    if (!_player)
        return; // 技能栏轮询先行缓存 _player
    CultivationSystem *cult = _player->get_cultivation();
    if (!cult)
        return;
    double max_law = cult->get_law_power_max();
    bool show = max_law > 0.0 && _hud_visible;
    if (show != _law_shown) {
        _law_shown = show;
        _law_bg->set_visible(show);
        _law_fill->set_visible(show);
        _law_label->set_visible(show);
        _layout_left_column(); // 显隐切换 → 左列重排补位
    }
    if (!show)
        return;
    double cur = cult->get_law_power();
    _law_fill->set_size(Vector2(BAR_WIDTH * float(cur / max_law), BAR_HEIGHT));
    _law_label->set_text(LOC("法则 ") + String::num_int64(int64_t(cur)) + LOC("/") + String::num_int64(int64_t(max_law)));
}

// 数字键消耗品栏（技能栏上方一行：1~6 槽，名首字+数量；design/alchemy.md S6）

// ============================================================
// 威压/灵压冷却指示器（法则条下方；就绪亮色 / 冷却灰+秒）
// ============================================================

void GameHUD::_create_pressure_indicators() {
	const float x0 = 0.0f; // 由 _layout_right_side() 锚定（与法则条同左缘）
	const float y = 20.0f;
	const float W = 66.0f, H = 14.0f;

	// V 威压
	_wei_bg = memnew(ColorRect);
	_wei_bg->set_position(Vector2(x0, y));
	_wei_bg->set_size(Vector2(W, H));
	_wei_bg->set_color(Color(0.12f, 0.10f, 0.05f, 0.85f));
	add_child(_wei_bg);

	_wei_label = memnew(Label);
	_wei_label->set_text(LOC("U 威压"));
	_wei_label->add_theme_font_size_override("font_size", FONT_SIZE_XS);
	_wei_label->add_theme_color_override("font_color", Color(0.9f, 0.7f, 0.3f, 0.95f));
	_wei_label->set_position(Vector2(x0 + 2, y - 1));
	add_child(_wei_label);

	// I 灵压
	_lin_bg = memnew(ColorRect);
	_lin_bg->set_position(Vector2(x0 + W + 4, y));
	_lin_bg->set_size(Vector2(W, H));
	_lin_bg->set_color(Color(0.10f, 0.06f, 0.14f, 0.85f));
	add_child(_lin_bg);

	_lin_label = memnew(Label);
	_lin_label->set_text(LOC("I 灵压"));
	_lin_label->add_theme_font_size_override("font_size", FONT_SIZE_XS);
	_lin_label->add_theme_color_override("font_color", Color(0.7f, 0.5f, 0.95f, 0.95f));
	_lin_label->set_position(Vector2(x0 + W + 6, y - 1));
	add_child(_lin_label);
	_layout_right_side();
}

void GameHUD::_update_pressure_indicators() {
	if (!_wei_bg || !_lin_bg) return;
	if (!_player) return;

	bool show = _hud_visible;
	_wei_bg->set_visible(show);
	_wei_label->set_visible(show);
	_lin_bg->set_visible(show);
	_lin_label->set_visible(show);
	if (!show) return;

	double wei_cd = _player->get_wei_cooldown_left();
	double lin_cd = _player->get_lin_cooldown_left();

	if (wei_cd > 0.05) {
		_wei_label->set_text(vformat(LOC("U %.1fs"), wei_cd));
		_wei_label->add_theme_color_override("font_color", Color(0.4f, 0.35f, 0.30f, 0.9f));
		_wei_bg->set_color(Color(0.08f, 0.06f, 0.03f, 0.7f));
	} else {
		_wei_label->set_text(LOC("U 威压"));
		_wei_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.35f, 0.95f));
		_wei_bg->set_color(Color(0.18f, 0.14f, 0.05f, 0.85f));
	}

	if (lin_cd > 0.05) {
		_lin_label->set_text(vformat(LOC("I %.1fs"), lin_cd));
		_lin_label->add_theme_color_override("font_color", Color(0.40f, 0.30f, 0.35f, 0.9f));
		_lin_bg->set_color(Color(0.06f, 0.03f, 0.10f, 0.7f));
	} else {
		_lin_label->set_text(LOC("I 灵压"));
		_lin_label->add_theme_color_override("font_color", Color(0.85f, 0.65f, 1.0f, 0.95f));
		_lin_bg->set_color(Color(0.12f, 0.07f, 0.18f, 0.85f));
	}
}

void GameHUD::_create_consumable_bar() {
    const float SLOT_W = 20.0f; // 槽尺寸（位置由 _layout_consumable_bar() 统一锚定，底边自适应）
    const int N = 6;
    const float y = 0.0f;

    for (int i = 0; i < N; i++) {
        float x = 0.0f;

        ColorRect *slot = memnew(ColorRect);
        slot->set_position(Vector2(x, y));
        slot->set_size(Vector2(SLOT_W, SLOT_W));
        slot->set_color(Color(0.25f, 0.35f, 0.25f, 0.85f));
        add_child(slot);
        _skill_bar_nodes.push_back(slot);

        Label *key = memnew(Label);
        key->set_text(String::num_int64(i + 1)); // ASCII 数字无需 TXT
        key->add_theme_font_size_override("font_size", 8);
        key->add_theme_color_override("font_color", Color(0.75f, 0.75f, 0.75f, 0.9f));
        key->set_position(Vector2(x + 1, y + 1));
        add_child(key);
        _skill_bar_nodes.push_back(key);

        Label *name = memnew(Label);
        name->set_text(LOC("·"));
        name->add_theme_font_size_override("font_size", 10);
        name->add_theme_color_override("font_color", Color(1, 1, 1, 0.95f));
        name->set_position(Vector2(x + 6, y + 4));
        add_child(name);
        _skill_bar_nodes.push_back(name);
        _bar_name_labels.push_back(name);

        Label *count = memnew(Label);
        count->set_text("");
        count->add_theme_font_size_override("font_size", 8);
        count->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.6f, 0.95f));
        count->set_position(Vector2(x + 12, y + 12));
        add_child(count);
        _skill_bar_nodes.push_back(count);
        _bar_count_labels.push_back(count);
    }
    _layout_consumable_bar();
}

void GameHUD::_update_consumable_bar() {
    if (_bar_name_labels.empty())
        return;
    if (!_player)
        return; // _update_skill_bar 先行缓存
    Inventory *inv = _player->get_inventory();
    ItemDatabase *db = ItemDatabase::get_singleton();
    for (int i = 0; i < (int)_bar_name_labels.size(); i++) {
        StringName id = _player->get_consumable_bar_slot(i);
        if (id == StringName()) {
            _bar_name_labels[i]->set_text(LOC("·"));
            _bar_name_labels[i]->add_theme_color_override("font_color", Color(1, 1, 1, 0.35f));
            _bar_count_labels[i]->set_text("");
            continue;
        }
        const Item *def = db ? db->get_item(id) : nullptr;
        String name = def ? LOC(def->name) : String(id);
        _bar_name_labels[i]->set_text(name.substr(0, 1));
        int qty = inv ? inv->get_item_count(id) : 0;
        _bar_count_labels[i]->set_text(qty > 0 ? String::num_int64(qty) : "");
        // 耗尽暗显
        _bar_name_labels[i]->add_theme_color_override("font_color",
            qty > 0 ? Color(1, 1, 1, 0.95f) : Color(1, 1, 1, 0.35f));
    }
}

void GameHUD::_update_skill_bar() {
    if (_skill_name_labels.empty())
        return;
    if (!_player) {
        Node *n = get_tree() ? get_tree()->get_root()->find_child("Player", true, false) : nullptr;
        _player = Object::cast_to<Player>(n);
        if (!_player)
            return;
    }
    int page = _player->get_skill_page();
    if (_page_badge)
        _page_badge->set_visible(page == 1);
    // 槽 i 的数据源：法宝页且对应下行 A~H（slot 6..11）→ 法宝槽 0..5；其余 → 技能槽
    SkillSystem *skills = _player->get_skills();
    ArtifactSystem *arts = _player->get_artifacts();
    for (int i = 0; i < (int)_skill_name_labels.size(); i++) {
        bool artifact_side = (page == 1 && i >= 6 && i < 12); // 下行 ASDFGH
        int art_slot = i - 6;
        Dictionary info = artifact_side
            ? (arts ? arts->get_slot_info(art_slot) : Dictionary())
            : (skills ? skills->get_slot_info(i) : Dictionary());
        if (info.is_empty() || info.has("locked")) {
            _skill_name_labels[i]->set_text(info.has("locked") ? LOC("×") : LOC("·"));
            _skill_cd_labels[i]->set_text("");
            continue;
        }
        // 渡劫「只带本命法宝」：次要法宝灰显（×）
        if (info.has("tribulation_off")) {
            _skill_name_labels[i]->set_text(LOC("×"));
            _skill_name_labels[i]->add_theme_color_override("font_color", Color(1, 1, 1, 0.35f));
            _skill_cd_labels[i]->set_text("");
            continue;
        }
        String name = info.get("name", "");
        _skill_name_labels[i]->set_text(name.is_empty() ? LOC("·") : name.substr(0, 1));
        double rem = double(info.get("cd_remaining", 0.0));
        Color ready_c = artifact_side ? Color(1.0f, 0.85f, 0.4f, 0.95f) : Color(1, 1, 1, 0.95f);
        if (rem > 0.05) {
            _skill_cd_labels[i]->set_text(String::num(rem, 1));
            _skill_name_labels[i]->add_theme_color_override("font_color", Color(1, 1, 1, 0.4f));
        } else {
            _skill_cd_labels[i]->set_text("");
            _skill_name_labels[i]->add_theme_color_override("font_color", ready_c);
        }
    }
}

// ============================================================
// Boss 血条（顶部居中；SignalBus 驱动）
// ============================================================

GameHUD::BossBarUi *GameHUD::_create_boss_bar_item() {
    // bg/fill/名字直接挂 CanvasLayer；名字用 font 精确度量绝对定位居中（见 on_boss_fight_update）
    const float W = 240.0f, H = 16.0f;
    const float x = (_vw - W) * 0.5f;

    _boss_bars.push_back(BossBarUi());
    BossBarUi &bar = _boss_bars.back();

    bar.bg = memnew(ColorRect);
    bar.bg->set_position(Vector2(x, 8.0f));
    bar.bg->set_size(Vector2(W, H));
    bar.bg->set_color(Color(0.10f, 0.06f, 0.06f, 0.88f));
    bar.bg->set_visible(false);
    add_child(bar.bg);

    bar.fill = memnew(ColorRect);
    bar.fill->set_position(Vector2(x, 8.0f));
    bar.fill->set_size(Vector2(W, H));
    bar.fill->set_color(Color(0.80f, 0.12f, 0.12f, 1.0f));
    bar.fill->set_visible(false);
    add_child(bar.fill);

    bar.name_label = memnew(Label);
    bar.name_label->set_text("");
    bar.name_label->add_theme_font_size_override("font_size", 9);
    bar.name_label->add_theme_color_override("font_color", Color(1.0f, 0.95f, 0.9f, 0.98f));
    bar.name_label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.85f));
    bar.name_label->add_theme_constant_override("outline_size", 2);
    bar.name_label->set_visible(false);
    add_child(bar.name_label);

    return &_boss_bars.back();
}

GameHUD::BossBarUi *GameHUD::_find_boss_bar(const String &p_name) {
    for (BossBarUi &b : _boss_bars) {
        if (b.name == p_name) return &b;
    }
    return nullptr;
}

void GameHUD::_relayout_boss_bars() {
    // 任意数量：自上而下排列，血条 16px/条（名字在条内居中）
    const float W = 240.0f, H = 16.0f;
    const float x = (_vw - W) * 0.5f;
    for (int i = 0; i < (int)_boss_bars.size(); i++) {
        BossBarUi &b = _boss_bars[i];
        float y = 8.0f + (float)i * 18.0f;
        if (b.bg) b.bg->set_position(Vector2(x, y));
        if (b.fill) b.fill->set_position(Vector2(x, y));
        if (b.name_label) _position_boss_name(&b, x, y);
    }
}

// 名字用 font 实际度量（宽度=get_string_size.x，高度=get_height）绝对定位居中于血条，
// 再上移 2px 补偿 CJK 字形在 line-height 内的视觉偏下
void GameHUD::_position_boss_name(BossBarUi *p_bar, float p_x, float p_y) {
    const float W = 240.0f, H = 16.0f;
    Label *lbl = p_bar->name_label;
    Ref<Font> font = lbl->get_theme_font("font");
    int fs = lbl->get_theme_font_size("font_size");
    Size2 t = font.is_valid()
            ? font->get_string_size(lbl->get_text(), HORIZONTAL_ALIGNMENT_LEFT, -1, fs)
            : lbl->get_minimum_size();
    float tw = Math::min((float)t.x, W);
    float th = font.is_valid() ? (float)font->get_height(fs) : Math::min((float)t.y, H);
    lbl->set_position(Vector2(p_x + (W - tw) * 0.5f, p_y + (H - th) * 0.5f - 1.0f));
    lbl->set_size(Vector2(tw, th));
}

void GameHUD::on_boss_fight_update(const String &p_name, double p_current, double p_max) {
    if (p_max <= 0.0)
        return;
    BossBarUi *bar = _find_boss_bar(p_name);
    if (!bar) {
        bar = _create_boss_bar_item();
        bar->name = p_name;
        bar->realm_tag = _boss_realm_tag(p_name); // 建条时解析一次缓存（Boss realm 战内不变）
        _relayout_boss_bars();
    }
    float frac = Math::clamp(float(p_current / p_max), 0.0f, 1.0f);
    bar->fill->set_size(Vector2(240.0f * frac, 16.0f));
    // 名字后附修为境界名（压迫感）：「幽谷螭龙 · 大乘」；无 realm 标注的老怪只显名字
    String disp = LOC(p_name);
    if (!bar->realm_tag.is_empty())
        disp += TXT(" · ") + bar->realm_tag;
    bar->name_label->set_text(disp);
    bar->alive = p_current > 0.0;
    bool show = _hud_visible && bar->alive;
    bar->bg->set_visible(show);
    bar->fill->set_visible(show);
    bar->name_label->set_visible(show);
    _relayout_boss_bars(); // 名字按最新文本宽度重定位居中
}

String GameHUD::_boss_realm_tag(const String &p_name) const {
    SceneTree *st = get_tree();
    if (!st)
        return String();
    TypedArray<Node> enemies = st->get_nodes_in_group("enemies");
    for (int i = 0; i < enemies.size(); i++) {
        Enemy *e = Object::cast_to<Enemy>(enemies[i]);
        if (!e)
            continue;
        String nm = e->get_display_name().is_empty() ? String(e->get_name()) : e->get_display_name();
        if (nm == p_name && e->get_realm() > 0) { // realm 0 = 未标注，不显「凡人」
            return CultivationSystem::realm_name_of(e->get_realm());
        }
    }
    return String();
}

void GameHUD::on_boss_fight_ended(const String &p_name) {
    for (auto it = _boss_bars.begin(); it != _boss_bars.end(); ++it) {
        if (it->name == p_name) {
            if (it->bg) it->bg->queue_free();
            if (it->fill) it->fill->queue_free();
            if (it->name_label) it->name_label->queue_free();
            _boss_bars.erase(it);
            _relayout_boss_bars();
            return;
        }
    }
}

// ============================================================
// Input
// ============================================================

void GameHUD::_unhandled_input(const Ref<InputEvent> &p_event) {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    Ref<InputEventKey> key = p_event;
    if (key.is_null() || !key->is_pressed() || key->is_echo())
        return;

    if (key->get_keycode() == KEY_F4) { // Toggle gameplay HUD
        set_hud_visible(!_hud_visible);
    }
}

// ============================================================
// Visibility switches
// ============================================================

void GameHUD::_apply_hud_visibility() {
    if (_health_bg)     _health_bg->set_visible(_hud_visible);
    if (_health_fill)   _health_fill->set_visible(_hud_visible);
    if (_health_label)  _health_label->set_visible(_hud_visible);
    if (_energy_bg)     _energy_bg->set_visible(_hud_visible);
    if (_energy_fill)   _energy_fill->set_visible(_hud_visible);
    if (_energy_label)  _energy_label->set_visible(_hud_visible);
    if (_xp_bg)         _xp_bg->set_visible(_hud_visible);
    if (_xp_fill)       _xp_fill->set_visible(_hud_visible);
    if (_xp_label)      _xp_label->set_visible(_hud_visible);
    if (_realm_label)   _realm_label->set_visible(_hud_visible);
    if (_jiyuan_label)  _jiyuan_label->set_visible(_hud_visible && _xp_progress >= 1.0f);
    // 饱食度条：辟谷后隐藏
    bool show_fullness = _hud_visible && !_bigu;
    if (_fullness_bg) _fullness_bg->set_visible(show_fullness);
    if (_fullness_fill) _fullness_fill->set_visible(show_fullness);
    if (_fullness_label) _fullness_label->set_visible(show_fullness);
    // Combo/prompt manage their own visibility; only show when HUD is on
    if (_combo_label)   _combo_label->set_visible(_hud_visible && _combo_count >= 3);
    if (_interact_label) _interact_label->set_visible(_hud_visible && _prompt_showing);
    for (CanvasItem *n : _skill_bar_nodes) {
        if (n) n->set_visible(_hud_visible);
    }
    // 多 Boss 血条：按各自 alive 状态恢复/隐藏
    for (BossBarUi &b : _boss_bars) {
        bool s = _hud_visible && b.alive;
        if (b.bg) b.bg->set_visible(s);
        if (b.fill) b.fill->set_visible(s);
        if (b.name_label) b.name_label->set_visible(s);
    }
}

void GameHUD::set_hud_visible(bool p_visible) {
    _hud_visible = p_visible;
    _apply_hud_visibility();
}


// ============================================================
// Update helpers
// ============================================================

void GameHUD::_update_bar(ColorRect *p_fill, float p_current, float p_max, bool p_horizontal) {
    if (!p_fill || p_max <= 0.0f)
        return;

    float ratio = Math::clamp(p_current / p_max, 0.0f, 1.0f);
    Vector2 size = p_fill->get_size();
    if (p_horizontal) {
        size.x = BAR_WIDTH * ratio;
    } else {
        size.y = BAR_HEIGHT * ratio;
    }
    p_fill->set_size(size);
}

// ============================================================
// Callbacks
// ============================================================

void GameHUD::on_player_health_changed(float p_current, float p_max) {
    _health_current = p_current;
    _health_max = p_max;
    _update_bar(_health_fill, p_current, p_max);

    // Color shifts toward red as health decreases
    float ratio = Math::clamp(p_current / p_max, 0.0f, 1.0f);
    Color c = _health_color.lerp(Color(0.6f, 0.45f, 0.15f, 1), 1.0f - ratio);
    _health_fill->set_color(c);

    if (_health_label) {
        _health_label->set_text(
            LOC("生命 ") + String::num_int64(int(p_current)) + "/" + String::num_int64(int(p_max)));
    }
}

void GameHUD::_refresh_mana_label() {
    if (_energy_label) {
        _energy_label->set_text(
            _mana_prefix + " " + String::num_int64(int64_t(_mana_current)) +
            "/" + String::num_int64(int64_t(_mana_max)));
    }
}

void GameHUD::_refresh_xp_label() {
    if (_xp_label) {
        _xp_label->set_text(
            _xp_prefix + " " + String::num_int64(int64_t(_xp_progress * 100.0f)) + "%");
    }
}

void GameHUD::_refresh_realm_label() {
    if (_realm_label) {
        _realm_label->set_text(_realm_name);
    }
}

void GameHUD::on_spiritual_energy_changed(int64_t p_current, int64_t p_max, float p_progress) {
    // 修为经验：HUD 只显示境内进度百分比（符合修仙"进度感"）
    _xp_progress = p_progress;
    if (_xp_fill) {
        Vector2 size = _xp_fill->get_size();
        size.x = BAR_WIDTH * Math::clamp(p_progress, 0.0f, 1.0f);
        _xp_fill->set_size(size);
    }
    // 修为圆满 → 提示机缘已至（渡劫/天尊无经验条，恒满不提示）
    bool jiyuan = (p_max > 0) && p_progress >= 1.0f;
    if (_jiyuan_label)
        _jiyuan_label->set_visible(_hud_visible && jiyuan);
    _refresh_xp_label();
}

void GameHUD::on_mana_changed(double p_current, double p_max) {
    _mana_current = p_current;
    _mana_max = p_max;
    _update_bar(_energy_fill, float(p_current), float(p_max));
    _refresh_mana_label();
}

void GameHUD::on_realm_changed(int p_old_realm, int p_new_realm, const String &p_realm_name) {
    _realm_name = p_realm_name;
    _refresh_realm_label();
    if (_realm_label) {
        _realm_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
    }

    // 法力体系随境界切换：凡尘用灵力，仙级用仙元
    bool immortal = p_new_realm >= CultivationSystem::TRUE_IMMORTAL;
    String new_prefix = immortal ? LOC("仙元") : LOC("灵力");
    if (new_prefix != _mana_prefix) {
        _mana_prefix = new_prefix;
        _refresh_mana_label();
    }

    // 修为条同步：真仙+ 修为经验即仙元（九九归一），条名改显「仙元」
    String new_xp_prefix = immortal ? LOC("仙元") : LOC("修为");
    if (new_xp_prefix != _xp_prefix) {
        _xp_prefix = new_xp_prefix;
        _refresh_xp_label();
    }
}

void GameHUD::on_fullness_changed(float p_current, float p_max) {
    _fullness_current = p_current;
    _fullness_max = p_max;
    _update_bar(_fullness_fill, p_current, p_max);
    if (_fullness_label) {
        // 归零（饥饿）：标红提示
        _fullness_label->set_text(LOC("饱食 ") + String::num_int64(int64_t(p_current)) + "/" +
                                  String::num_int64(int64_t(p_max)));
        Color c = p_current <= 0.0f ? Color(1.0f, 0.35f, 0.35f, 1) : Color(1, 1, 1, 1);
        _fullness_label->add_theme_color_override("font_color", c);
        _fullness_fill->set_color(p_current <= 0.0f
            ? Color(0.75f, 0.2f, 0.2f, 1.0f) : _fullness_color);
    }
}

void GameHUD::on_bigu_changed(bool p_bigu) {
    _bigu = p_bigu;
    // 辟谷：不需要进食，隐藏饱食度条
    if (_fullness_bg) _fullness_bg->set_visible(_hud_visible && !p_bigu);
    if (_fullness_fill) _fullness_fill->set_visible(_hud_visible && !p_bigu);
    if (_fullness_label) _fullness_label->set_visible(_hud_visible && !p_bigu);
}

void GameHUD::on_ledger_inspect(const Dictionary &p_data, bool p_show) {
    if (!_ledger_overlay || _ledger_lines.size() < 5)
        return;
    _ledger_overlay->set_visible(p_show);
    if (!p_show)
        return;
    String origin = p_data.get("origin", TXT("后天修炼"));
    String body = p_data.get("original_body", TXT("凡人"));
    int ledger_life = int(p_data.get("ledger_lifespan", 0));
    int actual_life = int(p_data.get("actual_lifespan", 0));
    bool protected_ = bool(p_data.get("soul_protection", false));
    String realm = p_data.get("realm_name", String());

    if (bool(p_data.get("trial", false))) {
        // 秦广王审判叙事（初次核簿 → 放还阳/划名提示）
        _ledger_lines[0]->set_text(LOC("—— 一殿 · 秦广王 初审 ——"));
        _ledger_lines[1]->set_text(LOC("簿对：出身 ") + origin + LOC(" · 原身 ") + body);
        _ledger_lines[2]->set_text(LOC("境界 ") + realm + LOC("，簿上寿元 ") + String::num_int64(ledger_life) +
                                   LOC("，实际 ") + _lifespan_txt(actual_life));
        _ledger_lines[3]->set_text(LOC("「阳寿未绝，放还阳去；"));
        _ledger_lines[4]->set_text(LOC("  若改簿划名，永离勾魂。」"));
        for (int i = 0; i < 5; i++)
            _ledger_lines[i]->add_theme_color_override("font_color", Color(0.9f, 0.9f, 0.95f, 1));
        return;
    }

    _ledger_lines[0]->set_text(LOC("—— 生死簿 · 崔判官 ——"));
    _ledger_lines[1]->set_text(LOC("出身：") + origin + LOC("   原身：") + body);
    _ledger_lines[2]->set_text(LOC("境界：") + realm);
    _ledger_lines[3]->set_text(LOC("簿上寿元：") + String::num_int64(ledger_life) +
                               LOC("    实际：") + _lifespan_txt(actual_life));
    _ledger_lines[4]->set_text(protected_ ? LOC("名讳已划——免死一次！") : LOC("注：簿上阳寿已尽，勾魂将至"));
    // 信息差着色：实际无限（跳出五行）金 / 实际 > 簿上 绿 / 其余红
    Color c = actual_life < 0 ? Color(1.0f, 0.85f, 0.4f, 1)
            : actual_life > ledger_life ? Color(0.5f, 0.9f, 0.5f, 1) : Color(1.0f, 0.5f, 0.5f, 1);
    _ledger_lines[3]->add_theme_color_override("font_color", c);
}

void GameHUD::on_combo_changed(int p_count) {
    _combo_count = p_count;
    if (!_combo_label) return;

    if (p_count >= 3) {
        _combo_label->set_visible(_hud_visible);
        _combo_label->set_text(String::num_int64(p_count) + " HIT");

        // Color gets more intense with higher combos
        Color combo_colors[] = {
            Color(1, 1, 1, 1),       // 0-2 (not shown)
            Color(1, 0.9f, 0.5f, 1), // 3-5
            Color(1, 0.7f, 0.2f, 1), // 6-9
            Color(1, 0.4f, 0.1f, 1), // 10+
        };
        int idx = p_count >= 10 ? 3 : (p_count >= 6 ? 2 : 1);
        _combo_label->add_theme_color_override("font_color", combo_colors[idx]);

        // Pulse scale for impact
        float scale = 1.0f + Math::sin(float(p_count) * 0.5f) * 0.1f;
        _combo_label->set_scale(Vector2(scale, scale));
    } else {
        _combo_label->set_visible(false);
    }
}

void GameHUD::on_combo_ended(int p_final_count) {
    if (_combo_label && p_final_count >= 3) {
        _combo_label->set_text(String::num_int64(p_final_count) + " HIT!");
        _combo_label->set_visible(_hud_visible);
        // Will be hidden on next combo change or after a timeout
    }
}

void GameHUD::on_interaction_prompt(const String &p_text, bool p_show) {
    _prompt_showing = p_show;
    if (!_interact_label) return;
    _interact_label->set_text(p_text);
    _interact_label->set_visible(_hud_visible && p_show);
}

void GameHUD::on_player_died() {
    // 玩家阵亡即撤下全部 Boss 条（多 Boss 同场）
    for (BossBarUi &b : _boss_bars) {
        if (b.bg) b.bg->queue_free();
        if (b.fill) b.fill->queue_free();
        if (b.name_label) b.name_label->queue_free();
    }
    _boss_bars.clear();
    if (_death_overlay) {
        _death_overlay->set_visible(true);
    }
}

void GameHUD::on_player_respawned() {
    if (_death_overlay) {
        _death_overlay->set_visible(false);
    }
}

} // namespace godot
