#ifndef CPP_KAKI_DROP_SYSTEM_H
#define CPP_KAKI_DROP_SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace godot {

// DropSystem — 掉落物总控。
// 所有掉落（所有级别、所有类别的物品）都在这里定义和生成：
// 敌人死亡 → SignalBus enemy_killed → 按敌人种类 roll 掉落表 → 生成 ItemPickup。
//
// v1: 内置掉落表（按 普通/远程/飞行/Boss 分类）。
// 后续扩展方向：.tres 数据驱动掉落表、境界加成、品质稀有度系统——
// 都只需要改这一个类。
//
class DropSystem : public Node {
    GDCLASS(DropSystem, Node);

public:
    void _ready() override;

    // SignalBus enemy_killed(enemy, killer) 回调
    void _on_enemy_killed(Object *p_enemy, Object *p_killer);

    // 延迟执行的实际生成（碰撞回调里不能直接创建物理体）
    void _do_spawn_drops(const Vector2 &p_pos, bool p_is_boss,
                         bool p_is_ranged, bool p_is_flying);

    // 在指定位置生成一个掉落物（公开接口，任何系统可调用）
    void spawn_drop(const StringName &p_item_id, int p_qty, const Vector2 &p_pos);

protected:
    static void _bind_methods();

private:
    struct DropEntry {
        StringName item_id;
        int min_qty = 1;
        int max_qty = 1;
        float chance = 1.0f; // 0~1
    };

    // 按敌人种类给出本次掉落列表（v1 内置表）
    std::vector<DropEntry> _roll_drops(bool p_is_boss, bool p_is_ranged, bool p_is_flying);
};

} // namespace godot

#endif // CPP_KAKI_DROP_SYSTEM_H
