#ifndef CPP_KAKI_ABILITY_MANAGER_H
#define CPP_KAKI_ABILITY_MANAGER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

    class CultivationSystem;

    class AbilityManager : public Object {
        GDCLASS(AbilityManager, Object);

    public:
        static const char *ABILITY_DOUBLE_JUMP;
        static const char *ABILITY_WALL_CLING;
        static const char *ABILITY_DASH;
        static const char *ABILITY_AIR_DASH;
        static const char *ABILITY_SPIRIT_VISION;
        static const char *ABILITY_GLIDE;
        // 炼气：纳戒（储物无限）
        static const char *ABILITY_STORAGE_RING;
        // 筑基：短暂飞行 / 金丹：自主飞行
        static const char *ABILITY_SHORT_FLIGHT;
        static const char *ABILITY_FREE_FLIGHT;
        // 元婴期
        static const char *ABILITY_SOUL_EXIT;
        static const char *ABILITY_DOMAIN;
        // 化神期
        static const char *ABILITY_SPIRIT_TRAVEL;
        static const char *ABILITY_SPIRIT_SENSE;
        // 炼虚
        static const char *ABILITY_VOID_SHIFT;
        // 合体
        static const char *ABILITY_UNITY_FORM;
        // 大乘
        static const char *ABILITY_MERIT_HALO;
        // 真仙（含渡劫成功奖励：免疫凡间雷火风）
        static const char *ABILITY_CLOUD_FLIGHT;
        static const char *ABILITY_TRIBULATION_IMMUNITY;
        // 金仙
        static const char *ABILITY_GIANT_FORM;
        static const char *ABILITY_GOLDEN_BODY;
        // 混元一气
        static const char *ABILITY_DAO_DOMAIN;
        static const char *ABILITY_MYRIAD_AVATARS;

        AbilityManager();

        void set_cultivation(CultivationSystem *p_cultivation) { _cultivation = p_cultivation; }
        void unlock_ability(const StringName &p_ability_id);
        bool has_ability(const StringName &p_ability_id) const;
        void check_realm_unlocks();
        String get_unlocked_list() const;

    protected:
        static void _bind_methods();

    private:
        HashSet<StringName> _unlocked;
        CultivationSystem *_cultivation = nullptr;
    };

} // namespace godot

#endif // CPP_KAKI_ABILITY_MANAGER_H
