#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace godot {

class Label;
class Node2D;

// 伤害数字显示：监听 SignalBus damage_dealt，在世界坐标生成上浮淡出的数字。
// 唯一的生成入口（与 DropSystem 对掉落的关系一致）。
class DamageNumbers : public Node {
	GDCLASS(DamageNumbers, Node)

	struct Entry {
		Node2D *root = nullptr;
		float t = 0.0f;
		float drift_x = 0.0f;
	};
	std::vector<Entry> _active;
	int _spawn_counter = 0;

	static constexpr float LIFETIME = 0.8f;
	static constexpr float RISE_SPEED = 28.0f;

protected:
	static void _bind_methods();

public:
	void _ready() override;
	void _process(double p_delta) override;

	void _on_damage_dealt(Vector2 p_world_pos, float p_amount, bool p_is_player_victim);
};

} // namespace godot
