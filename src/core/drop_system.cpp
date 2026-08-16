#include "drop_system.h"
#include "../nodes/enemy.h"
#include "../nodes/item_pickup.h"
#include <godot_cpp/classes/node2d.hpp>
import mcpp_kaki.core;

import mcpp_kaki.utils;

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void DropSystem::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_enemy_killed", "enemy", "killer"),
                         &DropSystem::_on_enemy_killed);
    ClassDB::bind_method(D_METHOD("_on_elite_killed", "pos", "tier", "realm"),
                         &DropSystem::_on_elite_killed);
    ClassDB::bind_method(D_METHOD("_do_spawn_drops", "pos", "drop_table", "is_boss", "is_ranged", "is_flying", "realm"),
                         &DropSystem::_do_spawn_drops);
    ClassDB::bind_method(D_METHOD("_do_spawn_elite_drops", "pos", "tier", "realm"),
                         &DropSystem::_do_spawn_elite_drops);
    ClassDB::bind_method(D_METHOD("spawn_drop", "item_id", "qty", "pos"),
                         &DropSystem::spawn_drop);
}

void DropSystem::_ready() {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    SignalBus *bus = SignalBus::get_singleton();
    if (bus) {
        bus->connect("enemy_killed", Callable(this, "_on_enemy_killed"));
        bus->connect("elite_killed", Callable(this, "_on_elite_killed"));
    }
}

void DropSystem::spawn_drop(const StringName &p_item_id, int p_qty, const Vector2 &p_pos) {
    ItemPickup *pickup = memnew(ItemPickup);
    pickup->set_item_id(p_item_id);
    pickup->set_quantity(p_qty);
    pickup->set_position(p_pos);
    // 掉落物挂到玩家当前父节点（Portal 房间/洞天内击杀 → 挂进房间，否则跨父节点拾取被拒）；
    // 找不到玩家时退回场景根（与 bootstrap 的拾取物同级）
    Node *parent = nullptr;
    Node *scene = get_tree() ? get_tree()->get_current_scene() : nullptr;
    Node *player = scene ? scene->find_child("Player", true, false) : nullptr;
    if (player && player->get_parent())
        parent = player->get_parent();
    if (!parent)
        parent = get_parent() ? get_parent() : this;
    parent->add_child(pickup);
}

void DropSystem::_on_enemy_killed(Object *p_enemy, Object *p_killer) {
    Node2D *enemy = Object::cast_to<Node2D>(p_enemy);
    if (!enemy)
        return;

    // 幻境之敌（心魔/三尸）不掉落
    Enemy *e = Object::cast_to<Enemy>(p_enemy);
    if (e && e->no_drops)
        return;

    // 命名掉落表（敌人侧注册属性 drop_table；旧敌人/测试怪 get 不到 → 空串=类别兜底）
    String drop_table;
    Variant dt_v = enemy->get("drop_table");
    if (dt_v.get_type() == Variant::STRING)
        drop_table = dt_v;

    // 境界（用于 min_realm 门槛过滤；get 不到按 0）
    int realm = 0;
    Variant realm_v = enemy->get("realm");
    if (realm_v.get_type() == Variant::INT)
        realm = realm_v;

    // enemy_killed 来自碰撞回调（物理刷新中），生成掉落必须延迟到空闲帧
    call_deferred("_do_spawn_drops",
                  enemy->get_global_position(),
                  drop_table,
                  bool(enemy->get("is_boss")),
                  bool(enemy->get("is_ranged")),
                  bool(enemy->get("is_flying")),
                  realm);
}

void DropSystem::_on_elite_killed(const Vector2 &p_pos, int p_tier, int p_realm) {
    call_deferred("_do_spawn_elite_drops", p_pos, p_tier, p_realm);
}

void DropSystem::_do_spawn_drops(const Vector2 &p_pos, const String &p_drop_table, bool p_is_boss,
                                 bool p_is_ranged, bool p_is_flying, int p_realm) {
    std::vector<DropEntry> drops = _roll_drops(p_drop_table, p_is_boss, p_is_ranged, p_is_flying, p_realm);

    for (const DropEntry &entry : drops) {
        int qty = entry.min_qty;
        if (entry.max_qty > entry.min_qty) {
            qty = UtilityFunctions::randi_range(entry.min_qty, entry.max_qty);
        }
        // 小范围散落，避免叠在一起
        Vector2 scatter(UtilityFunctions::randf_range(-10.0f, 10.0f),
                        UtilityFunctions::randf_range(-6.0f, 2.0f));
        spawn_drop(entry.item_id, qty, p_pos + scatter);
    }
}

void DropSystem::_do_spawn_elite_drops(const Vector2 &p_pos, int p_tier, int p_realm) {
    // tier=1 roll 一次，tier=2 两次，封顶 3 次，每次独立 roll
    int rolls = p_tier < 1 ? 1 : (p_tier > 3 ? 3 : p_tier);
    for (int i = 0; i < rolls; i++) {
        std::vector<DropEntry> drops = _roll_elite_drops(p_realm);
        for (const DropEntry &entry : drops) {
            int qty = entry.min_qty;
            if (entry.max_qty > entry.min_qty) {
                qty = UtilityFunctions::randi_range(entry.min_qty, entry.max_qty);
            }
            Vector2 scatter(UtilityFunctions::randf_range(-10.0f, 10.0f),
                            UtilityFunctions::randf_range(-6.0f, 2.0f));
            spawn_drop(entry.item_id, qty, p_pos + scatter);
        }
    }
}

