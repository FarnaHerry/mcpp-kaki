#include "cultivation_system.h"

#include <cstdlib>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

    void CultivationSystem::_init_realms() {
        // 凡人 - Mortal
        _realms[MORTAL] = { "凡人", 80.0f, 0.8f, 1.0f, 1.0f, 1.0f };

        // 炼气期 - Qi Refining
        _realms[QI_REFINING] = { "炼气期", 200.0f, 0.65f, 1.3f, 1.2f, 1.1f };

        // 筑基期 - Foundation Building
        _realms[FOUNDATION] = { "筑基期", 500.0f, 0.5f, 1.7f, 1.5f, 1.2f };

        // 金丹期 - Golden Core
        _realms[GOLDEN_CORE] = { "金丹期", 1200.0f, 0.35f, 2.3f, 2.0f, 1.35f };

        // 元婴期 - Nascent Soul
        _realms[NASCENT_SOUL] = { "元婴期", 3000.0f, 0.2f, 3.2f, 2.8f, 1.5f };

        // 化神期 - Spirit Severing
        _realms[SPIRIT_SEVERING] = { "化神期", 0.0f, 0.0f, 5.0f, 4.5f, 1.8f };
    }

    CultivationSystem::CultivationSystem() {
        _init_realms();
    }

    void CultivationSystem::_bind_methods() {
        ClassDB::bind_method(D_METHOD("accumulate_energy", "amount"), &CultivationSystem::accumulate_energy);
        ClassDB::bind_method(D_METHOD("attempt_breakthrough"), &CultivationSystem::attempt_breakthrough);
        ClassDB::bind_method(D_METHOD("get_realm_name"), &CultivationSystem::get_realm_name);
        ClassDB::bind_method(D_METHOD("get_realm_index"), &CultivationSystem::get_realm_index);
        ClassDB::bind_method(D_METHOD("get_spiritual_energy"), &CultivationSystem::get_spiritual_energy);
        ClassDB::bind_method(D_METHOD("get_max_energy"), &CultivationSystem::get_max_energy);
        ClassDB::bind_method(D_METHOD("get_damage_multiplier"), &CultivationSystem::get_damage_multiplier);
        ClassDB::bind_method(D_METHOD("get_defense_multiplier"), &CultivationSystem::get_defense_multiplier);
        ClassDB::bind_method(D_METHOD("get_speed_multiplier"), &CultivationSystem::get_speed_multiplier);
        ClassDB::bind_method(D_METHOD("energy_to_next_realm"), &CultivationSystem::energy_to_next_realm);
        ClassDB::bind_method(D_METHOD("is_max_realm"), &CultivationSystem::is_max_realm);

        ADD_SIGNAL(MethodInfo("realm_changed",
                              PropertyInfo(Variant::INT, "old_realm"),
                              PropertyInfo(Variant::INT, "new_realm")));
        ADD_SIGNAL(MethodInfo("breakthrough_success",
                              PropertyInfo(Variant::INT, "new_realm")));
        ADD_SIGNAL(MethodInfo("breakthrough_failed"));
        ADD_SIGNAL(MethodInfo("energy_changed",
                              PropertyInfo(Variant::FLOAT, "current"),
                              PropertyInfo(Variant::FLOAT, "max")));
    }

    String CultivationSystem::get_realm_name() const {
        return _realms[_current_realm].name;
    }

    float CultivationSystem::get_max_energy() const {
        return _realms[_current_realm].energy_to_advance;
    }

    void CultivationSystem::accumulate_energy(float p_amount) {
        _spiritual_energy += p_amount;
        emit_signal("energy_changed", _spiritual_energy, get_max_energy());
    }

    bool CultivationSystem::attempt_breakthrough() {
        if (is_max_realm())
            return false;

        float required = _realms[_current_realm].energy_to_advance;
        if (_spiritual_energy < required)
            return false;

        // Consume energy
        _spiritual_energy -= required;

        float chance = _realms[_current_realm].breakthrough_chance;
        float roll = float(std::rand()) / float(RAND_MAX);

        if (roll <= chance) {
            // Success!
            Realm old = _current_realm;
            _current_realm = (Realm)((int)_current_realm + 1);
            emit_signal("breakthrough_success", (int)_current_realm);
            emit_signal("realm_changed", (int)old, (int)_current_realm);
            return true;
        } else {
            // Failed — keep some progress (50% of energy)
            _spiritual_energy += required * 0.5f;
            emit_signal("breakthrough_failed");
            return false;
        }
    }

    float CultivationSystem::get_damage_multiplier() const {
        return _realms[_current_realm].damage_mult;
    }

    float CultivationSystem::get_defense_multiplier() const {
        return _realms[_current_realm].defense_mult;
    }

    float CultivationSystem::get_speed_multiplier() const {
        return _realms[_current_realm].speed_mult;
    }

    float CultivationSystem::energy_to_next_realm() const {
        if (is_max_realm())
            return 0.0f;
        return _realms[_current_realm].energy_to_advance;
    }

    const RealmDefinition &CultivationSystem::get_realm_definition(Realm p_realm) {
        static CultivationSystem dummy;
        return dummy._realms[p_realm];
    }

} // namespace godot
