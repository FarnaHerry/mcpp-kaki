#ifndef CPP_KAKI_TELEMETRYPANEL_H
#define CPP_KAKI_TELEMETRYPANEL_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

namespace godot {

class Player;

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
	Object *_player = nullptr;
	double _accum = 0.0;
	bool _telemetry_visible = true;

	void _update();
};

} // namespace godot

#endif // CPP_KAKI_TELEMETRYPANEL_H
