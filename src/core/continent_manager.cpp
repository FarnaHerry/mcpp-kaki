module;

#include "game_manager.h"
#include "../nodes/player.h"
#include "../utils/text.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

module mcpp_kaki.core;
import mcpp_kaki.cultivation;
import mcpp_kaki.utils;
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

std::vector<ContinentManager::Def> ContinentManager::s_defs;
bool ContinentManager::s_loaded = false;

void ContinentManager::ensure_loaded() {
	if (s_loaded) return;
	s_loaded = true;
	// Def 是 const char*——JSON 字符串必须有人持有。deque push_back 不搬动既有元素，
	// c_str() 不会悬空（vector 扩容搬 SSO 内联缓冲会 dangling，禁用）
	static std::deque<std::string> s_strings;
	auto own = [](const String &p_s) -> const char * {
		s_strings.push_back(std::string(p_s.utf8().get_data()));
		return s_strings.back().c_str();
	};
	SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	Node *scene = st ? st->get_current_scene() : nullptr;
	DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
	if (dl) {
		Array all = dl->get_all_continents();
		for (int i = 0; i < all.size(); i++) {
			Dictionary d = all[i];
			if (!d.has("id") || !d.has("scene")) continue;
			Def def;
			def.id = own(d["id"]);
			def.name = own(d.get("name", d["id"]));
			def.scene = own(d["scene"]);
			def.spawn_x = float(double(d.get("spawn_x", 150.0)));
			def.spawn_y = float(double(d.get("spawn_y", 200.0)));
			def.min_realm = int(d.get("min_realm", 0));
			def.desc = own(d.get("desc", String()));
			def.gate = own(d.get("gate", String()));
			s_defs.push_back(def);
		}
		// HashMap 遍历无序——云游页顺序按门槛境界稳定排序
		std::sort(s_defs.begin(), s_defs.end(),
				[](const Def &a, const Def &b) { return a.min_realm < b.min_realm; });
	}
	if (s_defs.empty()) { // JSON 不可用退回硬编码
		for (const Def &d : CONTINENT_DEFS) { s_defs.push_back(d); }
	}
}

void ContinentManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_current_id"), &ContinentManager::get_current_id);
	ClassDB::bind_method(D_METHOD("get_current_name"), &ContinentManager::get_current_name);
	ClassDB::bind_method(D_METHOD("is_unlocked", "id"), &ContinentManager::is_unlocked);
	ClassDB::bind_method(D_METHOD("can_travel", "id"), &ContinentManager::can_travel);
	ClassDB::bind_method(D_METHOD("travel_to", "id"), &ContinentManager::travel_to);
	ClassDB::bind_method(D_METHOD("travel_to_direct", "id"), &ContinentManager::travel_to_direct);
	ClassDB::bind_method(D_METHOD("complete_travel"), &ContinentManager::complete_travel);
	ClassDB::bind_method(D_METHOD("get_continent_list"), &ContinentManager::get_continent_list);
}

// 云海强渡（design/world-map.md）：跨洲必经之地，金丹飞行门控
static const char *YUNHAI_SCENE = "res://scenes/continents/yunhai.tscn";
static const Vector2 YUNHAI_SPAWN(60.0f, 180.0f);

// 跨场景会话态（云海等非洲场景时保持上一洲身份）——函数局部 static，
// 严禁文件级 static Dictionary/String 全局构造（引擎内存初始化前 segfault 教训）
static String &_last_continent_id() {
	static String s = "dongsheng";
	return s;
}

const ContinentManager::Def *ContinentManager::find_def(const String &p_id) {
	ensure_loaded(); for (const Def &d : s_defs) {
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
	bool matched = false;
	ensure_loaded();
	for (const Def &d : s_defs) {
		if (scene_path == d.scene) {
			_current_id = d.id;
			matched = true;
			break;
		}
	}
	if (matched) {
		_last_continent_id() = _current_id;
	} else {
		_current_id = _last_continent_id(); // 云海等过渡场景：保持上一洲身份
	}

	// 只在真正踏上洲土时播报（云海过渡不触发横幅）
	if (matched) {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("continent_changed", _current_id, get_current_name());
		}
	}
}

String ContinentManager::get_current_name() const {
	const Def *d = find_def(_current_id);
	return d ? LOC(d->name) : String();
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

	// 云海强渡：检查点改写到云海起云台，目的洲记入 cp.travel_dest
	// （随存档持久化——渡海途中存档/读档不丢目的地），登岸时 complete_travel 接力
	Dictionary data = gm->collect_save_data();
	Dictionary cp;
	cp["position_x"] = YUNHAI_SPAWN.x;
	cp["position_y"] = YUNHAI_SPAWN.y;
	cp["scene_path"] = String(YUNHAI_SCENE);
	cp["has_checkpoint"] = true;
	cp["travel_dest"] = p_id;
	data["checkpoint"] = cp;
	GameManager::set_travel_bridge(data);
	GameManager::set_travel_target(YUNHAI_SPAWN);
	gm->request_scene_change(String(YUNHAI_SCENE), YUNHAI_SPAWN);
	return true;
}

bool ContinentManager::travel_to_direct(const String &p_id) {
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

bool ContinentManager::complete_travel() {
	GameManager *gm = GameManager::get_singleton();
	if (!gm) return false;
	String dest = gm->get_travel_dest();
	const Def *d = dest.is_empty() ? nullptr : find_def(dest);
	if (!d) return false;

	// 登岸：检查点改写到目的洲，travel_dest 消耗掉（到岸即清）
	Dictionary data = gm->collect_save_data();
	Dictionary cp;
	cp["position_x"] = d->spawn_x;
	cp["position_y"] = d->spawn_y;
	cp["scene_path"] = String(d->scene);
	cp["has_checkpoint"] = true;
	cp["travel_dest"] = String();
	data["checkpoint"] = cp;
	GameManager::set_travel_bridge(data);
	GameManager::set_travel_target(Vector2(d->spawn_x, d->spawn_y));
	gm->request_scene_change(String(d->scene), Vector2(d->spawn_x, d->spawn_y));
	return true;
}

Array ContinentManager::get_continent_list() const {
	Array out;
	ensure_loaded();
	for (const Def &d : s_defs) {
		Dictionary c;
		c["id"] = String(d.id);
		c["name"] = LOC(d.name);
		c["desc"] = LOC(d.desc);
		c["min_realm"] = d.min_realm;
		c["gate"] = LOC(d.gate);
		c["unlocked"] = is_unlocked(d.id);
		c["current"] = _current_id == String(d.id);
		out.push_back(c);
	}
	return out;
}

} // namespace godot
