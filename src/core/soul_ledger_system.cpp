#include "soul_ledger_system.h"

#include "game_manager.h"
#include "../nodes/player.h"
#include "../nodes/enemy.h"

import mcpp_kaki.cultivation; // CultivationSystem（REALM_COUNT / get_realm_index / get_origin_name）
import mcpp_kaki.utils;       // SignalBus

#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace godot {

// ============================================================
// 生死簿数据
// ============================================================

int SoulLedgerSystem::lifespan_for_realm(int p_realm) {
	// 凡人100 / 炼气150 / 筑基250 / 金丹500 / 元婴2000 / 化神5000 / 炼虚8000
	// / 合体12000 / 大乘20000 / 渡劫50000 / 真仙100000 / 金仙200000
	// / 天尊（三清级，跳出五行）寿元无限（-1）——成仙后寿元正常，仅三清不入轮回
	static const int TABLE[CultivationSystem::REALM_COUNT] = {
		100, 150, 250, 500, 2000, 5000, 8000, 12000, 20000, 50000, 100000, 200000, 0
	};
	if (p_realm < 0)
		p_realm = 0;
	if (p_realm >= CultivationSystem::REALM_COUNT)
		p_realm = CultivationSystem::REALM_COUNT - 1;
	if (p_realm >= CultivationSystem::TIAN_ZUN)
		return LIFESPAN_INFINITE;
	return TABLE[p_realm];
}

void SoulLedgerSystem::set_player(Player *p) {
	_player = p;
	if (_player && _player->get_cultivation()) {
		_realm_cache = _player->get_cultivation()->get_realm_index();
	}
	_emit_lifespan();
}

void SoulLedgerSystem::set_ledger_lifespan(int v) {
	_ledger_lifespan = v;
	_emit_lifespan();
}

void SoulLedgerSystem::set_original_body(const String &v) {
	_original_body = v;
}

String SoulLedgerSystem::get_origin_name() const {
	if (_player && _player->get_cultivation())
		return _player->get_cultivation()->get_origin_name();
	return TXT("后天修炼");
}

bool SoulLedgerSystem::mark_soul_exempt() {
	if (_soul_protection && _struck)
		return false;
	_soul_protection = true;
	_struck = true; // 划名 = 脱离生死轮回（阴寿豁免：不再被勾魂）
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("soul_protection_changed", true);
	return true;
}

bool SoulLedgerSystem::consume_soul_protection() {
	if (!_soul_protection)
		return false;
	_soul_protection = false;
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("soul_protection_changed", false);
	return true;
}

bool SoulLedgerSystem::was_killed_by_reaper(Node *p_source) {
	Enemy *e = Object::cast_to<Enemy>(p_source);
	return e && e->is_soul_reaper;
}

Dictionary SoulLedgerSystem::save_to_dict() const {
	Dictionary d;
	d["original_body"] = _original_body;
	d["ledger_lifespan"] = _ledger_lifespan;
	d["soul_protection"] = _soul_protection;
	d["struck"] = _struck;
	return d;
}

void SoulLedgerSystem::load_from_dict(const Dictionary &p_data) {
	_original_body = p_data.get("original_body", _original_body);
	_ledger_lifespan = int(p_data.get("ledger_lifespan", _ledger_lifespan));
	_soul_protection = bool(p_data.get("soul_protection", false));
	_struck = bool(p_data.get("struck", false));
	// cultivation 段已先恢复，这里刷新实际寿元缓存并广播给 HUD
	if (_player && _player->get_cultivation()) {
		_realm_cache = _player->get_cultivation()->get_realm_index();
	}
	_emit_lifespan();
}

// ============================================================
// 生命周期
// ============================================================

void SoulLedgerSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_player", "player"), &SoulLedgerSystem::set_player);
	ClassDB::bind_method(D_METHOD("get_player"), &SoulLedgerSystem::get_player);
	ClassDB::bind_method(D_METHOD("get_ledger_lifespan"), &SoulLedgerSystem::get_ledger_lifespan);
	ClassDB::bind_method(D_METHOD("set_ledger_lifespan", "v"), &SoulLedgerSystem::set_ledger_lifespan);
	ClassDB::bind_method(D_METHOD("get_actual_lifespan"), &SoulLedgerSystem::get_actual_lifespan);
	ClassDB::bind_method(D_METHOD("get_original_body"), &SoulLedgerSystem::get_original_body);
	ClassDB::bind_method(D_METHOD("set_original_body", "v"), &SoulLedgerSystem::set_original_body);
	ClassDB::bind_method(D_METHOD("get_origin_name"), &SoulLedgerSystem::get_origin_name);
	ClassDB::bind_method(D_METHOD("has_soul_protection"), &SoulLedgerSystem::has_soul_protection);
	ClassDB::bind_method(D_METHOD("is_struck"), &SoulLedgerSystem::is_struck);
	ClassDB::bind_method(D_METHOD("mark_soul_exempt"), &SoulLedgerSystem::mark_soul_exempt);
	ClassDB::bind_method(D_METHOD("consume_soul_protection"), &SoulLedgerSystem::consume_soul_protection);
	ClassDB::bind_method(D_METHOD("is_reaper_active"), &SoulLedgerSystem::is_reaper_active);
	ClassDB::bind_method(D_METHOD("was_killed_by_reaper", "source"), &SoulLedgerSystem::was_killed_by_reaper);
	ClassDB::bind_method(D_METHOD("save_to_dict"), &SoulLedgerSystem::save_to_dict);
	ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &SoulLedgerSystem::load_from_dict);
	// 信号回调（Callable(this,"_on_*") 必须绑定，否则静默失效——CLAUDE.md 潜伏 bug 教训）
	ClassDB::bind_method(D_METHOD("_on_enemy_killed", "enemy", "killer"), &SoulLedgerSystem::_on_enemy_killed);
	ClassDB::bind_method(D_METHOD("_on_realm_changed", "old_realm", "new_realm", "name"), &SoulLedgerSystem::_on_realm_changed);
	ClassDB::bind_method(D_METHOD("_on_player_respawned"), &SoulLedgerSystem::_on_player_respawned);
}

void SoulLedgerSystem::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;
	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->connect("realm_changed", Callable(this, "_on_realm_changed"));
		bus->connect("enemy_killed", Callable(this, "_on_enemy_killed"));
		bus->connect("player_respawned", Callable(this, "_on_player_respawned"));
	}
	set_process(true);
}

void SoulLedgerSystem::_process(double p_delta) {
	SignalBus *bus = SignalBus::get_singleton();
	if (_tip_t > 0.0f) {
		_tip_t -= float(p_delta);
		if (_tip_t <= 0.0f && bus)
			bus->emit_signal("interaction_prompt", "", false);
	}

	if (!_player || _player->is_dead())
		return;
	if (_struck)
		return; // 划名=阴寿豁免：脱离生死轮回，不再被勾魂（勾魂错抓的终点）
	// 天尊（三清级，跳出五行，寿元无限）：不在生死簿可勾之列，不再刷勾魂使
	if (_player && _player->get_cultivation() &&
			_player->get_cultivation()->get_realm_index() >= CultivationSystem::TIAN_ZUN)
		return;
	Node *root = get_tree()->get_current_scene();
	if (!root)
		return;
	// 房间/秘境/洞天/地府内不刷勾魂使
	if (_player->get_parent() != root)
		return;
	if (root->get_scene_file_path() == String(GameManager::DIFU_SCENE))
		return;

	float max_h = _player->max_health;
	if (max_h <= 0.0f)
		return;
	float ratio = _player->current_health / max_h;

	// 回血 >50% 复位本轮触发（可再次濒死触发）
	if (ratio > 0.5f) {
		_reaper_spawned = false;
	}
	if (_reaper_active || _reaper_spawned)
		return;
	if (ratio < 0.2f) {
		_spawn_reapers();
	}
}

// ============================================================
// 勾魂使者（黑白无常）
// ============================================================

void SoulLedgerSystem::_spawn_reapers() {
	_reaper_spawned = true;
	_reaper_active = true;
	Node *root = get_tree()->get_current_scene();
	if (!root || !_player)
		return;
	Vector2 pp = _player->get_global_position();
	_reaper_a = _spawn_one_reaper(root, TXT("黑无常"), pp + Vector2(-70, -10), Color(0.12f, 0.10f, 0.16f));
	_reaper_b = _spawn_one_reaper(root, TXT("白无常"), pp + Vector2(70, -10), Color(0.88f, 0.88f, 0.94f));
	_show_tip(TXT("黑白无常来勾魂了！"), 2.0f);
}

