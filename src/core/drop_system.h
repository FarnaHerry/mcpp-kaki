#ifndef CPP_KAKI_DROP_SYSTEM_H
#define CPP_KAKI_DROP_SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace godot {

// DropSystem — 掉落物总控。
// 所有掉落（所有级别、所有类别的物品）都在这里定义和生成：
// 敌人死亡 → SignalBus enemy_killed → 按敌人种类/命名表 roll 掉落表 → 生成 ItemPickup。
//
// v2（session 020）：命名掉落表 + 境界门槛 + 精英奖励掉落。
// - 敌人可带 drop_table 属性（String，空=类别兜底）指向 drops.json "tables" 里的命名表
//   （如 you_gu_chi_long / xuan_ming），命名表找不到时回落类别兜底（不许空掉）。
// - 条目带 min_realm（默认 0）：敌人 realm < min_realm 时该条跳过（高境怪才掉好货）。
// - SignalBus elite_killed(pos, tier, realm) → roll "elite" 表 tier 次（封顶 3）追加奖励掉落。
// - drops.json v2 顶层为 {"tables": {...}}；老格式平铺（顶层直接 boss/normal/...）兼容保留。
//
class DropSystem : public Node {
    GDCLASS(DropSystem, Node);

public:
    void _ready() override;

    // SignalBus enemy_killed(enemy, killer) 回调
    void _on_enemy_killed(Object *p_enemy, Object *p_killer);

    // SignalBus elite_killed(pos, tier, realm) 回调（精英怪奖励掉落）
    void _on_elite_killed(const Vector2 &p_pos, int p_tier, int p_realm);

    // 延迟执行的实际生成（碰撞回调里不能直接创建物理体）
    void _do_spawn_drops(const Vector2 &p_pos, const String &p_drop_table, bool p_is_boss,
                         bool p_is_ranged, bool p_is_flying, int p_realm);

    // 精英奖励：roll "elite" 表 tier 次（1~3），每次独立 roll
    void _do_spawn_elite_drops(const Vector2 &p_pos, int p_tier, int p_realm);

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
        int min_realm = 0;   // 敌人境界门槛：realm < min_realm 跳过该条
    };

    // 按敌人种类/命名表给出本次掉落列表
    std::vector<DropEntry> _roll_drops(const String &p_drop_table, bool p_is_boss,
                                       bool p_is_ranged, bool p_is_flying, int p_realm);

    // 精英奖励表（"elite"）roll 一次
    std::vector<DropEntry> _roll_elite_drops(int p_realm);

    // 从 DataLoader 读一张表（兼容 v2 "tables" 包裹与老格式平铺），不过滤不 roll。
    // 返回 false = 表缺失/为空（调用方回落）。
    bool _load_table(const String &p_key, std::vector<DropEntry> &p_out) const;

    // min_realm 过滤 + chance roll
    static std::vector<DropEntry> _filter_and_roll(const std::vector<DropEntry> &p_table, int p_realm);

    // 硬编码兜底表（JSON 不可用时）
    static std::vector<DropEntry> _fallback_category_table(bool p_is_boss, bool p_is_ranged, bool p_is_flying);
    static std::vector<DropEntry> _fallback_elite_table();
};

} // namespace godot

#endif // CPP_KAKI_DROP_SYSTEM_H
