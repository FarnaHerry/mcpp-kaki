#include "game_hud.h"

#include "../combat/skill_system.h"
#include "../cultivation/artifact_system.h"
#include "../cultivation/cultivation_system.h"
#include "player.h"
#include "../inventory/inventory.h"
#include "../inventory/item.h"
#include "../inventory/item_database.h"
#include "../utils/signal_bus.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// Layout constants (480×270 viewport)
static constexpr float BAR_WIDTH = 140.0f;
static constexpr float BAR_HEIGHT = 16.0f; // tall enough to hold the value text inside
static constexpr float BAR_X = 8.0f;
static constexpr float HEALTH_BAR_Y = 6.0f;
static constexpr float ENERGY_BAR_Y = 24.0f; // 灵力（法力）
static constexpr float XP_BAR_Y = 42.0f;     // 修为经验（百分比）
static constexpr float REALM_LABEL_Y = 62.0f;
static constexpr int FONT_SIZE_XS = 9;
static constexpr int FONT_SIZE_MD = 14;
static constexpr int FONT_SIZE_LG = 20;

void GameHUD::_bind_methods() {
    ClassDB::bind_method(D_METHOD("on_player_health_changed", "current", "max"),
                         &GameHUD::on_player_health_changed);
    ClassDB::bind_method(D_METHOD("on_buffs_changed", "active"), &GameHUD::on_buffs_changed);
    ClassDB::bind_method(D_METHOD("on_continent_changed", "id", "name"), &GameHUD::on_continent_changed);
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
    ClassDB::bind_method(D_METHOD("on_boss_fight_ended"), &GameHUD::on_boss_fight_ended);
    ClassDB::bind_method(D_METHOD("is_boss_bar_visible"), &GameHUD::is_boss_bar_visible);
    ClassDB::bind_method(D_METHOD("get_boss_bar_name"), &GameHUD::get_boss_bar_name);
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

    // Buff 行（生命条下方小行）
    _buff_label = memnew(Label);
    _buff_label->set_name("BuffLabel");
    _buff_label->set_position(Vector2(8.0f, XP_BAR_Y + BAR_HEIGHT + 2.0f));
    _buff_label->add_theme_font_size_override("font_size", 8);
    _buff_label->add_theme_color_override("font_color", Color(0.7f, 0.9f, 1.0f, 1.0f));
    _buff_label->set_visible(false);
    add_child(_buff_label);
    _create_boss_bar();

    // 洲名横幅（进入新洲时大字淡入淡出，不随 _hud_visible 隐藏——过场也要看得到）
    _continent_label = memnew(Label);
    _continent_label->set_position(Vector2(0, 78));
    _continent_label->set_size(Vector2(480, 30));
    _continent_label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
    _continent_label->add_theme_font_size_override("font_size", 18);
    _continent_label->add_theme_color_override("font_color", Color(1.0f, 0.92f, 0.6f, 1.0f));
    _continent_label->add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9f));
    _continent_label->add_theme_constant_override("outline_size", 4);
    _continent_label->set_visible(false);
    add_child(_continent_label);

    set_process_unhandled_input(true);
    set_process(true); // 技能栏冷却轮询

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
               _health_bg, _health_fill, _health_label, TXT("生命 100/100"));
    _health_bg->set_name("HealthBg");
    _health_fill->set_name("HealthFill");
    _health_label->set_name("HealthLabel");
}

void GameHUD::_create_energy_bar() {
    _build_bar(this, ENERGY_BAR_Y, _energy_color,
               _energy_bg, _energy_fill, _energy_label, TXT("灵力 0/0"));
    _energy_bg->set_name("ManaBg");
    _energy_fill->set_name("ManaFill");
    _energy_label->set_name("ManaLabel");
    _energy_fill->set_size(Vector2(0, BAR_HEIGHT)); // starts empty
}

