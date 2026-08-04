#ifndef CPP_KAKI_DONGTIAN_MANAGER_H
#define CPP_KAKI_DONGTIAN_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../utils/text.h"

namespace godot {

class Player;
class CameraRoom2D;

// 洞天系统 v1（后花园·空洞天）— design/dongtian.md
// 玩家随身小世界：炼虚解锁 dongtian 能力后，安全状态按 O 键进出。
// 进出模式复用 Portal 经验（挂子场景 + 玩家重挂载 + 相机锁定），
// 但无固定点——任意安全点进出，退出回到进入时的位置和场景。
class DongtianManager : public Node {
	GDCLASS(DongtianManager, Node);

public:
	void set_player(Player *p) { _player = p; }
	void set_camera(CameraRoom2D *c) { _camera = c; }

	bool is_inside() const { return _inside; }
	Vector2 get_return_position() const { return _saved_world_pos; }

	// 读档专用：只把玩家挪回主场景根并卸载洞天，不恢复位置（由读档回填）
	void force_exit_for_load();

	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	static constexpr const char *DONGTIAN_SCENE = "res://scenes/rooms/dongtian.tscn";
	// 云海强渡途中禁入（坠海遣返等机制与洞天往返冲突）
	static constexpr const char *YUNHAI_SCENE = "res://scenes/continents/yunhai.tscn";

	Rect2 _bounds = Rect2(0, 0, 480, 270);

	Player *_player = nullptr;
	CameraRoom2D *_camera = nullptr;
	Node *_loaded_scene = nullptr;
	bool _inside = false;
	Vector2 _saved_world_pos;

	float _hint_t = 0.0f; // 原因提示自动消隐计时

	void _try_enter();
	void _enter();
	void _exit(bool p_restore_pos);
	void _show_reason(const String &p_text);
	bool _in_combat() const;
};

} // namespace godot

#endif // CPP_KAKI_DONGTIAN_MANAGER_H
