#include "telemetry_panel.h"

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

static constexpr float VIEWPORT_W = 480.0f;
static constexpr int FONT_SIZE_SM = 10;
static constexpr double UPDATE_INTERVAL = 0.2; // seconds between refreshes

void TelemetryPanel::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_player", "player"), &TelemetryPanel::set_player);
    ClassDB::bind_method(D_METHOD("set_telemetry_visible", "visible"),
                         &TelemetryPanel::set_telemetry_visible);
    ClassDB::bind_method(D_METHOD("is_telemetry_visible"),
                         &TelemetryPanel::is_telemetry_visible);
}

void TelemetryPanel::_ready() {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    set_layer(101); // Above GameHUD (100)

    _label = memnew(Label);
    _label->set_name("TelemetryLabel");
    _label->set_position(Vector2(VIEWPORT_W - 210.0f, 8.0f));
    _label->set_size(Vector2(202.0f, 200.0f));
    _label->add_theme_font_size_override("font_size", FONT_SIZE_SM);
    _label->add_theme_color_override("font_color", Color(1, 1, 1, 1));
    _label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
    add_child(_label);

    set_process(true);
    set_process_unhandled_input(true);
}

void TelemetryPanel::_update() {
    if (!_label)
        return;

    String txt = "FPS:" + String::num_int64(Engine::get_singleton()->get_frames_per_second());

    if (_player) {
        Node *player_node = Object::cast_to<Node>(_player);
        Vector2 pos;
        Vector2 vel = _player->get("velocity");
        if (player_node && player_node->is_class("Node2D")) {
            pos = _player->get("position");
        }
        txt += "\nPos:(" + String::num_int64(int64_t(pos.x)) + "," +
               String::num_int64(int64_t(pos.y)) + ")";
        txt += " Vel:(" + String::num_int64(int64_t(vel.x)) + "," +
               String::num_int64(int64_t(vel.y)) + ")";
        bool on_floor = _player->call("is_on_floor");
        bool on_wall = _player->call("is_on_wall");
        txt += String("\nFloor:") + (on_floor ? "Y" : "N") +
               "  Wall:" + (on_wall ? "Y" : "N");

        Object *cult = _player->call("get_cultivation");
        if (cult) {
            txt += TXT("\n") + String(cult->call("get_full_title"));
            float progress = cult->call("get_realm_progress");
            txt += TXT("\n修为: ") + String::num_int64(int64_t(progress * 100.0f)) + "%";
            txt += TXT("\n") + String(cult->call("get_mana_name")) + ": " +
                   String::num_int64(int64_t(double(cult->call("get_mana")))) + "/" +
                   String::num_int64(int64_t(double(cult->call("get_max_mana"))));
            String focus = cult->call("get_focus_name");
            if (!focus.is_empty()) {
                txt += TXT("\n功法: ") + focus;
            }
        }

        Object *inv = _player->call("get_inventory");
        if (inv) {
            int64_t cap = inv->call("get_capacity");
            int64_t item_count = 0;
            for (int64_t i = 0; i < cap; i++) {
                Dictionary slot = inv->call("get_slot", i);
                if (!slot.is_empty()) {
                    item_count += int64_t(slot.get("quantity", 0));
                }
            }
            txt += "\nItems:" + String::num_int64(item_count);
        }
    }

    // GameManager stats (sibling node created by bootstrap)
    Node *gm = get_node_or_null(NodePath("../GameManager"));
    if (gm) {
        txt += "\nKills:" + String::num_int64(int64_t(gm->call("get_kill_count")));
        bool has_save = gm->call("has_save", "auto");
        txt += has_save ? "  Save:OK" : "  Save:--";
    }

    _label->set_text(txt);
}

void TelemetryPanel::_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint())
        return;
    if (!_telemetry_visible || !_label)
        return;

    _accum += p_delta;
    if (_accum >= UPDATE_INTERVAL) {
        _accum = 0.0;
        _update();
    }
}

void TelemetryPanel::_unhandled_input(const Ref<InputEvent> &p_event) {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    Ref<InputEventKey> key = p_event;
    if (key.is_null() || !key->is_pressed() || key->is_echo())
        return;

    switch (key->get_keycode()) {
        case KEY_F3: // Toggle telemetry
            set_telemetry_visible(!_telemetry_visible);
            break;
        case KEY_F5: // Debug: toggle free breakthrough (no XP requirement)
            if (_player) {
                Object *cult = _player->call("get_cultivation");
                if (cult) {
                    bool cur = cult->call("is_free_breakthrough");
                    cult->call("set_free_breakthrough", !cur);
                    UtilityFunctions::print(String("[DEBUG] free_breakthrough = ") +
                        (!cur ? TXT("ON (突破无经验门槛)") : TXT("OFF (需要经验圆满)")));
                }
            }
            break;
        default:
            break;
    }
}

void TelemetryPanel::set_telemetry_visible(bool p_visible) {
    _telemetry_visible = p_visible;
    if (_label) {
        _label->set_visible(p_visible);
    }
    if (p_visible) {
        _accum = UPDATE_INTERVAL; // refresh immediately on re-enable
    }
}

} // namespace godot
