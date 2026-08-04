#ifndef CPP_KAKI_STORAGE_CHEST_H
#define CPP_KAKI_STORAGE_CHEST_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../utils/text.h"

namespace godot {

class Player;

// 洞天仓库（v2 储物箱）—— 数据在 DongtianManager，本节点只是交互门：
// 玩家贴近显示 [X] 打开仓库，按 X 打开双栏 StoragePanel。
class StorageChest : public Area2D {
	GDCLASS(StorageChest, Area2D);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	Player *_player = nullptr;

	void _update_prompt();
	void _open_panel();
};

} // namespace godot

#endif // CPP_KAKI_STORAGE_CHEST_H
