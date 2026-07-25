#include "continent_manager.h"

#include "game_manager.h"
#include "../cultivation/cultivation_system.h"
#include "../nodes/player.h"
#include "../utils/signal_bus.h"
#include "../utils/text.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// 四大部洲注册表（design/world-map.md）：东胜神洲开局，金丹云海入西牛贺，
// 炼虚入南赡部，渡劫入北俱芦。
static const ContinentManager::Def CONTINENT_DEFS[] = {
	{ "dongsheng", "东胜神洲", "res://scenes/main.tscn", 200.0f, 200.0f, 0,
	  "傲来国地界，花果山临东海——西游起笔之地。", "" },
	{ "xiniuhe", "西牛贺洲", "res://scenes/continents/xiniuhe.tscn", 150.0f, 200.0f, 3,
	  "佛门故地。火焰山八百里，灵台方寸有仙踪。", "金丹（云海强渡）" },
	{ "nanzhanbu", "南赡部洲", "res://scenes/continents/nanzhanbu.tscn", 150.0f, 200.0f, 6,
	  "人间王朝。长安繁华，地府入口隐于此。", "炼虚" },
	{ "beijulu", "北俱芦洲", "res://scenes/continents/beijulu.tscn", 150.0f, 200.0f, 9,
	  "极北莽荒。玄冰巨兽，渡劫后方敢踏足。", "渡劫" },
};

void ContinentManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_current_id"), &ContinentManager::get_current_id);
	ClassDB::bind_method(D_METHOD("get_current_name"), &ContinentManager::get_current_name);
	ClassDB::bind_method(D_METHOD("is_unlocked", "id"), &ContinentManager::is_unlocked);
	ClassDB::bind_method(D_METHOD("can_travel", "id"), &ContinentManager::can_travel);
	ClassDB::bind_method(D_METHOD("travel_to", "id"), &ContinentManager::travel_to);
	ClassDB::bind_method(D_METHOD("get_continent_list"), &ContinentManager::get_continent_list);
}

const ContinentManager::Def *ContinentManager::find_def(const String &p_id) {
	for (const Def &d : CONTINENT_DEFS) {
		if (p_id == d.id) return &d;
	}
	return nullptr;
}

void ContinentManager::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		return;

	// 当前洲 = 当前根场景路径反推（main.tscn 即东胜神洲）
	Node *cur = get_tree()->get_current_scene();
	String scene_path = cur ? cur->get_scene_file_path() : String();
	for (const Def &d : CONTINENT_DEFS) {
		if (scene_path == d.scene) {
			_current_id = d.id;
			break;
		}
	}
	if (_current_id.is_empty()) {
		_current_id = "dongsheng"; // 未知场景（秘境等）按本洲计
	}

	SignalBus *bus = SignalBus::get_singleton();
	if (bus) {
		bus->emit_signal("continent_changed", _current_id, get_current_name());
	}
}

String ContinentManager::get_current_name() const {
	const Def *d = find_def(_current_id);
	return d ? TXT(d->name) : String();
}

Player *ContinentManager::_find_player() const {
	GameManager *gm = GameManager::get_singleton();
	return gm ? gm->get_player() : nullptr;
}

bool ContinentManager::is_unlocked(const String &p_id) const {
	const Def *d = find_def(p_id);
	if (!d) return false;
	Player *p = _find_player();
	if (!p || !p->get_cultivation()) return d->min_realm == 0;
	return p->get_cultivation()->get_realm_index() >= d->min_realm;
}

bool ContinentManager::can_travel(const String &p_id) const {
	return p_id != _current_id && is_unlocked(p_id);
}

bool ContinentManager::travel_to(const String &p_id) {
	const Def *d = find_def(p_id);
	if (!d || !can_travel(p_id)) return false;
	GameManager *gm = GameManager::get_singleton();
	if (!gm) return false;

	// 存档桥：全量状态 → 新场景 GameManager 应用（满血到岸，落点=洲 spawn）
	Dictionary data = gm->collect_save_data();
	// 检查点改写到目的地（防死亡回上一洲；新场景 _process 也会用 travel spawn 覆盖）
	Dictionary cp;
	cp["position_x"] = d->spawn_x;
	cp["position_y"] = d->spawn_y;
	cp["scene_path"] = String(d->scene);
	cp["has_checkpoint"] = true;
	data["checkpoint"] = cp;
	GameManager::set_travel_bridge(data);
	GameManager::set_travel_target(Vector2(d->spawn_x, d->spawn_y));
	gm->request_scene_change(String(d->scene), Vector2(d->spawn_x, d->spawn_y));
	return true;
}

Array ContinentManager::get_continent_list() const {
	Array out;
	for (const Def &d : CONTINENT_DEFS) {
		Dictionary c;
		c["id"] = String(d.id);
		c["name"] = TXT(d.name);
		c["desc"] = TXT(d.desc);
		c["min_realm"] = d.min_realm;
		c["gate"] = TXT(d.gate);
		c["unlocked"] = is_unlocked(d.id);
		c["current"] = _current_id == String(d.id);
		out.push_back(c);
	}
	return out;
}

} // namespace godot
