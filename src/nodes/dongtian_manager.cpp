#include "dongtian_manager.h"
#include "camera_room_2d.h"
#include "enemy.h"
#include "player.h"
#include "../core/currency_system.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/marker2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

import mcpp_kaki.cultivation; // BreakthroughManager / AbilityManager / GongfaSystem
import mcpp_kaki.inventory;   // ItemDatabase / Inventory
import mcpp_kaki.utils;       // SignalBus

namespace godot {

    // 灵植采集点静态定义表：{固定草药, 产量, 刷新时长 s（现实时间）}
    struct HerbSpotDef {
        const char *herb;
        int qty;
        int64_t refresh;
    };
    static const HerbSpotDef HERB_SPOT_DEFS[DongtianManager::HERB_SPOTS] = {
        { "ju_ling_cao", 2, 120 },        // 聚灵草（灵品，2 分钟复生）
        { "qian_nian_ling_zhi", 1, 600 }, // 千年灵芝（地品，10 分钟复生）
        { "bing_xin_lian", 1, 300 },      // 冰心莲（地品，5 分钟复生）
        { "chi_yan_hua", 1, 300 },        // 赤焰花（地品，5 分钟复生）
    };

    DongtianManager::DongtianManager() {
        for (int i = 0; i < HERB_SPOTS; i++)
            _herb_spots[i].herb = StringName(HERB_SPOT_DEFS[i].herb);
    }