Enemy *SoulLedgerSystem::_spawn_one_reaper(Node *root, const String &name, const Vector2 &pos, const Color &tint) {
	Enemy *e = memnew(Enemy);
	e->set_name(name);
	e->set_position(pos);
	e->is_soul_reaper = true;
	e->no_drops = true;   // 勾魂使不入掉落
	e->show_hp_bar = true;
	e->display_name = name;
	int realm = _player && _player->get_cultivation() ? _player->get_cultivation()->get_realm_index() : 0;
	e->realm = Math::max(1, realm); // 威压/灵压可生效
	float hp = _player ? _player->max_health : 100.0f;
	e->max_health = e->current_health = hp * 0.7f;
	e->attack_damage = Math::max(10.0f, hp * 0.12f); // 濒死 1~2 刀带走
	e->move_speed = 95.0f;
	e->detection_radius = 260.0f;
	e->attack_range = 36.0f;

	// 碰撞 + 视觉（黑白无常剪影，仿 BreakthroughManager::_spawn_wave）
	CollisionShape2D *shape = memnew(CollisionShape2D);
	Ref<CapsuleShape2D> cap;
	cap.instantiate();
	cap->set_radius(8.0f);
	cap->set_height(18.0f);
	shape->set_shape(cap);
	e->add_child(shape);

	Polygon2D *glow = memnew(Polygon2D);
	glow->set_color(Color(tint.r * 1.6f, tint.g * 1.6f, tint.b * 1.6f, 0.5f));
	PackedVector2Array glow_rect;
	glow_rect.push_back(Vector2(-11, -17));
	glow_rect.push_back(Vector2(11, -17));
	glow_rect.push_back(Vector2(11, 17));
	glow_rect.push_back(Vector2(-11, 17));
	glow->set_polygon(glow_rect);
	glow->set_z_index(-1);
	e->add_child(glow);

	Polygon2D *vis = memnew(Polygon2D);
	vis->set_color(tint);
	PackedVector2Array poly;
	poly.push_back(Vector2(-8, -14));
	poly.push_back(Vector2(8, -14));
	poly.push_back(Vector2(8, 14));
	poly.push_back(Vector2(-8, 14));
	vis->set_polygon(poly);
	e->add_child(vis);

	root->add_child(e);
	return e;
}

void SoulLedgerSystem::_on_enemy_killed(Node *enemy, Node *killer) {
	if (!_reaper_active)
		return;
	if (enemy == _reaper_a)
		_reaper_a = nullptr;
	if (enemy == _reaper_b)
		_reaper_b = nullptr;
	if (!_reaper_a && !_reaper_b) {
		_reaper_active = false;
		if (_player && killer)
			_player->gain_spiritual_energy(30.0f); // 反杀修为（Enemy 自带掉落另算）
		_show_tip(TXT("反杀勾魂使！地府暂记错抓。"), 2.5f);
	}
}

void SoulLedgerSystem::_on_realm_changed(int p_old, int p_new, const String &p_name) {
	_realm_cache = p_new;
	_emit_lifespan();
}

void SoulLedgerSystem::_on_player_respawned() {
	// 重生清理：清掉残留无常（防死后继续追击）+ 复位状态
	if (_reaper_a) {
		_reaper_a->queue_free();
		_reaper_a = nullptr;
	}
	if (_reaper_b) {
		_reaper_b->queue_free();
		_reaper_b = nullptr;
	}
	_reaper_active = false;
	_reaper_spawned = false;
	_tip_t = 0.0f;
}

void SoulLedgerSystem::_emit_lifespan() {
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("lifespan_changed", _ledger_lifespan, get_actual_lifespan());
}

void SoulLedgerSystem::_show_tip(const String &text, float seconds) {
	SignalBus *bus = SignalBus::get_singleton();
	if (bus)
		bus->emit_signal("interaction_prompt", text, true);
	_tip_t = seconds;
}

} // namespace godot
