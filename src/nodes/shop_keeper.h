#ifndef CPP_KAKI_SHOP_KEEPER_H
#define CPP_KAKI_SHOP_KEEPER_H

#include <godot_cpp/classes/area2d.hpp>

namespace godot {

class Player;

// 商店掌柜 NPC：长安坊市交易（StorageChest 交互模板）。
// 贴近 → "[X] 交易" → 打开 ShopPanel。
class ShopKeeper : public Area2D {
	GDCLASS(ShopKeeper, Area2D);

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

#endif // CPP_KAKI_SHOP_KEEPER_H