    void DongtianManager::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_player", "player"), &DongtianManager::set_player);
        ClassDB::bind_method(D_METHOD("set_camera", "camera"), &DongtianManager::set_camera);
        ClassDB::bind_method(D_METHOD("is_inside"), &DongtianManager::is_inside);
        ClassDB::bind_method(D_METHOD("get_return_position"), &DongtianManager::get_return_position);
        ClassDB::bind_method(D_METHOD("force_exit_for_load"), &DongtianManager::force_exit_for_load);
        ClassDB::bind_method(D_METHOD("get_plot", "index"), &DongtianManager::get_plot);
        ClassDB::bind_method(D_METHOD("get_first_plantable"), &DongtianManager::get_first_plantable);
        ClassDB::bind_method(D_METHOD("plant", "index"), &DongtianManager::plant);
        ClassDB::bind_method(D_METHOD("harvest", "index"), &DongtianManager::harvest);
        ClassDB::bind_method(D_METHOD("debug_age_plot", "index", "seconds"), &DongtianManager::debug_age_plot);
        ClassDB::bind_method(D_METHOD("get_plot_count"), &DongtianManager::get_plot_count);
        ClassDB::bind_method(D_METHOD("get_expand_cost"), &DongtianManager::get_expand_cost);
        ClassDB::bind_method(D_METHOD("expand_plot"), &DongtianManager::expand_plot);
        ClassDB::bind_method(D_METHOD("get_jlz_level"), &DongtianManager::get_jlz_level);
        ClassDB::bind_method(D_METHOD("get_jlz_upgrade_cost"), &DongtianManager::get_jlz_upgrade_cost);
        ClassDB::bind_method(D_METHOD("upgrade_jlz"), &DongtianManager::upgrade_jlz);
        ClassDB::bind_method(D_METHOD("get_jlz_bonus"), &DongtianManager::get_jlz_bonus);
        ClassDB::bind_method(D_METHOD("save_to_dict"), &DongtianManager::save_to_dict);
        ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &DongtianManager::load_from_dict);
        ClassDB::bind_method(D_METHOD("get_storage_slot", "index"), &DongtianManager::get_storage_slot);
        ClassDB::bind_method(D_METHOD("deposit_from_player", "inv_slot"), &DongtianManager::deposit_from_player);
        ClassDB::bind_method(D_METHOD("withdraw_to_player", "storage_slot"), &DongtianManager::withdraw_to_player);
        ClassDB::bind_method(D_METHOD("get_herb_spot", "index"), &DongtianManager::get_herb_spot);
        ClassDB::bind_method(D_METHOD("gather_herb_spot", "index"), &DongtianManager::gather_herb_spot);
        ClassDB::bind_method(D_METHOD("debug_age_herb_spot", "index", "seconds"), &DongtianManager::debug_age_herb_spot);
        ClassDB::bind_method(D_METHOD("is_invasion_active"), &DongtianManager::is_invasion_active);
        ClassDB::bind_method(D_METHOD("get_invaders_left"), &DongtianManager::get_invaders_left);
        ClassDB::bind_method(D_METHOD("debug_force_invasion"), &DongtianManager::debug_force_invasion);
        ClassDB::bind_method(D_METHOD("debug_suppress_invasion"), &DongtianManager::debug_suppress_invasion);
        ClassDB::bind_method(D_METHOD("_on_invader_killed", "enemy", "killer"), &DongtianManager::_on_invader_killed);
    }

    void DongtianManager::_process(double p_delta) {
        // enemy_killed 惰性连接（构造时 SignalBus 未必就绪；连接过一次即止）
        if (!_bus_connected) {
            SignalBus *bus = SignalBus::get_singleton();
            if (bus) {
                bus->connect("enemy_killed", Callable(this, "_on_invader_killed"));
                _bus_connected = true;
            }
        }

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
        // 储物面板打开（暂停中）时 O 归面板（关闭用），不响应退出洞天
        Node *root = get_tree()->get_current_scene();
        Node *panel = root ? root->find_child("StoragePanel", true, false) : nullptr;
        if (panel && bool(panel->call("is_open")))
            return;
        // 丹房面板（GDScript）同理
        Node *pill = root ? root->find_child("PillLabPanel", true, false) : nullptr;
        if (pill && bool(pill->call("is_open")))
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

        _roll_invasion();
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

        // 闯阵是一次性事件：出洞天即清场（场景卸载带走入侵者），状态不持久化
        _invasion_active = false;
        _invaders_left = 0;
        _invasion_forced = false;
        _invasion_suppressed = false;

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

    // ============================================================
    // 灵兽闯阵（随机入侵事件；不持久化，出洞天/读档清场）
    // ============================================================

    void DongtianManager::_roll_invasion() {
        bool forced = _invasion_forced;
        bool suppressed = _invasion_suppressed;
        _invasion_forced = false;
        _invasion_suppressed = false;
        if (suppressed || _invasion_active)
            return;
        bool trigger = forced;
        if (!trigger) {
            // 无头测试环境禁随机 roll（老洞天测试用例稳定性；测试走 debug_force_invasion）
            DisplayServer *ds = DisplayServer::get_singleton();
            bool headless = ds && ds->get_name() == String("headless");
            if (!headless)
                trigger = UtilityFunctions::randf() < INVASION_CHANCE;
        }
        if (trigger)
            _start_invasion();
    }

    void DongtianManager::_start_invasion() {
        if (!_loaded_scene || _invasion_active)
            return;
        // 难度随玩家境界缩放：realm = 玩家-1，最低 1
        int realm = 1;
        if (_player && _player->get_cultivation())
            realm = MAX(1, _player->get_cultivation()->get_realm_index() - 1);
        int spawned = 0;
        if (_loaded_scene->has_method("spawn_invasion"))
            spawned = int(_loaded_scene->call("spawn_invasion", realm));
        if (spawned <= 0)
            return;
        _invasion_active = true;
        _invaders_left = spawned;
        _show_reason(LOC("有灵兽闯入洞天！"));
    }

    void DongtianManager::_on_invader_killed(Object *p_enemy, Object *p_killer) {
        if (!_invasion_active)
            return;
        Node *n = Object::cast_to<Node>(p_enemy);
        if (!n || !n->is_in_group(StringName("dongtian_invaders")))
            return;
        _invaders_left--;
        if (_invaders_left <= 0) {
            _invasion_active = false;
            _invaders_left = 0;
            _show_reason(LOC("灵兽已伏诛，洞天重归安宁"));
        }
    }

    void DongtianManager::debug_force_invasion() {
        _invasion_forced = true;
        if (_inside && !_invasion_active)
            _start_invasion();
    }

    void DongtianManager::debug_suppress_invasion() {
        _invasion_suppressed = true;
    }

    // ============================================================
    // 灵田（现实时间生长；状态自持，场景卸载不丢）
    // ============================================================

    int64_t DongtianManager::_now() {
        return int64_t(Time::get_singleton()->get_unix_time_from_system());
    }

    Dictionary DongtianManager::get_plot(int p_index) const {
        Dictionary d;
        ERR_FAIL_INDEX_V(p_index, _plot_count, d);
        const Plot &p = _plots[p_index];
        d["empty"] = p.herb.is_empty();
        if (p.herb.is_empty())
            return d;
        const Item *def = ItemDatabase::get_singleton()->get_item(p.herb);
        int grow = def ? def->grow_seconds : 60;
        int64_t elapsed = _now() - p.planted_at;
        d["herb"] = p.herb;
        d["herb_name"] = def ? def->name : String(p.herb);
        d["mature"] = elapsed >= grow;
        d["remaining"] = int64_t(grow) - elapsed > 0 ? int(grow - elapsed) : 0;
        return d;
    }

    StringName DongtianManager::get_first_plantable() const {
        if (!_player || !_player->get_inventory())
            return StringName();
        ItemDatabase *db = ItemDatabase::get_singleton();
        if (!db)
            return StringName();
        // 品级低者优先（凡→灵→地）
        for (const StringName &id : db->get_plantable_ids()) {
            if (_player->get_inventory()->get_item_count(id) > 0)
                return id;
        }
        return StringName();
    }

    bool DongtianManager::plant(int p_index) {
        ERR_FAIL_INDEX_V(p_index, _plot_count, false);
        if (!_plots[p_index].herb.is_empty() || !_player || !_player->get_inventory())
            return false;
        StringName herb = get_first_plantable();
        if (herb.is_empty())
            return false;
        if (!_player->get_inventory()->remove_item(herb, 1))
            return false;
        _plots[p_index].herb = herb;
        _plots[p_index].planted_at = _now();
        return true;
    }

    int DongtianManager::harvest(int p_index) {
        ERR_FAIL_INDEX_V(p_index, _plot_count, 0);
        Plot &p = _plots[p_index];
        if (p.herb.is_empty() || !_player)
            return 0;
        const Item *def = ItemDatabase::get_singleton()->get_item(p.herb);
        int grow = def ? def->grow_seconds : 60;
        if (_now() - p.planted_at < grow)
            return 0; // 未成熟
        StringName herb = p.herb;
        p.herb = StringName();
        p.planted_at = 0;
        int yield = 2; // 种一收二
        _player->pickup_item(herb, yield);
        // 收获 = 练气行为（同采集）
        if (_player->get_gongfa()) {
            _player->get_gongfa()->feed(GongfaSystem::SCHOOL_QI, 2.0f);
        }
        return yield;
    }

    void DongtianManager::debug_age_plot(int p_index, double p_seconds) {
        ERR_FAIL_INDEX(p_index, _plot_count);
        _plots[p_index].planted_at -= int64_t(p_seconds);
    }

    // ============================================================
    // v4 扩张经营：灵田扩张 + 聚灵阵升级（灵石四阶钱包扣款）
    // ============================================================

    int DongtianManager::get_expand_cost() const {
        // 第 7~12 块价格递增（下品基准）
        static const int COSTS[MAX_PLOTS - BASE_PLOTS] = { 500, 800, 1200, 1800, 2500, 3500 };
        if (_plot_count >= MAX_PLOTS)
            return 0;
        return COSTS[_plot_count - BASE_PLOTS];
    }

    bool DongtianManager::expand_plot() {
        int cost = get_expand_cost();
        if (cost <= 0)
            return false; // 已至极限
        CurrencySystem *cur = CurrencySystem::get_singleton();
        if (!cur || !cur->spend(cost))
            return false; // 灵石不足
        _plot_count++;
        return true;
    }

    int DongtianManager::get_jlz_upgrade_cost() const {
        // 两级：上品×5（500 下品）/ 上品×15（1500 下品）
        static const int COSTS[JLZ_MAX_LEVEL] = { 500, 1500 };
        if (_jlz_level >= JLZ_MAX_LEVEL)
            return 0;
        return COSTS[_jlz_level];
    }

    bool DongtianManager::upgrade_jlz() {
        int cost = get_jlz_upgrade_cost();
        if (cost <= 0)
            return false; // 已至极限
        CurrencySystem *cur = CurrencySystem::get_singleton();
        if (!cur || !cur->spend(cost))
            return false; // 灵石不足
        _jlz_level++;
        return true;
    }

    // ============================================================
    // 仓库（储物槽自持于 Manager，随档持久化）
    // ============================================================

    Dictionary DongtianManager::get_storage_slot(int p_index) const {
        Dictionary d;
        ERR_FAIL_INDEX_V(p_index, STORAGE_SLOTS, d);
        const StorageSlot &s = _storage[p_index];
        if (s.item.is_empty() || s.qty <= 0)
            return d;
        d["id"] = s.item;
        d["quantity"] = s.qty;
        const Item *def = ItemDatabase::get_singleton()->get_item(s.item);
        d["name"] = def ? def->name : String(s.item);
        return d;
    }

    int DongtianManager::deposit_from_player(int p_inv_slot) {
        if (!_player || !_player->get_inventory())
            return 0;
        Inventory *inv = _player->get_inventory();
        Dictionary slot = inv->get_slot(p_inv_slot);
        StringName id = slot.get("id", StringName());
        int qty = int(slot.get("quantity", 0));
        if (id.is_empty() || qty <= 0)
            return 0;
        const Item *def = ItemDatabase::get_singleton()->get_item(id);
        int max_stack = def ? def->max_stack : 99;

        int remaining = qty;
        // 先叠放进同类槽
        for (int i = 0; i < STORAGE_SLOTS && remaining > 0; i++) {
            if (_storage[i].item != id)
                continue;
            int space = max_stack - _storage[i].qty;
            int moved = remaining < space ? remaining : space;
            _storage[i].qty += moved;
            remaining -= moved;
        }
        // 余下进空格
        for (int i = 0; i < STORAGE_SLOTS && remaining > 0; i++) {
            if (!_storage[i].item.is_empty())
                continue;
            int moved = remaining < max_stack ? remaining : max_stack;
            _storage[i].item = id;
            _storage[i].qty = moved;
            remaining -= moved;
        }

        int stored = qty - remaining;
        if (stored > 0)
            inv->remove_item(id, stored);
        return stored;
    }

    int DongtianManager::withdraw_to_player(int p_storage_slot) {
        ERR_FAIL_INDEX_V(p_storage_slot, STORAGE_SLOTS, 0);
        StorageSlot &s = _storage[p_storage_slot];
        if (s.item.is_empty() || s.qty <= 0 || !_player)
            return 0;
        StringName id = s.item;
        int qty = s.qty;
        if (!_player->get_inventory() || !_player->get_inventory()->add_item(id, qty))
            return 0; // 背包满，原样保留
        s.item = StringName();
        s.qty = 0;
        return qty;
    }

    // ============================================================
    // 灵植采集点（现实时间刷新；状态自持，场景卸载不丢）
    // ============================================================

    Dictionary DongtianManager::get_herb_spot(int p_index) const {
        Dictionary d;
        ERR_FAIL_INDEX_V(p_index, HERB_SPOTS, d);
        const HerbSpot &s = _herb_spots[p_index];
        const HerbSpotDef &def = HERB_SPOT_DEFS[p_index];
        StringName herb = s.herb.is_empty() ? StringName(def.herb) : s.herb;
        const Item *idef = ItemDatabase::get_singleton()->get_item(herb);
        int64_t elapsed = s.harvested_at > 0 ? _now() - s.harvested_at : def.refresh;
        bool available = s.harvested_at <= 0 || elapsed >= def.refresh;
        d["herb"] = herb;
        d["herb_name"] = idef ? idef->name : String(herb);
        d["qty"] = def.qty;
        d["refresh"] = def.refresh;
        d["available"] = available;
        d["remaining"] = available ? 0 : int(def.refresh - elapsed);
        return d;
    }

    bool DongtianManager::gather_herb_spot(int p_index) {
        ERR_FAIL_INDEX_V(p_index, HERB_SPOTS, false);
        Dictionary info = get_herb_spot(p_index);
        if (!bool(info.get("available", false)) || !_player)
            return false;
        StringName herb = info["herb"];
        _player->pickup_item(herb, int(info["qty"]));
        // 采集 = 练气行为（同 HerbNode，+2）
        if (_player->get_gongfa()) {
            _player->get_gongfa()->feed(GongfaSystem::SCHOOL_QI, 2.0f);
        }
        _herb_spots[p_index].herb = herb;
        _herb_spots[p_index].harvested_at = _now();
        return true;
    }

    void DongtianManager::debug_age_herb_spot(int p_index, double p_seconds) {
        ERR_FAIL_INDEX(p_index, HERB_SPOTS);
        if (_herb_spots[p_index].harvested_at > 0)
            _herb_spots[p_index].harvested_at -= int64_t(p_seconds);
    }

    Dictionary DongtianManager::save_to_dict() const {
        Dictionary d;
        d["plot_count"] = _plot_count;
        d["jlz_level"] = _jlz_level;
        Array plots;
        for (int i = 0; i < MAX_PLOTS; i++) {
            Dictionary p;
            p["herb"] = _plots[i].herb;
            p["planted_at"] = _plots[i].planted_at;
            plots.push_back(p);
        }
        d["plots"] = plots;
        Array storage;
        for (int i = 0; i < STORAGE_SLOTS; i++) {
            Dictionary s;
            s["id"] = _storage[i].item;
            s["qty"] = _storage[i].qty;
            storage.push_back(s);
        }
        d["storage"] = storage;
        Array herb_spots;
        for (int i = 0; i < HERB_SPOTS; i++) {
            Dictionary h;
            h["herb"] = _herb_spots[i].herb;
            h["harvested_at"] = _herb_spots[i].harvested_at;
            herb_spots.push_back(h);
        }
        d["herb_spots"] = herb_spots;
        return d;
    }

    void DongtianManager::load_from_dict(const Dictionary &p_data) {
        // v4 字段缺省走默认值（老档迁移安全）
        _plot_count = CLAMP(int(p_data.get("plot_count", BASE_PLOTS)), BASE_PLOTS, MAX_PLOTS);
        _jlz_level = CLAMP(int(p_data.get("jlz_level", 0)), 0, JLZ_MAX_LEVEL);
        Array plots = p_data.get("plots", Array());
        for (int i = 0; i < MAX_PLOTS && i < plots.size(); i++) {
            Dictionary p = plots[i];
            _plots[i].herb = StringName(p.get("herb", StringName()));
            _plots[i].planted_at = int64_t(p.get("planted_at", int64_t(0)));
        }
        Array storage = p_data.get("storage", Array());
        for (int i = 0; i < STORAGE_SLOTS && i < storage.size(); i++) {
            Dictionary s = storage[i];
            _storage[i].item = StringName(s.get("id", StringName()));
            _storage[i].qty = int(s.get("qty", 0));
        }
        // 灵植采集点（缺省走定义表默认 = 全部可采集，老档迁移安全）
        Array herb_spots = p_data.get("herb_spots", Array());
        for (int i = 0; i < HERB_SPOTS && i < herb_spots.size(); i++) {
            Dictionary h = herb_spots[i];
            StringName herb = StringName(h.get("herb", StringName()));
            _herb_spots[i].herb = herb.is_empty() ? StringName(HERB_SPOT_DEFS[i].herb) : herb;
            _herb_spots[i].harvested_at = int64_t(h.get("harvested_at", int64_t(0)));
        }
    }

} // namespace godot
