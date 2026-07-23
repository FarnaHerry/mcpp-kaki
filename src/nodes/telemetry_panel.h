#ifndef CPP_KAKI_TELEMETRY_PANEL_H
#define CPP_KAKI_TELEMETRY_PANEL_H

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>

namespace godot {

// Telemetry / debug readout (top-right, screen-fixed).
// Independent from GameHUD so debug UI never entangles gameplay UI.
// Shows: FPS, position/velocity, floor/wall, title, cultivation progress,
// mana, item count, kills, save status.
//
// F3 toggles visibility. Data source: set_player() from bootstrap.
//
class TelemetryPanel : public CanvasLayer {
    GDCLASS(TelemetryPanel, CanvasLayer);

public:
    void _ready() override;
    void _process(double p_delta) override;
    void _unhandled_input(const Ref<InputEvent> &p_event) override;

    void set_player(Object *p_player) { _player = p_player; }

    void set_telemetry_visible(bool p_visible);
    bool is_telemetry_visible() const { return _telemetry_visible; }

protected:
    static void _bind_methods();

private:
    Label *_label = nullptr;
    Object *_player = nullptr; // not owned
    double _accum = 0.0;
    bool _telemetry_visible = true;

    void _update();
};

} // namespace godot

#endif // CPP_KAKI_TELEMETRY_PANEL_H
