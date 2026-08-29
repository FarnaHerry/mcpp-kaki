#ifndef CPP_KAKI_TOWN_NPC_H
#define CPP_KAKI_TOWN_NPC_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class Player;

// 城镇 NPC（交互模板同 ShopKeeper）：贴近 → "[X] 交谈/歇息"。
// X 循环播放对话气泡（头顶，2.5s 自消）；heal 型（客栈掌柜/仙娥）改为全恢复 HP+灵力。
class TownNpc : public Area2D {
	GDCLASS(TownNpc, Area2D);

public:
	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

	// add_child 之后调用（世界脚本 create_town 装配）
	void setup(const String &p_npc_name, const Color &p_robe, const PackedStringArray &p_lines, bool p_heal);

	String get_npc_name() const { return _npc_name; }
	bool is_healer() const { return _heal; }
	String get_bubble_text() const; // 测试探针：当前气泡文本（空=未显示）

protected:
	static void _bind_methods();

private:
	String _npc_name;
	Color _robe = Color(0.5f, 0.5f, 0.62f);
	PackedStringArray _lines;
	int _line_idx = 0;
	bool _heal = false;

	Player *_player = nullptr;
	Label *_bubble = nullptr;
	ColorRect *_bubble_bg = nullptr;
	float _bubble_t = 0.0f;

	void _build_visuals();
	void _update_prompt();
	void _interact();
	void _say_bubble(const String &p_text);
};

} // namespace godot

#endif // CPP_KAKI_TOWN_NPC_H