bool DropSystem::_load_table(const String &p_key, std::vector<DropEntry> &p_out) const {
    SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *scene = st ? st->get_current_scene() : nullptr;
    DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
    if (!dl)
        return false;

    Dictionary dt = dl->get_drop_table();

    // v2: 顶层 "tables" 包裹；老格式：顶层平铺
    Array arr;
    if (dt.has("tables")) {
        Dictionary tables = dt["tables"];
        if (tables.has(p_key))
            arr = tables[p_key];
    } else if (dt.has(p_key)) {
        arr = dt[p_key];
    }
    if (arr.is_empty())
        return false;

    for (int i = 0; i < arr.size(); i++) {
        Dictionary d = arr[i];
        DropEntry entry;
        entry.item_id = StringName(d["item"]);
        entry.min_qty = int(d["min"]);
        entry.max_qty = int(d["max"]);
        entry.chance = float(d["chance"]);
        entry.min_realm = d.has("min_realm") ? int(d["min_realm"]) : 0;
        p_out.push_back(entry);
    }
    return !p_out.empty();
}

std::vector<DropSystem::DropEntry> DropSystem::_filter_and_roll(
        const std::vector<DropEntry> &p_table, int p_realm) {
    std::vector<DropEntry> result;
    for (const DropEntry &entry : p_table) {
        if (p_realm < entry.min_realm)
            continue; // 境界门槛：高境怪才掉好货
        if (UtilityFunctions::randf() <= entry.chance) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<DropSystem::DropEntry> DropSystem::_roll_drops(
        const String &p_drop_table, bool p_is_boss, bool p_is_ranged, bool p_is_flying, int p_realm) {
    std::vector<DropEntry> table;

    // 1) 命名表优先（drop_table 非空且 JSON 有同名表）
    if (!p_drop_table.is_empty())
        _load_table(p_drop_table, table);

    // 2) 命名表找不到/为空 → 类别兜底（不许空掉）
    if (table.empty()) {
        _load_table(p_is_boss ? "boss" : "normal", table);
        if (!p_is_boss && (p_is_ranged || p_is_flying))
            _load_table("normal_ranged", table);
    }

    // 3) JSON 不可用 → 硬编码兜底
    if (table.empty())
        table = _fallback_category_table(p_is_boss, p_is_ranged, p_is_flying);

    return _filter_and_roll(table, p_realm);
}

std::vector<DropSystem::DropEntry> DropSystem::_roll_elite_drops(int p_realm) {
    std::vector<DropEntry> table;
    if (!_load_table("elite", table))
        table = _fallback_elite_table();
    return _filter_and_roll(table, p_realm);
}

std::vector<DropSystem::DropEntry> DropSystem::_fallback_category_table(
        bool p_is_boss, bool p_is_ranged, bool p_is_flying) {
    std::vector<DropEntry> table;
    if (p_is_boss) {
        table.push_back({ "spirit_stone", 5, 10, 1.0f, 0 });
        table.push_back({ "spirit_stone_mid", 1, 2, 0.8f, 0 }); // 中品灵石（session 012）
        table.push_back({ "healing_pill", 2, 3, 1.0f, 0 });
        table.push_back({ "foundation_pill", 1, 2, 0.8f, 0 });
        table.push_back({ "qi_pill", 1, 3, 0.6f, 0 });
        table.push_back({ "qian_nian_ling_zhi", 1, 1, 1.0f, 0 });
    } else {
        table.push_back({ "spirit_stone", 2, 4, 0.6f, 0 });
        table.push_back({ "healing_pill", 1, 1, 0.25f, 0 });
        table.push_back({ "zhi_xue_cao", 1, 2, 0.15f, 0 });
        table.push_back({ "ju_ling_cao", 1, 2, 0.15f, 0 });
        table.push_back({ "brown_rice", 1, 2, 0.25f, 0 }); // 食物：糙米饭
        table.push_back({ "dry_ration", 1, 1, 0.15f, 0 });  // 食物：干粮
        table.push_back({ "spirit_stone_mid", 1, 1, 0.3f, 4 });  // 金丹境怪起掉中品
        table.push_back({ "spirit_stone_high", 1, 1, 0.15f, 7 }); // 炼虚境起掉上品
        if (p_is_ranged || p_is_flying) {
            table.push_back({ "qi_pill", 1, 2, 0.2f, 0 });
        }
    }
    return table;
}

std::vector<DropSystem::DropEntry> DropSystem::_fallback_elite_table() {
    std::vector<DropEntry> table;
    table.push_back({ "spirit_stone", 2, 4, 1.0f, 0 });
    table.push_back({ "healing_pill", 1, 2, 0.8f, 0 });
    table.push_back({ "qi_pill", 1, 2, 0.5f, 0 });
    table.push_back({ "spirit_stone_mid", 1, 2, 0.6f, 4 }); // 金丹境精英起掉中品
    table.push_back({ "ju_ling_cao", 1, 2, 0.4f, 0 });
    return table;
}

} // namespace godot
