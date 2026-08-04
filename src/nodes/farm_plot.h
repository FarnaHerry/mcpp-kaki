#ifndef CPP_KAKI_FARM_PLOT_H
#define CPP_KAKI_FARM_PLOT_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "../utils/text.h"

namespace godot {

class Player;
class DongtianManager;

// 灵田地块（洞天 v2 种植）—— 状态在 DongtianManager，本节点只是交互+视觉。
// 空地 X 播种（背包第一种可种植草药）→ 现实时间生长 → 成熟 X 收获（种一收二）。
class FarmPlot : public Area2D {
	GDCLASS(FarmPlot, Area2D);

public:
	void set_plot_index(int p_index) { _plot_index = p_index; }
	int get_plot_index() const { return _plot_index; }

	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	int _plot_index = 0;
	Player *_player = nullptr;
	DongtianManager *_manager = nullptr;
	Polygon2D *_sprout = nullptr; // 生长中/成熟视觉（无 = 空地）
	bool _was_mature = false;
	float _prompt_refresh = 0.0f;

	DongtianManager *_find_manager();
	void _refresh_visual();
	void _update_prompt();
	void _interact();
};

} // namespace godot

#endif // CPP_KAKI_FARM_PLOT_H
