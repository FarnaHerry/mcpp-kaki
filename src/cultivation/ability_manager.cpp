module;

#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.cultivation;
namespace godot {

    const char *AbilityManager::ABILITY_DOUBLE_JUMP = "double_jump";
    const char *AbilityManager::ABILITY_WALL_CLING = "wall_cling";
    const char *AbilityManager::ABILITY_DASH = "dash";
    const char *AbilityManager::ABILITY_AIR_DASH = "air_dash";
    const char *AbilityManager::ABILITY_SPIRIT_VISION = "spirit_vision";
    const char *AbilityManager::ABILITY_GLIDE = "glide";
    const char *AbilityManager::ABILITY_STORAGE_RING = "storage_ring";
    const char *AbilityManager::ABILITY_SHORT_FLIGHT = "short_flight";
    const char *AbilityManager::ABILITY_FREE_FLIGHT = "free_flight";
    const char *AbilityManager::ABILITY_SOUL_EXIT = "soul_exit";
    const char *AbilityManager::ABILITY_DOMAIN = "domain";
    const char *AbilityManager::ABILITY_SPIRIT_TRAVEL = "spirit_travel";
    const char *AbilityManager::ABILITY_SPIRIT_SENSE = "spirit_sense";
    const char *AbilityManager::ABILITY_VOID_SHIFT = "void_shift";
    const char *AbilityManager::ABILITY_UNITY_FORM = "unity_form";
    const char *AbilityManager::ABILITY_MERIT_HALO = "merit_halo";
    const char *AbilityManager::ABILITY_CLOUD_FLIGHT = "cloud_flight";
    const char *AbilityManager::ABILITY_TRIBULATION_IMMUNITY = "tribulation_immunity";
    const char *AbilityManager::ABILITY_GIANT_FORM = "giant_form";
    const char *AbilityManager::ABILITY_GOLDEN_BODY = "golden_body";
    const char *AbilityManager::ABILITY_DAO_DOMAIN = "dao_domain";
    const char *AbilityManager::ABILITY_MYRIAD_AVATARS = "myriad_avatars";

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

        // 炼气 (1): 二段跳、纳戒（储物无限）
        if (realm >= CultivationSystem::QI_REFINING) {
            unlock_ability(ABILITY_DOUBLE_JUMP);
            unlock_ability(ABILITY_STORAGE_RING);
        }

        // 筑基 (2): 空冲、灵视、短暂飞行
        if (realm >= CultivationSystem::FOUNDATION) {
            unlock_ability(ABILITY_AIR_DASH);
            unlock_ability(ABILITY_SPIRIT_VISION);
            unlock_ability(ABILITY_SHORT_FLIGHT);
        }

        // 金丹 (3): 滑翔、自主飞行
        if (realm >= CultivationSystem::GOLDEN_CORE) {
            unlock_ability(ABILITY_GLIDE);
            unlock_ability(ABILITY_FREE_FLIGHT);
        }

        // 元婴 (4): 元婴出窍、领域展开
        if (realm >= CultivationSystem::NASCENT_SOUL) {
            unlock_ability(ABILITY_SOUL_EXIT);
            unlock_ability(ABILITY_DOMAIN);
        }

        // 化神 (5): 神游太虚、神识扫描
        if (realm >= CultivationSystem::SPIRIT_SEVERING) {
            unlock_ability(ABILITY_SPIRIT_TRAVEL);
            unlock_ability(ABILITY_SPIRIT_SENSE);
        }

        // 炼虚 (6): 虚实转换
        if (realm >= CultivationSystem::LIAN_XU) {
            unlock_ability(ABILITY_VOID_SHIFT);
        }

        // 合体 (7): 形神合一
        if (realm >= CultivationSystem::HE_TI) {
            unlock_ability(ABILITY_UNITY_FORM);
        }

        // 大乘 (8): 功德金光
        if (realm >= CultivationSystem::DA_CHENG) {
            unlock_ability(ABILITY_MERIT_HALO);
        }

        // 真仙 (10): 腾云驾雾、三灾免疫（渡劫成功奖励：免疫凡间雷火风）
        if (realm >= CultivationSystem::TRUE_IMMORTAL) {
            unlock_ability(ABILITY_CLOUD_FLIGHT);
            unlock_ability(ABILITY_TRIBULATION_IMMUNITY);
        }

        // 金仙 (11): 法天象地、金身护体
        if (realm >= CultivationSystem::GOLDEN_IMMORTAL) {
            unlock_ability(ABILITY_GIANT_FORM);
            unlock_ability(ABILITY_GOLDEN_BODY);
        }

        // 混元一气: 道域展开、化身千万
        if (_cultivation->is_hunyuan()) {
            unlock_ability(ABILITY_DAO_DOMAIN);
            unlock_ability(ABILITY_MYRIAD_AVATARS);
        }
    }

    String AbilityManager::get_unlocked_list() const {
        String result;
        // Iterate HashSet and build comma-separated list
        // For now just return count
        return String(" abilities unlocked");
    }

} // namespace godot
