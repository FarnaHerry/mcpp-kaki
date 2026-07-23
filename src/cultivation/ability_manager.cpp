#include "ability_manager.h"
#include "cultivation_system.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

    const char *AbilityManager::ABILITY_DOUBLE_JUMP = "double_jump";
    const char *AbilityManager::ABILITY_WALL_CLING = "wall_cling";
    const char *AbilityManager::ABILITY_DASH = "dash";
    const char *AbilityManager::ABILITY_AIR_DASH = "air_dash";
    const char *AbilityManager::ABILITY_SPIRIT_VISION = "spirit_vision";
    const char *AbilityManager::ABILITY_GLIDE = "glide";

    AbilityManager::AbilityManager() {
        // Basic abilities everyone starts with
        _unlocked.insert(ABILITY_WALL_CLING);
        _unlocked.insert(ABILITY_DASH);
    }

    void AbilityManager::_bind_methods() {
        ClassDB::bind_method(D_METHOD("unlock_ability", "ability_id"), &AbilityManager::unlock_ability);
        ClassDB::bind_method(D_METHOD("has_ability", "ability_id"), &AbilityManager::has_ability);
        ClassDB::bind_method(D_METHOD("check_realm_unlocks"), &AbilityManager::check_realm_unlocks);
        ClassDB::bind_method(D_METHOD("get_unlocked_list"), &AbilityManager::get_unlocked_list);

        ADD_SIGNAL(MethodInfo("ability_unlocked", PropertyInfo(Variant::STRING_NAME, "ability_id")));
    }

    void AbilityManager::unlock_ability(const StringName &p_ability_id) {
        if (_unlocked.has(p_ability_id))
            return;

        _unlocked.insert(p_ability_id);
        emit_signal("ability_unlocked", p_ability_id);
    }

    bool AbilityManager::has_ability(const StringName &p_ability_id) const {
        return const_cast<HashSet<StringName> &>(_unlocked).has(p_ability_id);
    }

    void AbilityManager::check_realm_unlocks() {
        if (!_cultivation)
            return;

        int realm = _cultivation->get_realm_index();

        // 炼气期 (1): double jump
        if (realm >= CultivationSystem::QI_REFINING) {
            unlock_ability(ABILITY_DOUBLE_JUMP);
        }

        // 筑基期 (2): air dash, spirit vision
        if (realm >= CultivationSystem::FOUNDATION) {
            unlock_ability(ABILITY_AIR_DASH);
            unlock_ability(ABILITY_SPIRIT_VISION);
        }

        // 金丹期 (3): glide
        if (realm >= CultivationSystem::GOLDEN_CORE) {
            unlock_ability(ABILITY_GLIDE);
        }
    }

    String AbilityManager::get_unlocked_list() const {
        String result;
        // Iterate HashSet and build comma-separated list
        // For now just return count
        return String(" abilities unlocked");
    }

} // namespace godot
