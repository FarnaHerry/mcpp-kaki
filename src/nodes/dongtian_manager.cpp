#include "dongtian_manager.h"
#include "camera_room_2d.h"
#include "enemy.h"
#include "player.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/marker2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.cultivation; // BreakthroughManager / AbilityManager
import mcpp_kaki.utils;       // SignalBus

namespace godot {

    void DongtianManager::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_player", "player"), &DongtianManager::set_player);
        ClassDB::bind_method(D_METHOD("set_camera", "camera"), &DongtianManager::set_camera);
        ClassDB::bind_method(D_METHOD("is_inside"), &DongtianManager::is_inside);
        ClassDB::bind_method(D_METHOD("get_return_position"), &DongtianManager::get_return_position);
        ClassDB::bind_method(D_METHOD("force_exit_for_load"), &DongtianManager::force_exit_for_load);
    }

    void DongtianManager::_process(double p_delta) {
        // 原因提示自动消隐
        if (_hint_t > 0.0f) {
            _hint_t -= float(p_delta);
            if (_hint_t <= 0.0f) {
                SignalBus *bus = SignalBus::get_singleton();
                if (bus) bus->emit_signal("interaction_prompt", "", false);
            }
        }

        if (!Input::get_singleton()->is_action_just_pressed("dongtian"))
            return;
        if (_inside) {
            _exit(true);
        } else {
            _try_enter();
        }
    }

    // ============================================================
    // 进入检查
    // ============================================================

    bool DongtianManager::_in_combat() const {
        TypedArray<Node> enemies = get_tree()->get_nodes_in_group("enemies");
        for (int i = 0; i < enemies.size(); i++) {
            Enemy *e = Object::cast_to<Enemy>(enemies[i]);
            if (!e || e->is_dead())
                continue;
            StringName s = e->state_machine ? e->state_machine->get_current_name() : StringName();
            if (s == StringName("chase") || s == StringName("attack") || s == StringName("boss_special"))
                return true;
        }
        return false;
    }

    void DongtianManager::_try_enter() {
        if (!_player || _player->is_dead())
            return;

        // 1. 能力门控：炼虚解锁
        AbilityManager *abilities = _player->get_ability_manager();
        if (!abilities || !abilities->has_ability(AbilityManager::ABILITY_DONGTIAN)) {
            _show_reason(LOC("需炼虚期修为，方可开辟洞天"));
            return;
        }

        Node *root = get_tree()->get_current_scene();

        // 2. 机缘事件/秘境中禁入
        BreakthroughManager *bt = root ? Object::cast_to<BreakthroughManager>(
                root->find_child("BreakthroughManager", false, false)) : nullptr;
        if (bt && bt->is_active()) {
            _show_reason(LOC("机缘当前，心无旁骛"));
            return;
        }

        // 3. 云海强渡中禁入
        if (root && root->get_scene_file_path() == String(YUNHAI_SCENE)) {
            _show_reason(LOC("云海强渡中，无法进入洞天"));
            return;
        }

        // 4. Portal 房间/秘境内禁入（玩家不在主场景根下即处于子空间）
        if (_player->get_parent() != root) {
            _show_reason(LOC("此处空间逼仄，无法开辟洞天"));
            return;
        }

        // 5. 战斗中禁入
        if (_in_combat()) {
            _show_reason(LOC("战斗中无法进入洞天"));
            return;
        }

        _enter();
    }

    // ============================================================
    // 进出
    // ============================================================

    void DongtianManager::_enter() {
        Node *root = get_tree()->get_current_scene();
        if (!root || !_player)
            return;

        _saved_world_pos = _player->get_global_position();

        Ref<PackedScene> scene = ResourceLoader::get_singleton()->load(DONGTIAN_SCENE);
        ERR_FAIL_COND(scene.is_null());
        _loaded_scene = scene->instantiate();
        ERR_FAIL_NULL(_loaded_scene);
        root->add_child(_loaded_scene);

        // 玩家重挂载进洞天
        Node *parent = _player->get_parent();
        if (parent) parent->remove_child(_player);
        _loaded_scene->add_child(_player);

        Vector2 spawn = _bounds.get_center();
        Marker2D *marker = Object::cast_to<Marker2D>(_loaded_scene->get_node_or_null("SpawnEntrance"));
        if (marker) spawn = marker->get_position();
        _player->set_position(spawn);
        _player->set("velocity", Vector2(0, 0));

        if (_camera) _camera->enter_room(_bounds);

        _inside = true;
        SignalBus *bus = SignalBus::get_singleton();
        if (bus) bus->emit_signal("dongtian_entered");
    }

    void DongtianManager::_exit(bool p_restore_pos) {
        if (!_loaded_scene || !_player)
            return;

        Node *parent = _player->get_parent();
        if (parent) parent->remove_child(_player);
        Node *root = get_tree()->get_current_scene();
        if (root) root->add_child(_player);

        if (p_restore_pos) {
            _player->set_global_position(_saved_world_pos);
            _player->set("velocity", Vector2(0, 0));
        }

        _loaded_scene->queue_free();
        _loaded_scene = nullptr;

        if (_camera) _camera->exit_room();

        _inside = false;
        SignalBus *bus = SignalBus::get_singleton();
        if (bus) bus->emit_signal("dongtian_exited");
    }

    void DongtianManager::force_exit_for_load() {
        if (_inside)
            _exit(false);
    }

    void DongtianManager::_show_reason(const String &p_text) {
        SignalBus *bus = SignalBus::get_singleton();
        if (!bus)
            return;
        bus->emit_signal("interaction_prompt", p_text, true);
        _hint_t = 2.5f;
    }

} // namespace godot