void GameHUD::_create_xp_bar() {
    _build_bar(this, XP_BAR_Y, _xp_color,
               _xp_bg, _xp_fill, _xp_label, TXT("修为 0%"));
    _xp_bg->set_name("XpBg");
    _xp_fill->set_name("XpFill");
    _xp_label->set_name("XpLabel");
    _xp_fill->set_size(Vector2(0, BAR_HEIGHT)); // starts empty
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
    _realm_label->set_text(TXT("凡人"));
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
    _jiyuan_label->set_text(TXT("机缘已至 [Q]"));
    _jiyuan_label->set_visible(false);
    add_child(_jiyuan_label);
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
    // DNF 式布局预留：武技(A/S) 法术(D/F) 法宝(G/H)，空槽暗框 + 键位标签。
    // 技能系统落地后：槽内填图标/冷却扫层，此处只负责槽位与可见性。
    struct SlotSpec { const char *caption; const char *key; Color tint; };
    static const SlotSpec SLOTS[] = {
        { "武技", "A", Color(0.55f, 0.25f, 0.20f, 0.85f) },
        { nullptr, "S", Color(0.55f, 0.25f, 0.20f, 0.85f) },
        { "法术", "D", Color(0.20f, 0.35f, 0.60f, 0.85f) },
        { nullptr, "F", Color(0.20f, 0.35f, 0.60f, 0.85f) },
        { "法宝", "G", Color(0.55f, 0.45f, 0.15f, 0.85f) },
        { nullptr, "H", Color(0.55f, 0.45f, 0.15f, 0.85f) },
        { "神通", "T", Color(0.40f, 0.20f, 0.55f, 0.85f) },
        { "仙法", "Y", Color(0.60f, 0.55f, 0.25f, 0.85f) },
    };
    const float SLOT_W = 20.0f, SLOT_GAP = 2.0f, GROUP_GAP = 10.0f;
    const int GROUP_SIZE = 2, N = 8;
    const float total_w = N * SLOT_W + (N - 1) * SLOT_GAP + 3 * GROUP_GAP; // 204
    const float x0 = (480.0f - total_w) * 0.5f;
    const float y = 270.0f - 24.0f;

    for (int i = 0; i < N; i++) {
        float x = x0 + i * (SLOT_W + SLOT_GAP) + (i / GROUP_SIZE) * GROUP_GAP;

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
        name->set_text(TXT("·"));
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

        if (SLOTS[i].caption) {
            Label *cap = memnew(Label);
            cap->set_text(TXT(SLOTS[i].caption));
            cap->add_theme_font_size_override("font_size", 7);
            cap->add_theme_color_override("font_color", Color(0.6f, 0.6f, 0.6f, 0.9f));
            cap->set_position(Vector2(x + 2, y - 10));
            add_child(cap);
            _skill_bar_nodes.push_back(cap);
        }
    }

    // 法宝页徽标（B 切换；仅法宝页显示）
    _page_badge = memnew(Label);
    _page_badge->set_text(TXT("法宝页 [B 返回]"));
    _page_badge->add_theme_font_size_override("font_size", 8);
    _page_badge->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.4f, 0.95f));
    _page_badge->set_position(Vector2(x0 + total_w + 8.0f, y + 6.0f));
    _page_badge->set_visible(false);
    add_child(_page_badge);
    _skill_bar_nodes.push_back(_page_badge);
}

// ============================================================
// 技能栏刷新（轮询 Player->SkillSystem；冷却走 Player._time 时基）
// ============================================================

void GameHUD::_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint())
        return;
    _update_skill_bar();
    _update_consumable_bar();
    _update_law_bar();
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
    _continent_label->set_text(TXT("—— ") + p_name + TXT(" ——"));
    Color c = _continent_label->get_theme_color("font_color");
    c.a = 1.0f;
    _continent_label->add_theme_color_override("font_color", c);
    _continent_label->set_visible(true);
    _continent_banner_t = 2.8f;
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
        text += String(d["name"]) + " " + String::num_int64((int64_t)Math::ceil(float(d["remaining"]))) + "s";
    }
    _buff_label->set_text(text);
    _buff_label->set_visible(_hud_visible);
}

// ============================================================
// 法则之力条（右上角；化神解锁后显示）
// ============================================================

void GameHUD::_create_law_bar() {
    const float x = 480.0f - BAR_WIDTH - 8.0f, y = 6.0f, h = 10.0f;
    _law_bg = memnew(ColorRect);
    _law_bg->set_position(Vector2(x, y));
    _law_bg->set_size(Vector2(BAR_WIDTH, h));
    _law_bg->set_color(Color(0.12f, 0.08f, 0.18f, 0.85f));
    add_child(_law_bg);

    _law_fill = memnew(ColorRect);
    _law_fill->set_position(Vector2(x, y));
    _law_fill->set_size(Vector2(0, h));
    _law_fill->set_color(Color(0.65f, 0.40f, 0.95f, 1.0f));
    add_child(_law_fill);

    _law_label = memnew(Label);
    _law_label->set_text(TXT("法则"));
    _law_label->add_theme_font_size_override("font_size", FONT_SIZE_XS);
    _law_label->add_theme_color_override("font_color", Color(0.85f, 0.70f, 1.0f, 0.95f));
    _law_label->set_position(Vector2(x + 2, y - 1));
    add_child(_law_label);

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
    _law_bg->set_visible(show);
    _law_fill->set_visible(show);
    _law_label->set_visible(show);
    if (!show)
        return;
    double cur = cult->get_law_power();
    _law_fill->set_size(Vector2(BAR_WIDTH * float(cur / max_law), 10.0f));
    _law_label->set_text(TXT("法则 ") + String::num_int64(int64_t(cur)) + TXT("/") + String::num_int64(int64_t(max_law)));
}

