#ifndef CPP_KAKI_TELEPORT_ARRAY_H
#define CPP_KAKI_TELEPORT_ARRAY_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class Player;
class Polygon2D;

// 云游阵（data/teleports.json）：各地传送阵碑。走近自动铭刻激活（GameManager flag
// "tp:<id>"，随档持久），贴近 X 打开驾云面板（TeleportPanel，GDScript）选阵点传送。
// 视觉：石碑 + 云纹（已铭刻青光 / 未铭刻暗沉）。
class TeleportArray : public Area2D {
	GDCLASS(TeleportArray, Area2D);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

	// add_child 之后调用（WC.setup 装配）
	void setup(const String &p_id, const String &p_name, const String &p_continent, const String &p_scene_path);

	String get_tp_id() const { return _id; }
	String get_tp_name() const { return _name; }
	bool is_activated() const;

protected:
	static void _bind_methods();

private:
	String _id, _name, _continent, _scene_path;
	Player *_player = nullptr;
	Polygon2D *_glow = nullptr;

	void _build_visuals();
	void _refresh_visual();
	void _update_prompt();
	void _open_panel();
};

} // namespace godot

#endif // CPP_KAKI_TELEPORT_ARRAY_H
