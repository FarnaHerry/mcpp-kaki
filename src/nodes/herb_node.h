#ifndef CPP_KAKI_HERB_NODE_H
#define CPP_KAKI_HERB_NODE_H

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

class Player;
class Polygon2D;

// 草药采集点（design/alchemy.md 第二节，已定稿）：
//   - 靠近显示「[X] 采集 ·止血草」（interaction_prompt 信号，X=普攻+交互合一，交互优先）
//   - X 采集 → 入背包 + 喂练气熟练（+2，量少不刷），节点枯萎
//   - 刷新：v1 不存档，房间重进即刷新（节点随场景重建）
//
// Usage from GDScript:
//   var herb = ClassDB.instantiate("HerbNode")
//   herb.set("herb_id", "zhi_xue_cao")
//   herb.position = Vector2(300, 218)
//   add_child(herb)
class HerbNode : public Area2D {
	GDCLASS(HerbNode, Area2D);

public:
	void set_herb_id(const StringName &p_id) { _herb_id = p_id; }
	StringName get_herb_id() const { return _herb_id; }

	void set_quantity(int p_qty) { _quantity = p_qty; }
	int get_quantity() const { return _quantity; }

	bool is_harvested() const { return _harvested; }

	void _ready() override;
	void _process(double p_delta) override;
	void _on_body_entered(Node2D *p_body);
	void _on_body_exited(Node2D *p_body);

protected:
	static void _bind_methods();

private:
	StringName _herb_id;
	int _quantity = 1;
	bool _harvested = false;
	Player *_player = nullptr;
	Polygon2D *_visual = nullptr;

	void _create_visual();
	void _harvest();
};

} // namespace godot

#endif // CPP_KAKI_HERB_NODE_H
