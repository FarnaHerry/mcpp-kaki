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
