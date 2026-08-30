module;

#include <deque>
#include <string>
#include <vector>

#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_set.hpp>

module mcpp_kaki.cultivation;
import mcpp_kaki.core; // DataLoader（JSON 优先加载，参考 GongfaSystem）
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
    const char *AbilityManager::ABILITY_DONGTIAN = "dongtian";
    const char *AbilityManager::ABILITY_UNITY_FORM = "unity_form";
    const char *AbilityManager::ABILITY_MERIT_HALO = "merit_halo";
    const char *AbilityManager::ABILITY_CLOUD_FLIGHT = "cloud_flight";
    const char *AbilityManager::ABILITY_TRIBULATION_IMMUNITY = "tribulation_immunity";
    const char *AbilityManager::ABILITY_GIANT_FORM = "giant_form";
    const char *AbilityManager::ABILITY_GOLDEN_BODY = "golden_body";
    const char *AbilityManager::ABILITY_DAO_DOMAIN = "dao_domain";
    const char *AbilityManager::ABILITY_MYRIAD_AVATARS = "myriad_avatars";

    // 能力解锁表（JSON data/abilities.json 优先，此为硬编码兜底；数组序=能力页显示序，
    // 主动 15 在前、被动 7 在后。开辟洞天 dongtian 属系统能力不入此表，见 check_realm_unlocks）
    static const AbilityManager::AbilityDef ABILITY_DEFS[] = {
        // ---- 主动 ----
        { "dash",                 "冲刺",     true,  true,  -1, false, "初始", "地面/空中短促冲刺（Z）。" },
        { "double_jump",          "二段跳",   true,  false, 1,  false, "炼气", "空中可再跳跃一次。" },
        { "air_dash",             "空中冲刺", true,  false, 2,  false, "筑基", "空中亦可发动冲刺。" },
        { "short_flight",         "短暂飞行", true,  false, 2,  false, "筑基", "空中再按跳驭剑短飞，耗灵力。" },
        { "glide",                "滑翔",     true,  false, 3,  false, "金丹", "高处缓降滑翔。" },
        { "free_flight",          "自主飞行", true,  false, 3,  false, "金丹", "无拘飞行，不耗灵力。" },
        { "soul_exit",            "元婴出窍", true,  false, 4,  false, "元婴", "元婴离体神游。" },
        { "domain",               "领域展开", true,  false, 4,  false, "元婴", "展开自身领域。" },
        { "spirit_travel",        "神游太虚", true,  false, 5,  false, "化神", "神识远游太虚。" },
        { "spirit_sense",         "神识扫描", true,  false, 5,  false, "化神", "神识外放探查四方。" },
        { "void_shift",           "虚实转换", true,  false, 6,  false, "炼虚", "虚实之间自在转换。" },
        { "cloud_flight",         "腾云驾雾", true,  false, 10, false, "真仙", "腾云驾雾，跨海登洲。" },
        { "giant_form",           "法天象地", true,  false, 11, false, "金仙", "法相顶天立地。" },
        { "dao_domain",           "道域展开", true,  false, -1, true,  "混元", "混元一气，展开大道之域。" },
        { "myriad_avatars",       "化身千万", true,  false, -1, true,  "混元", "混元一气，化身千千万万。" },
        // ---- 被动 ----
        { "wall_cling",           "攀墙",     false, true,  -1, false, "初始", "贴墙缓降，可蹬墙跳。" },
        { "storage_ring",         "纳戒",     false, false, 1,  false, "炼气", "纳戒储物，背包扩容并磁吸拾取。" },
        { "spirit_vision",        "灵视",     false, false, 2,  false, "筑基", "灵目初开，洞察灵机。" },
        { "unity_form",           "形神合一", false, false, 7,  false, "合体", "肉身元神双轨汇合为一。" },
        { "merit_halo",           "功德金光", false, false, 8,  false, "大乘", "功德加身，金光护体。" },
        { "tribulation_immunity", "三灾免疫", false, false, 10, false, "真仙", "渡劫功成，免疫凡间雷火风。" },
        { "golden_body",          "金身护体", false, false, 11, false, "金仙", "金仙不灭之躯。" },
    };

    std::vector<AbilityManager::AbilityDef> AbilityManager::s_defs;
    bool AbilityManager::s_defs_loaded = false;

    void AbilityManager::ensure_defs_loaded() {
        if (s_defs_loaded) return;
        s_defs_loaded = true;
        // AbilityDef 是 const char*——JSON 字符串必须有人持有。deque push_back 不搬动既有
        // 元素，c_str() 不会悬空（vector 扩容搬 SSO 内联缓冲会 dangling，禁用）
        static std::deque<std::string> s_strings;
        auto own = [](const String &p_s) -> const char * {
            s_strings.push_back(std::string(p_s.utf8().get_data()));
            return s_strings.back().c_str();
        };
        SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        Node *scene = st ? st->get_current_scene() : nullptr;
        DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
        if (dl) {
            Array all = dl->get_all_abilities();
            for (int i = 0; i < all.size(); i++) {
                Dictionary d = all[i];
                if (!d.has("id")) continue;
                AbilityDef def;
                def.id = own(d["id"]);
                def.name = own(d.get("name", d["id"]));
                def.active = String(d.get("type", "active")) != String("passive");
                def.innate = bool(d.get("innate", false));
                def.unlock_realm = int(d.get("unlock_realm", -1));
                def.hunyuan = bool(d.get("hunyuan", false));
                def.cond = own(d.get("cond", String()));
                def.desc = own(d.get("desc", String()));
                s_defs.push_back(def);
            }
        }
        if (s_defs.empty()) { // JSON 不可用退回硬编码
            for (const AbilityDef &d : ABILITY_DEFS) { s_defs.push_back(d); }
        }
    }

    int AbilityManager::get_ability_count() {
        ensure_defs_loaded();
        return (int)s_defs.size();
    }

    const AbilityManager::AbilityDef *AbilityManager::get_ability(int p_idx) {
        ensure_defs_loaded();
        if (p_idx < 0 || p_idx >= (int)s_defs.size()) return nullptr;
        return &s_defs[p_idx];
    }

    const AbilityManager::AbilityDef *AbilityManager::find_def(const StringName &p_id) {
        ensure_defs_loaded();
        for (const AbilityDef &d : s_defs) {
            if (p_id == StringName(d.id)) return &d;
        }
        return nullptr;
    }

    Array AbilityManager::get_ability_list() const {
        ensure_defs_loaded();
        Array out;
        for (const AbilityDef &d : s_defs) {
            Dictionary row;
            row["id"] = TXT(d.id);
            row["name"] = TXT(d.name);
            row["type"] = TXT(d.active ? "active" : "passive");
            row["innate"] = d.innate;
            row["unlock_realm"] = d.unlock_realm;
            row["hunyuan"] = d.hunyuan;
            row["cond"] = TXT(d.cond);
            row["desc"] = TXT(d.desc);
            out.push_back(row);
        }
        return out;
    }

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
        ClassDB::bind_method(D_METHOD("get_ability_list"), &AbilityManager::get_ability_list);

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

        ensure_defs_loaded();
        int realm = _cultivation->get_realm_index();
        bool hunyuan = _cultivation->is_hunyuan();

        // 数据表驱动（data/abilities.json / ABILITY_DEFS 兜底）：
        // innate 项构造时已解锁；hunyuan 项看混元一气；其余按 unlock_realm 境界门控
        for (const AbilityDef &d : s_defs) {
            if (d.innate)
                continue;
            if (d.hunyuan) {
                if (hunyuan)
                    unlock_ability(StringName(d.id));
                continue;
            }
            if (d.unlock_realm >= 0 && realm >= d.unlock_realm)
                unlock_ability(StringName(d.id));
        }

        // 开辟洞天（dongtian）：系统能力，不入能力页表——炼虚解锁（O 键进出随身小世界）
        if (realm >= CultivationSystem::LIAN_XU)
            unlock_ability(ABILITY_DONGTIAN);
    }

    String AbilityManager::get_unlocked_list() const {
        String result;
        // Iterate HashSet and build comma-separated list
        // For now just return count
        return String(" abilities unlocked");
    }

} // namespace godot