// 数字键消耗品栏（技能栏上方一行：1~6 槽，名首字+数量；design/alchemy.md S6）
void GameHUD::_create_consumable_bar() {
    const float SLOT_W = 20.0f, SLOT_GAP = 2.0f;
    const int N = 6;
    const float total_w = N * SLOT_W + (N - 1) * SLOT_GAP; // 130
    const float x0 = (480.0f - total_w) * 0.5f;
    const float y = 270.0f - 24.0f - 23.0f; // 技能栏上方

    for (int i = 0; i < N; i++) {
        float x = x0 + i * (SLOT_W + SLOT_GAP);

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
        name->set_text(TXT("·"));
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
            _bar_name_labels[i]->set_text(TXT("·"));
            _bar_name_labels[i]->add_theme_color_override("font_color", Color(1, 1, 1, 0.35f));
            _bar_count_labels[i]->set_text("");
            continue;
        }
        const Item *def = db ? db->get_item(id) : nullptr;
        String name = def ? def->name : String(id);
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
    // 槽 i 的数据源：法宝页且 i<6 → 法宝槽；其余 → 技能槽（T/Y 两页通用）
    SkillSystem *skills = _player->get_skills();
    ArtifactSystem *arts = _player->get_artifacts();
    for (int i = 0; i < (int)_skill_name_labels.size(); i++) {
        bool artifact_side = (page == 1 && i < 6);
        Dictionary info = artifact_side
            ? (arts ? arts->get_slot_info(i) : Dictionary())
            : (skills ? skills->get_slot_info(i) : Dictionary());
        if (info.is_empty() || info.has("locked")) {
            _skill_name_labels[i]->set_text(info.has("locked") ? TXT("×") : TXT("·"));
            _skill_cd_labels[i]->set_text("");
            continue;
        }
        String name = info.get("name", "");
        _skill_name_labels[i]->set_text(name.is_empty() ? TXT("·") : name.substr(0, 1));
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

void GameHUD::_create_boss_bar() {
    const float W = 240.0f, H = 8.0f;
    const float x = (480.0f - W) * 0.5f, y = 8.0f;

    _boss_bg = memnew(ColorRect);
    _boss_bg->set_position(Vector2(x, y));
    _boss_bg->set_size(Vector2(W, H));
    _boss_bg->set_color(Color(0.10f, 0.06f, 0.06f, 0.88f));
    add_child(_boss_bg);

    _boss_fill = memnew(ColorRect);
    _boss_fill->set_position(Vector2(x, y));
    _boss_fill->set_size(Vector2(W, H));
    _boss_fill->set_color(Color(0.80f, 0.12f, 0.12f, 1.0f));
    add_child(_boss_fill);

    _boss_name = memnew(Label);
    _boss_name->set_text("");
    _boss_name->add_theme_font_size_override("font_size", FONT_SIZE_XS);
    _boss_name->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.75f, 0.95f));
    _boss_name->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    _boss_name->set_position(Vector2(x, y + H + 1));
    _boss_name->set_size(Vector2(W, 12));
    add_child(_boss_name);

    _boss_bg->set_visible(false);
    _boss_fill->set_visible(false);
    _boss_name->set_visible(false);
}

void GameHUD::on_boss_fight_update(const String &p_name, double p_current, double p_max) {
    if (!_boss_bg || p_max <= 0.0)
        return;
    float frac = Math::clamp(float(p_current / p_max), 0.0f, 1.0f);
    _boss_fill->set_size(Vector2(240.0f * frac, 8.0f));
    _boss_name->set_text(p_name);
    bool show = _hud_visible && p_current > 0.0;
    _boss_bg->set_visible(show);
    _boss_fill->set_visible(show);
    _boss_name->set_visible(show);
}

void GameHUD::on_boss_fight_ended() {
    if (!_boss_bg)
        return;
    _boss_bg->set_visible(false);
    _boss_fill->set_visible(false);
    _boss_name->set_visible(false);
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
    // Combo/prompt manage their own visibility; only show when HUD is on
    if (_combo_label)   _combo_label->set_visible(_hud_visible && _combo_count >= 3);
    if (_interact_label) _interact_label->set_visible(_hud_visible && _prompt_showing);
    for (CanvasItem *n : _skill_bar_nodes) {
        if (n) n->set_visible(_hud_visible);
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
            TXT("生命 ") + String::num_int64(int(p_current)) + "/" + String::num_int64(int(p_max)));
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
            TXT("修为 ") + String::num_int64(int64_t(_xp_progress * 100.0f)) + "%");
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
    String new_prefix = p_new_realm >= CultivationSystem::TRUE_IMMORTAL
        ? TXT("仙元") : TXT("灵力");
    if (new_prefix != _mana_prefix) {
        _mana_prefix = new_prefix;
        _refresh_mana_label();
    }
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
    on_boss_fight_ended(); // 玩家阵亡即撤下 Boss 条
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
