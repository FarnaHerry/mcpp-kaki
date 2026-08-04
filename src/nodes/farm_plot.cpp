#include "farm_plot.h"
#include "dongtian_manager.h"
#include "../nodes/player.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

import mcpp_kaki.inventory; // ItemDatabase
import mcpp_kaki.utils;     // SignalBus

namespace godot {

    void FarmPlot::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_plot_index", "index"), &FarmPlot::set_plot_index);
        ClassDB::bind_method(D_METHOD("get_plot_index"), &FarmPlot::get_plot_index);
        ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &FarmPlot::_on_body_entered);
        ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &FarmPlot::_on_body_exited);

        ADD_PROPERTY(PropertyInfo(Variant::INT, "plot_index"), "set_plot_index", "get_plot_index");
    }

    void FarmPlot::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        // Detect Player body (layer 3)
        set_collision_layer_value(1, false);
        set_collision_mask_value(3, true);
        set_deferred("monitoring", true);
        set_deferred("monitorable", false);

        connect("body_entered", Callable(this, "_on_body_entered"));
        connect("body_exited", Callable(this, "_on_body_exited"));

        CollisionShape2D *shape = memnew(CollisionShape2D);
        Ref<RectangleShape2D> rect;
        rect.instantiate();
        rect->set_size(Vector2(22, 20));
        shape->set_shape(rect);
        add_child(shape);

        _refresh_visual();
        set_process(true);
    }

    DongtianManager *FarmPlot::_find_manager() {
        if (_manager)
            return _manager;
        Node *root = get_tree()->get_current_scene();
        if (root) {
            _manager = Object::cast_to<DongtianManager>(root->find_child("DongtianManager", false, false));
        }
        return _manager;
    }

    // ============================================================
    // 视觉（空地=无 / 生长中=嫩芽 / 成熟=品级色植株）
    // ============================================================

    void FarmPlot::_refresh_visual() {
        DongtianManager *mgr = _find_manager();
        Dictionary plot = mgr ? mgr->get_plot(_plot_index) : Dictionary();
        bool empty = plot.get("empty", true);
        bool mature = plot.get("mature", false);

        if (empty) {
            if (_sprout) {
                _sprout->queue_free();
                _sprout = nullptr;
            }
            _was_mature = false;
            return;
        }

        if (!_sprout) {
            _sprout = memnew(Polygon2D);
            _sprout->set_name("Sprout");
            add_child(_sprout);
        }

        if (mature) {
            // 成熟：品级配色（凡=翠绿 / 灵=冰蓝 / 地=紫金），完整植株
            const Item *def = ItemDatabase::get_singleton()->get_item(plot.get("herb", StringName()));
            Color c = Color(0.3f, 0.9f, 0.4f, 0.95f);
            if (def) {
                if (def->grade == 1) c = Color(0.4f, 0.7f, 1.0f, 0.95f);
                else if (def->grade >= 2) c = Color(0.8f, 0.5f, 1.0f, 0.95f);
            }
            _sprout->set_color(c);
            PackedVector2Array plant;
            plant.append(Vector2(0, -12));
            plant.append(Vector2(4, -4));
            plant.append(Vector2(8, -8));
            plant.append(Vector2(6, 0));
            plant.append(Vector2(4, 2));
            plant.append(Vector2(-4, 2));
            plant.append(Vector2(-6, 0));
            plant.append(Vector2(-8, -8));
            plant.append(Vector2(-4, -4));
            _sprout->set_polygon(plant);
        } else {
            // 生长中：小嫩芽
            _sprout->set_color(Color(0.5f, 0.8f, 0.4f, 0.9f));
            PackedVector2Array sprout;
            sprout.append(Vector2(0, -5));
            sprout.append(Vector2(2, -1));
            sprout.append(Vector2(2, 1));
            sprout.append(Vector2(-2, 1));
            sprout.append(Vector2(-2, -1));
            _sprout->set_polygon(sprout);
        }
        _was_mature = mature;
    }

    // ============================================================
    // 交互
    // ============================================================

    void FarmPlot::_on_body_entered(Node2D *p_body) {
        if (p_body->get_name() != StringName("Player"))
            return;
        // 幽灵 enter 守卫：场景 reparent/创建帧物理会误报远处重叠（实测 57px 外也触发），
        // 真重叠必在 48px 内（盒 22×20 + 胶囊 r8/h18）
        if (p_body->get_global_position().distance_to(get_global_position()) > 48.0f)
            return;
        _player = Object::cast_to<Player>(p_body);
        _update_prompt();
    }

    void FarmPlot::_on_body_exited(Node2D *p_body) {
        if (p_body->get_name() != StringName("Player"))
            return;
        if (!_player)
            return; // 幽灵 exit（对应 enter 被距离守卫挡掉）：不清提示
        _player = nullptr;
        // 同空间离开才清提示（跨空间 exit 时序不定，可能误清目标空间的新提示）
        if (p_body->get_parent() != get_parent())
            return;
        SignalBus *bus = SignalBus::get_singleton();
        if (bus)
            bus->emit_signal("interaction_prompt", "", false);
    }

    void FarmPlot::_update_prompt() {
        if (!_player)
            return;
        DongtianManager *mgr = _find_manager();
        if (!mgr)
            return;
        SignalBus *bus = SignalBus::get_singleton();
        if (!bus)
            return;

        Dictionary plot = mgr->get_plot(_plot_index);
        if (plot.get("empty", true)) {
            StringName seed = mgr->get_first_plantable();
            if (seed.is_empty()) {
                bus->emit_signal("interaction_prompt", LOC("灵田空置（无草药可播种）"), true);
            } else {
                const Item *def = ItemDatabase::get_singleton()->get_item(seed);
                bus->emit_signal("interaction_prompt",
                        String(LOC("[X] 播种 ·")) + (def ? def->name : String(seed)), true);
            }
        } else if (plot.get("mature", false)) {
            bus->emit_signal("interaction_prompt",
                    String(LOC("[X] 收获 ·")) + LOC(String(plot.get("herb_name", ""))), true);
        } else {
            int rem = int(plot.get("remaining", 0));
            String mm = String::num_int64(rem / 60).pad_zeros(2);
            String ss = String::num_int64(rem % 60).pad_zeros(2);
            bus->emit_signal("interaction_prompt",
                    LOC(String(plot.get("herb_name", ""))) + LOC(" 生长中 ") + mm + ":" + ss, true);
        }
    }

    void FarmPlot::_interact() {
        DongtianManager *mgr = _find_manager();
        if (!mgr || !_player)
            return;
        Dictionary plot = mgr->get_plot(_plot_index);
        SignalBus *bus = SignalBus::get_singleton();

        if (plot.get("empty", true)) {
            if (mgr->plant(_plot_index)) {
                _refresh_visual();
            } else if (bus) {
                bus->emit_signal("interaction_prompt", LOC("无草药可播种"), true);
            }
        } else if (plot.get("mature", false)) {
            int n = mgr->harvest(_plot_index);
            if (n > 0) {
                _refresh_visual();
                if (bus) {
                    bus->emit_signal("interaction_prompt",
                            LOC(String(plot.get("herb_name", ""))) + LOC(" 收获 ×") + String::num_int64(n), true);
                }
            }
        }
        // 生长中：不响应 X
    }

    void FarmPlot::_process(double p_delta) {
        // 成熟瞬间刷新视觉（生长中 → 成熟）
        DongtianManager *mgr = _find_manager();
        if (mgr) {
            Dictionary plot = mgr->get_plot(_plot_index);
            bool mature = !plot.get("empty", true) && plot.get("mature", false);
            if (mature != _was_mature) {
                _refresh_visual();
                if (_player)
                    _update_prompt();
            }
        }

        if (!_player)
            return;

        // 生长中提示每秒刷新倒计时
        _prompt_refresh -= float(p_delta);
        if (_prompt_refresh <= 0.0f) {
            _prompt_refresh = 1.0f;
            if (mgr) {
                Dictionary plot = mgr->get_plot(_plot_index);
                if (!plot.get("empty", true) && !plot.get("mature", false))
                    _update_prompt();
            }
        }

        if (Input::get_singleton()->is_action_just_pressed("interact")) {
            _interact();
            _update_prompt();
        }
    }

} // namespace godot
