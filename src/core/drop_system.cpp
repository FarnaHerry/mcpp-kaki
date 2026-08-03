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
    ClassDB::bind_method(D_METHOD("_do_spawn_drops", "pos", "is_boss", "is_ranged", "is_flying"),
                         &DropSystem::_do_spawn_drops);
    ClassDB::bind_method(D_METHOD("spawn_drop", "item_id", "qty", "pos"),
                         &DropSystem::spawn_drop);
}

void DropSystem::_ready() {
    if (Engine::get_singleton()->is_editor_hint())
        return;

    SignalBus *bus = SignalBus::get_singleton();
    if (bus) {
        bus->connect("enemy_killed", Callable(this, "_on_enemy_killed"));
    }
}

void DropSystem::spawn_drop(const StringName &p_item_id, int p_qty, const Vector2 &p_pos) {
    ItemPickup *pickup = memnew(ItemPickup);
    pickup->set_item_id(p_item_id);
    pickup->set_quantity(p_qty);
    pickup->set_position(p_pos);
    // 掉落物挂到场景根（与 bootstrap 的拾取物同级）
    Node *parent = get_parent() ? get_parent() : this;
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

    // enemy_killed 来自碰撞回调（物理刷新中），生成掉落必须延迟到空闲帧
    call_deferred("_do_spawn_drops",
                  enemy->get_global_position(),
                  bool(enemy->get("is_boss")),
                  bool(enemy->get("is_ranged")),
                  bool(enemy->get("is_flying")));
}

void DropSystem::_do_spawn_drops(const Vector2 &p_pos, bool p_is_boss,
                                 bool p_is_ranged, bool p_is_flying) {
    std::vector<DropEntry> drops = _roll_drops(p_is_boss, p_is_ranged, p_is_flying);

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

std::vector<DropSystem::DropEntry> DropSystem::_roll_drops(
        bool p_is_boss, bool p_is_ranged, bool p_is_flying) {
    std::vector<DropEntry> table;

    // Try DataLoader JSON first
    SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
    Node *scene = st ? st->get_current_scene() : nullptr;
    DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;

    auto parse = [&](const String &key) {
        if (!dl) return;
        Dictionary dt = dl->get_drop_table();
        if (!dt.has(key)) return;
        Array arr = dt[key];
        for (int i = 0; i < arr.size(); i++) {
            Dictionary d = arr[i];
            table.push_back({
                StringName(d["item"]),
                int(d["min"]), int(d["max"]),
                float(d["chance"])
            });
        }
    };

    if (dl) {
        parse(p_is_boss ? "boss" : "normal");
        if (!p_is_boss && (p_is_ranged || p_is_flying))
            parse("normal_ranged");
    }

    // Fallback: hardcoded table
    if (table.empty()) {
        if (p_is_boss) {
            table.push_back({ "spirit_stone", 5, 10, 1.0f });
            table.push_back({ "healing_pill", 2, 3, 1.0f });
            table.push_back({ "foundation_pill", 1, 2, 0.8f });
            table.push_back({ "qi_pill", 1, 3, 0.6f });
            table.push_back({ "qian_nian_ling_zhi", 1, 1, 1.0f });
        } else {
            table.push_back({ "spirit_stone", 1, 3, 0.6f });
            table.push_back({ "healing_pill", 1, 1, 0.25f });
            table.push_back({ "zhi_xue_cao", 1, 2, 0.15f });
            table.push_back({ "ju_ling_cao", 1, 2, 0.15f });
            if (p_is_ranged || p_is_flying) {
                table.push_back({ "qi_pill", 1, 2, 0.2f });
            }
        }
    }

    // Roll chance
    std::vector<DropEntry> result;
    for (const DropEntry &entry : table) {
        if (UtilityFunctions::randf() <= entry.chance) {
            result.push_back(entry);
        }
    }
    return result;
}

} // namespace godot
