#ifndef CPP_KAKI_GAME_HUD_H
#define CPP_KAKI_GAME_HUD_H

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

// In-game HUD rendered via CanvasLayer.
// Listens to SignalBus for health, energy, realm, combo, and interaction events.
//
// Layout (480×270 viewport):
//   Top-left:     [Health bar] [Energy bar] [Realm label]
//   Right:        Combo counter
//   Bottom-center: Interaction prompt
//
class GameHUD : public CanvasLayer {
    GDCLASS(GameHUD, CanvasLayer);

public:
    void _ready() override;

    // Callbacks connected to SignalBus
    void on_player_health_changed(float p_current, float p_max);
    void on_spiritual_energy_changed(float p_current, float p_max);
    void on_realm_changed(int p_old_realm, int p_new_realm, const String &p_realm_name);
    void on_combo_changed(int p_count);
    void on_combo_ended(int p_final_count);
    void on_interaction_prompt(const String &p_text, bool p_show);
    void on_player_died();
    void on_player_respawned();

    // Configuration
    void set_health_bar_color(const Color &p_color) { _health_color = p_color; }
    Color get_health_bar_color() const { return _health_color; }
    void set_energy_bar_color(const Color &p_color) { _energy_color = p_color; }
    Color get_energy_bar_color() const { return _energy_color; }

protected:
    static void _bind_methods();

private:
    // Health bar
    ColorRect *_health_bg = nullptr;
    ColorRect *_health_fill = nullptr;
    Label *_health_label = nullptr;
    float _health_current = 100.0f;
    float _health_max = 100.0f;
    Color _health_color = Color(0.85f, 0.2f, 0.2f, 1.0f);

    // Spiritual energy bar
    ColorRect *_energy_bg = nullptr;
    ColorRect *_energy_fill = nullptr;
    Label *_energy_label = nullptr;
    float _energy_current = 0.0f;
    float _energy_max = 100.0f;
    Color _energy_color = Color(0.2f, 0.6f, 1.0f, 1.0f);

    // Realm label
    Label *_realm_label = nullptr;

    // Combo
    Label *_combo_label = nullptr;
    int _combo_count = 0;

    // Interaction prompt
    Label *_interact_label = nullptr;

    // Death overlay
    ColorRect *_death_overlay = nullptr;

    void _create_health_bar();
    void _create_energy_bar();
    void _create_realm_label();
    void _create_combo_label();
    void _create_interact_prompt();
    void _create_death_overlay();
    void _update_bar(ColorRect *p_fill, float p_current, float p_max, bool p_horizontal = true);
};

} // namespace godot

#endif // CPP_KAKI_GAME_HUD_H
