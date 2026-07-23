#include "game_hud.h"

#include "../utils/signal_bus.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Layout constants (480×270 viewport)
static constexpr float BAR_WIDTH = 100.0f;
static constexpr float BAR_HEIGHT = 8.0f;
static constexpr float BAR_X = 8.0f;
static constexpr float HEALTH_BAR_Y = 8.0f;
static constexpr float ENERGY_BAR_Y = 22.0f;
static constexpr int FONT_SIZE_SM = 10;
static constexpr int FONT_SIZE_MD = 14;
static constexpr int FONT_SIZE_LG = 20;

void GameHUD::_bind_methods() {
    ClassDB::bind_method(D_METHOD("on_player_health_changed", "current", "max"),
                         &GameHUD::on_player_health_changed);
    ClassDB::bind_method(D_METHOD("on_spiritual_energy_changed", "current", "max"),
                         &GameHUD::on_spiritual_energy_changed);
    ClassDB::bind_method(D_METHOD("on_realm_changed", "old_realm", "new_realm", "realm_name"),
                         &GameHUD::on_realm_changed);
    ClassDB::bind_method(D_METHOD("on_combo_changed", "count"), &GameHUD::on_combo_changed);
    ClassDB::bind_method(D_METHOD("on_combo_ended", "final_count"), &GameHUD::on_combo_ended);
    ClassDB::bind_method(D_METHOD("on_interaction_prompt", "text", "show"),
                         &GameHUD::on_interaction_prompt);
    ClassDB::bind_method(D_METHOD("on_player_died"), &GameHUD::on_player_died);
    ClassDB::bind_method(D_METHOD("on_player_respawned"), &GameHUD::on_player_respawned);
}

void GameHUD::_ready() {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    set_layer(100); // Topmost UI layer

    _create_health_bar();
    _create_energy_bar();
    _create_realm_label();
    _create_combo_label();
    _create_interact_prompt();
    _create_death_overlay();

    // Connect to SignalBus
    SignalBus *bus = SignalBus::get_singleton();
    if (bus) {
        bus->connect("player_health_changed", Callable(this, "on_player_health_changed"));
        bus->connect("spiritual_energy_changed", Callable(this, "on_spiritual_energy_changed"));
        bus->connect("realm_changed", Callable(this, "on_realm_changed"));
        bus->connect("combo_changed", Callable(this, "on_combo_changed"));
        bus->connect("combo_ended", Callable(this, "on_combo_ended"));
        bus->connect("interaction_prompt", Callable(this, "on_interaction_prompt"));
        bus->connect("player_died", Callable(this, "on_player_died"));
        bus->connect("player_respawned", Callable(this, "on_player_respawned"));
    }
}

// ============================================================
// Health Bar
// ============================================================

void GameHUD::_create_health_bar() {
    // Background
    _health_bg = memnew(ColorRect);
    _health_bg->set_name("HealthBg");
    _health_bg->set_position(Vector2(BAR_X, HEALTH_BAR_Y));
    _health_bg->set_size(Vector2(BAR_WIDTH, BAR_HEIGHT));
    _health_bg->set_color(Color(0.15f, 0.15f, 0.15f, 0.8f));
    add_child(_health_bg);

    // Fill
    _health_fill = memnew(ColorRect);
    _health_fill->set_name("HealthFill");
    _health_fill->set_position(Vector2(BAR_X, HEALTH_BAR_Y));
    _health_fill->set_size(Vector2(BAR_WIDTH, BAR_HEIGHT));
    _health_fill->set_color(_health_color);
    add_child(_health_fill);

    // Label
    _health_label = memnew(Label);
    _health_label->set_name("HealthLabel");
    _health_label->set_position(Vector2(BAR_X, HEALTH_BAR_Y + BAR_HEIGHT + 1));
    _health_label->add_theme_font_size_override("font_size", FONT_SIZE_SM);
    _health_label->add_theme_color_override("font_color", Color(1, 1, 1, 1));
    _health_label->set_text("HP 100/100");
    add_child(_health_label);
}

// ============================================================
// Energy Bar
// ============================================================

void GameHUD::_create_energy_bar() {
    // Background
    _energy_bg = memnew(ColorRect);
    _energy_bg->set_name("EnergyBg");
    _energy_bg->set_position(Vector2(BAR_X, ENERGY_BAR_Y));
    _energy_bg->set_size(Vector2(BAR_WIDTH, BAR_HEIGHT));
    _energy_bg->set_color(Color(0.15f, 0.15f, 0.15f, 0.8f));
    add_child(_energy_bg);

    // Fill
    _energy_fill = memnew(ColorRect);
    _energy_fill->set_name("EnergyFill");
    _energy_fill->set_position(Vector2(BAR_X, ENERGY_BAR_Y));
    _energy_fill->set_size(Vector2(0, BAR_HEIGHT)); // starts empty
    _energy_fill->set_color(_energy_color);
    add_child(_energy_fill);

    // Label
    _energy_label = memnew(Label);
    _energy_label->set_name("EnergyLabel");
    _energy_label->set_position(Vector2(BAR_X, ENERGY_BAR_Y + BAR_HEIGHT + 1));
    _energy_label->add_theme_font_size_override("font_size", FONT_SIZE_SM);
    _energy_label->add_theme_color_override("font_color", Color(0.7f, 0.85f, 1.0f, 1));
    _energy_label->set_text("气 0/80");
    add_child(_energy_label);
}

// ============================================================
// Realm Label
// ============================================================

void GameHUD::_create_realm_label() {
    _realm_label = memnew(Label);
    _realm_label->set_name("RealmLabel");
    _realm_label->set_position(Vector2(BAR_X, 40));
    _realm_label->add_theme_font_size_override("font_size", FONT_SIZE_MD);
    _realm_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
    _realm_label->set_text("凡人");
    add_child(_realm_label);
}

// ============================================================
// Combo Counter
// ============================================================

void GameHUD::_create_combo_label() {
    _combo_label = memnew(Label);
    _combo_label->set_name("ComboLabel");
    _combo_label->set_position(Vector2(400, 20));
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
            "HP " + String::num_int64(int(p_current)) + "/" + String::num_int64(int(p_max)));
    }
}

void GameHUD::on_spiritual_energy_changed(float p_current, float p_max) {
    _energy_current = p_current;
    _energy_max = p_max;
    _update_bar(_energy_fill, p_current, p_max);

    if (_energy_label) {
        _energy_label->set_text(
            String::utf8("气 ") + String::num_int64(int(p_current)) + "/" + String::num_int64(int(p_max)));
    }
}

void GameHUD::on_realm_changed(int p_old_realm, int p_new_realm, const String &p_realm_name) {
    if (_realm_label) {
        _realm_label->set_text(p_realm_name);
        _realm_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.3f, 1));
    }
}

void GameHUD::on_combo_changed(int p_count) {
    _combo_count = p_count;
    if (!_combo_label) return;

    if (p_count >= 3) {
        _combo_label->set_visible(true);
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
        _combo_label->set_visible(true);
        // Will be hidden on next combo change or after a timeout
    }
}

void GameHUD::on_interaction_prompt(const String &p_text, bool p_show) {
    if (!_interact_label) return;
    _interact_label->set_text(p_text);
    _interact_label->set_visible(p_show);
}

void GameHUD::on_player_died() {
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
