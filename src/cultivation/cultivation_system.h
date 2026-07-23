#ifndef CPP_KAKI_CULTIVATION_SYSTEM_H
#define CPP_KAKI_CULTIVATION_SYSTEM_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    struct RealmDefinition {
        String name;
        float energy_to_advance = 100.0f;
        float breakthrough_chance = 0.6f;
        float damage_mult = 1.0f;
        float defense_mult = 1.0f;
        float speed_mult = 1.0f;
    };

    class CultivationSystem : public Object {
        GDCLASS(CultivationSystem, Object);

    public:
        // Realm enum
        enum Realm {
            MORTAL = 0,          // 凡人
            QI_REFINING = 1,     // 炼气期
            FOUNDATION = 2,      // 筑基期
            GOLDEN_CORE = 3,     // 金丹期
            NASCENT_SOUL = 4,    // 元婴期
            SPIRIT_SEVERING = 5  // 化神期
        };

        static const int REALM_COUNT = 6;

        CultivationSystem();

        // Current state
        Realm get_current_realm() const { return _current_realm; }
        String get_realm_name() const;
        int get_realm_index() const { return (int)_current_realm; }

        float get_spiritual_energy() const { return _spiritual_energy; }
        float get_max_energy() const;

        // Accumulate spiritual energy (from kills, meditation, pills)
        void accumulate_energy(float p_amount);

        // Attempt to break through to the next realm
        bool attempt_breakthrough();

        // Stat multipliers based on current realm
        float get_damage_multiplier() const;
        float get_defense_multiplier() const;
        float get_speed_multiplier() const;

        // Get energy required for next breakthrough
        float energy_to_next_realm() const;
        bool is_max_realm() const { return _current_realm >= SPIRIT_SEVERING; }

        // Realm definitions
        static const RealmDefinition &get_realm_definition(Realm p_realm);

    protected:
        static void _bind_methods();

    private:
        Realm _current_realm = MORTAL;
        float _spiritual_energy = 0.0f;
        RealmDefinition _realms[REALM_COUNT];

        void _init_realms();
    };

} // namespace godot

#endif // CPP_KAKI_CULTIVATION_SYSTEM_H
