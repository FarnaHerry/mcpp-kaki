#ifndef CPP_KAKI_SAFE_ZONE_H
#define CPP_KAKI_SAFE_ZONE_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class CollisionShape2D;
class Polygon2D;

// 城镇安全区（Area2D）：区内敌人失去视野（Enemy::can_see_player 抑制）、玩家缓慢休整回复。
// 查询走 "safe_zones" 组静态遍历（is_point_safe）——无共享状态，跨场景自然生效，无计数泄漏。
// 视觉：暖色地染（房屋/名牌由 WorldCommon.create_town 搭建）。
class SafeZone : public Area2D {
	GDCLASS(SafeZone, Area2D);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

	// add_child 之后调用：矩形尺寸（全宽×高，中心对齐节点）+ 城镇名
	void setup(const Vector2 &p_size, const String &p_town_name);

	// 点（全局坐标）是否落在任一安全区内
	static bool is_point_safe(const Vector2 &p_point);

	String get_town_name() const { return _town_name; }

protected:
	static void _bind_methods();

private:
	Vector2 _half = Vector2(120, 60);
	String _town_name;
	CollisionShape2D *_shape = nullptr;
	Polygon2D *_tint = nullptr;
	bool _player_inside = false;
	float _msg_t = 0.0f;

	void _rebuild_tint();
	void _say(const String &p_text, float p_secs);
};

} // namespace godot

#endif // CPP_KAKI_SAFE_ZONE_H
