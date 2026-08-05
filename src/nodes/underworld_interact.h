#ifndef CPP_KAKI_UNDERWORLD_INTERACT_H
#define CPP_KAKI_UNDERWORLD_INTERACT_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class Player;

// 地府交互节点：判官（查生死簿）/ 生死簿（改簿划名）。
// 复用 StorageChest 交互模板（Area2D + interaction_prompt + X 轮询）。
class UnderworldInteractNode : public Area2D {
	GDCLASS(UnderworldInteractNode, Area2D);

public:
	enum Mode { MODE_INSPECT = 0, MODE_AMEND = 1 };

	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

	int get_mode() const { return _mode; }
	void set_mode(int m) { _mode = m; }

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;
	int _mode = MODE_INSPECT;
	bool _overlay_open = false;
	float _msg_t = 0.0f; // 改簿提示自动消隐

	void _update_prompt();
	void _interact();
};

} // namespace godot

#endif // CPP_KAKI_UNDERWORLD_INTERACT_H
