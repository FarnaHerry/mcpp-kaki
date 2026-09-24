module;

#include "../utils/text.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <string>

module mcpp_kaki.cultivation;
import mcpp_kaki.utils;
import mcpp_kaki.core;
import mcpp_kaki.combat;
namespace godot {

	// 静态 buff 定义表（design/alchemy.md：buff 时长 300s，同名刷新不叠加）
	static const BuffSystem::Def BUFF_DEFS[] = {
		// id              name        dur    atk    def    elem        抗性
		{ "buff_bing_xin", "冰心",     300.0f, 0.0f,  0.0f,  ELEM_SHUI,  0.15f }, // 冰心丹：水抗+15%
		{ "buff_chi_yan",  "赤焰",     300.0f, 0.15f, 0.0f,  ELEM_NONE,  0.0f  }, // 赤焰丹：攻击+15%
		{ "buff_jin_gang", "金刚",     300.0f, 0.0f,  0.20f, ELEM_NONE,  0.0f  }, // 金刚丹：防御+20%
		{ "buff_tu_dun",   "土盾",     12.0f,  0.0f,  0.30f, ELEM_NONE,  0.0f  }, // 土盾术：防御+30%（法术自buff）
		{ "buff_shen_wai", "身外化身", 30.0f,  0.35f, 0.0f,  ELEM_NONE,  0.0f  }, // 身外化身：毫毛助威，攻击+35%（神通自buff）
		{ "buff_xuan_gui", "玄龟护体", 30.0f,  0.0f,  0.25f, ELEM_NONE,  0.0f  }, // 玄龟护体：防御+25%（蓬莱法术自buff）
		// ---- 食物（design/cultivation-realms.md 饮食）：筑基辟谷后食物转纯 buff ----
		{ "buff_fullness_low",  "果腹", 600.0f,  0.0f,  0.05f, ELEM_NONE,  0.0f  }, // 糙米饭：防御+5%
		{ "buff_fullness_mid",  "干粮", 600.0f,  0.05f, 0.0f,  ELEM_NONE,  0.0f  }, // 干粮：攻击+5%
		{ "buff_fullness_high", "饱足", 900.0f,  0.08f, 0.08f, ELEM_NONE,  0.0f  }, // 灵米：攻击+8% 防御+8%
		{ "buff_hunger",        "饥饿", 99999.0f, -0.2f, -0.2f, ELEM_NONE,  0.0f  }, // 饥饿：饱食度归零（force-managed，吃食物解除）
		{ "buff_ren_shen_guo",  "人参果", 900.0f, 0.15f, 0.15f, ELEM_NONE,  0.0f  }, // 人参果：攻击+15% 防御+15%（五庄观镇观灵果）
		// ---- 北俱芦洲（极北莽荒，design/world-map.md v5）：炼体圣地 + 玄龙丹 ----
		{ "buff_lianti",    "炼体", 600.0f, 0.0f,  0.20f, ELEM_NONE,  0.0f  }, // 炼体圣地：防御+20%（极寒淬体，圣地交互）
		{ "buff_xuan_long", "玄龙", 900.0f, 0.20f, 0.20f, ELEM_NONE,  0.0f  }, // 玄龙丹：攻击+20% 防御+20%（渡劫灵丹，强于人参果）
		// ---- 北俱芦洲·寒墨行宫（旧魔宫 Boss 秘藏）----
		{ "buff_xuan_ming", "玄冥归元", 900.0f, 0.25f, 0.25f, ELEM_NONE,  0.0f  }, // 玄冥归元丹：攻击+25% 防御+25%（渡劫顶级灵丹，宫主镇宫之宝）
		// ---- 机缘突破战败惩罚 ----
		{ "buff_dao_xin_bu_wen", "道心不稳", 300.0f, -0.05f, -0.05f, ELEM_NONE, 0.0f }, // 心魔劫/三尸劫战败：攻击-5% 防御-5%
			// ---- 机缘叙事（仙人抚顶 / 醍醐灌顶）----
			{ "buff_chang_sheng", "长生", 900.0f, 0.15f, 0.15f, ELEM_NONE, 0.0f }, // 仙人抚顶：攻+15% 防+15%（太上老君授长生）
			{ "buff_ti_hu",      "醍醐", 900.0f, 0.10f, 0.10f, ELEM_NONE, 0.0f }, // 醍醐灌顶：攻+10% 防+10%（菩提佛法开悟）
		// ---- 丹毒（design/alchemy.md：同种丹 60s 内 ≥3 次积毒，apply_pill 内部施加）----
		{ "buff_dan_du",     "丹毒", 120.0f, -0.10f, -0.10f, ELEM_NONE, 0.0f }, // 丹毒：攻-10% 防-10%（连磕积毒，期间同种丹效果再减半）
		// ---- 大雷音寺遗址（Boss 心猿石像秘藏·旃檀功德香）----
		{ "buff_zhan_tan", "旃檀佛光", 600.0f, 0.0f, 0.0f, ELEM_NONE, 0.15f, true }, // 旃檀功德香：全元素抗性+15%
	};

	std::vector<BuffSystem::Def> BuffSystem::s_defs;
	bool BuffSystem::s_defs_loaded = false;

	void BuffSystem::ensure_defs_loaded() {
		if (s_defs_loaded) return;
		s_defs_loaded = true;
		static std::vector<std::string> s_strings; // c_str 持久化池（防悬垂）
		SceneTree *st = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
		Node *scene = st ? st->get_current_scene() : nullptr;
		DataLoader *dl = scene ? Object::cast_to<DataLoader>(scene->find_child("DataLoader", true, false)) : nullptr;
		if (dl) {
			Array all = dl->get_all_buffs();
			if (all.size() > 0) {
				s_defs.reserve(all.size());
				s_strings.reserve(all.size() * 2);
				for (int i = 0; i < all.size(); i++) {
					Dictionary d = all[i];
					s_strings.push_back(String(d["id"]).utf8().get_data());
					s_strings.push_back(String(d["name"]).utf8().get_data());
					Def def;
					def.id = s_strings[s_strings.size() - 2].c_str();
					def.name = s_strings[s_strings.size() - 1].c_str();
					def.duration = float(d["duration"]);
					def.atk_mult = float(d["atk_mult"]);
					def.def_mult = float(d["def_mult"]);
					def.elem = Element(int(d["elem"]));
					def.elem_resist = float(d["elem_resist"]);
					def.elem_all = bool(d.get("elem_all", false)); // 全元素抗性（旃檀佛光）
					s_defs.push_back(def);
				}
				return;
			}
		}
		for (const Def &d : BUFF_DEFS) { s_defs.push_back(d); }
	}

	const BuffSystem::Def *BuffSystem::find_def(const StringName &p_id) {
		ensure_defs_loaded();
		for (const Def &d : s_defs) {
			if (StringName(d.id) == p_id) return &d;
		}
		return nullptr;
	}

	void BuffSystem::_bind_methods() {
		ClassDB::bind_method(D_METHOD("apply", "id"), &BuffSystem::apply);
		ClassDB::bind_method(D_METHOD("apply_pill", "id"), &BuffSystem::apply_pill);
		ClassDB::bind_method(D_METHOD("remove", "id"), &BuffSystem::remove);
		ClassDB::bind_method(D_METHOD("clear"), &BuffSystem::clear);
		ClassDB::bind_method(D_METHOD("tick", "delta"), &BuffSystem::tick);
		ClassDB::bind_method(D_METHOD("has", "id"), &BuffSystem::has);
		ClassDB::bind_method(D_METHOD("get_atk_mult"), &BuffSystem::get_atk_mult);
		ClassDB::bind_method(D_METHOD("get_def_mult"), &BuffSystem::get_def_mult);
		ClassDB::bind_method(D_METHOD("get_elem_resist_bonus", "elem"), &BuffSystem::get_elem_resist_bonus);
		ClassDB::bind_method(D_METHOD("get_potency", "id"), &BuffSystem::get_potency);
		ClassDB::bind_method(D_METHOD("get_dose_count", "id"), &BuffSystem::get_dose_count);
		ClassDB::bind_method(D_METHOD("get_active_list"), &BuffSystem::get_active_list);
		ClassDB::bind_method(D_METHOD("save_to_dict"), &BuffSystem::save_to_dict);
		ClassDB::bind_method(D_METHOD("load_from_dict", "data"), &BuffSystem::load_from_dict);
	}

	bool BuffSystem::apply(const StringName &p_id) {
		const Def *def = find_def(p_id);
		if (!def) return false;
		// 同名刷新不叠加
		for (Active &a : _active) {
			if (a.id == p_id) {
				a.remaining = def->duration;
				a.potency = 1.0f; // 普通通道恒全效（丹药递减只走 apply_pill）
				_emit_changed();
				return true;
			}
		}
		Active a;
		a.id = p_id;
		a.remaining = def->duration;
		_active.push_back(a);
		_recalc();
		_emit_changed();
		return true;
	}

	// ============================================================
	// 丹毒数值调参外抽（data/tuning.json "dan_du" 段直读，仿 AffixDatabase 先例）
	// JSON 优先 + constexpr _DEF 兜底：逐键覆盖，键缺失/类型错→保留原常量值。
	// ============================================================

	void BuffSystem::_ensure_dose_tuning() {
		static bool s_loaded = false;
		if (s_loaded)
			return;
		s_loaded = true;
		const String path = "res://data/tuning.json";
		if (!FileAccess::file_exists(path))
			return; // JSON 不可用 → 全量兜底默认
		String raw = FileAccess::get_file_as_string(path);
		Variant parsed = JSON::parse_string(raw);
		if (parsed.get_type() != Variant::DICTIONARY) {
			UtilityFunctions::printerr(TXT("BuffSystem: tuning.json 顶层须为对象"));
			return;
		}
		Dictionary root = parsed;
		if (!root.has("dan_du"))
			return;
		Variant sec = root["dan_du"];
		if (sec.get_type() != Variant::DICTIONARY)
			return;
		Dictionary d = sec;
		auto get_num = [&d](const char *p_key, Variant &r_v) -> bool {
			if (!d.has(p_key))
				return false;
			r_v = d[p_key];
			return r_v.get_type() == Variant::FLOAT || r_v.get_type() == Variant::INT;
		};
		Variant v;
		if (get_num("dose_window", v)) DOSE_WINDOW = double(v);
		if (get_num("dose_toxic_at", v)) DOSE_TOXIC_AT = int(v);
		if (get_num("refresh_potency", v)) REFRESH_POTENCY = float(v);
		if (get_num("toxic_potency", v)) TOXIC_POTENCY = float(v);
	}

	// 丹药服用入口（丹毒机制，design/alchemy.md「成败与丹毒」）
	bool BuffSystem::apply_pill(const StringName &p_id) {
		const Def *def = find_def(p_id);
		if (!def) return false;
		_ensure_dose_tuning();
		_prune_doses();
		std::vector<double> &doses = _doses[p_id];
		doses.push_back(_time);
		const int count = (int)doses.size();

		float potency = 1.0f;
		// 连磕递减：同名 buff 剩余 >50% 再服 → 本次数值 6 折（持续同）
		for (const Active &a : _active) {
			if (a.id == p_id && a.remaining > def->duration * 0.5f) {
				potency *= REFRESH_POTENCY;
				break;
			}
		}
		// 丹毒期间再服同种丹：效果再减半
		const StringName DAN_DU = StringName("buff_dan_du");
		const bool toxic_same = has(DAN_DU) && _dan_du_source == p_id;
		if (toxic_same) potency *= TOXIC_POTENCY;

		// 上药/刷新（同名不叠加，potency 为本次实例数值倍率）
		bool found = false;
		for (Active &a : _active) {
			if (a.id == p_id) {
				a.remaining = def->duration;
				a.potency = potency;
				found = true;
				break;
			}
		}
		if (!found) {
			Active a;
			a.id = p_id;
			a.remaining = def->duration;
			a.potency = potency;
			_active.push_back(a);
		}

		// 积毒：窗口内同种丹 ≥3 次 → 上丹毒并记丹种；丹毒期同种丹刷新丹毒
		if (count >= DOSE_TOXIC_AT) {
			_dan_du_source = p_id;
			apply(DAN_DU);
		} else if (toxic_same) {
			apply(DAN_DU); // 刷新 120s
		}
		_recalc();
		_emit_changed();
		return true;
	}

	float BuffSystem::get_potency(const StringName &p_id) const {
		for (const Active &a : _active) {
			if (a.id == p_id) return a.potency;
		}
		return 1.0f;
	}

	int BuffSystem::get_dose_count(const StringName &p_id) {
		_prune_doses();
		const std::vector<double> *doses = _doses.getptr(p_id);
		return doses ? (int)doses->size() : 0;
	}

	void BuffSystem::_prune_doses() {
		_ensure_dose_tuning(); // DOSE_WINDOW 走 tuning.json（幂等）
		std::vector<StringName> empty_keys;
		for (auto &kv : _doses) {
			std::vector<double> &v = kv.value;
			size_t keep = 0;
			while (keep < v.size() && _time - v[keep] > DOSE_WINDOW) keep++;
			if (keep > 0) v.erase(v.begin(), v.begin() + (ptrdiff_t)keep);
			if (v.empty()) empty_keys.push_back(kv.key);
		}
		for (const StringName &k : empty_keys) _doses.erase(k);
	}

	void BuffSystem::remove(const StringName &p_id) {
		for (size_t i = 0; i < _active.size(); i++) {
			if (_active[i].id == p_id) {
				_active.erase(_active.begin() + i);
				_recalc();
				_emit_changed();
				return;
			}
		}
	}

	void BuffSystem::clear() {
		if (_active.empty()) return;
		_active.clear();
		_recalc();
		_emit_changed();
	}

	void BuffSystem::tick(double p_delta) {
		_time += p_delta; // 内部时钟：丹毒滑动窗口计时基准（无活跃 buff 也推进）
		_prune_doses();
		if (_active.empty()) return;
		bool expired = false;
		for (int i = (int)_active.size() - 1; i >= 0; i--) {
			_active[i].remaining -= (float)p_delta;
			if (_active[i].remaining <= 0.0f) {
				_active.erase(_active.begin() + i);
				expired = true;
			}
		}
		if (expired) {
			_recalc();
			_emit_changed();
		}
	}

	bool BuffSystem::has(const StringName &p_id) const {
		for (const Active &a : _active) {
			if (a.id == p_id) return true;
		}
		return false;
	}

	float BuffSystem::get_elem_resist_bonus(int p_elem) const {
		if (p_elem < 0 || p_elem >= ELEM_CAPACITY) return 0.0f;
		return _sum_elem[p_elem];
	}

	Array BuffSystem::get_active_list() const {
		Array out;
		for (const Active &a : _active) {
			const Def *def = find_def(a.id);
			Dictionary d;
			d["id"] = a.id;
			d["name"] = def ? LOC(def->name) : String(a.id);
			d["remaining"] = a.remaining;
			d["potency"] = a.potency;
			out.push_back(d);
		}
		return out;
	}

	Dictionary BuffSystem::save_to_dict() const {
		Dictionary d;
		Array arr;
		for (const Active &a : _active) {
			Dictionary e;
			e["id"] = a.id;
			e["remaining"] = a.remaining;
			e["potency"] = a.potency;
			arr.push_back(e);
		}
		d["active"] = arr;
		// 丹毒窗口：服药时刻（内部时钟基准）+ 丹毒丹种，读档后递减/积毒状态连续
		d["dose_time"] = _time;
		Dictionary doses;
		for (const auto &kv : _doses) {
			Array ts;
			for (double t : kv.value) ts.push_back(t);
			doses[String(kv.key)] = ts;
		}
		d["doses"] = doses;
		d["dan_du_source"] = String(_dan_du_source);
		return d;
	}

	void BuffSystem::load_from_dict(const Dictionary &p_data) {
		_active.clear();
		_doses.clear();
		_dan_du_source = StringName();
		_time = 0.0;
		if (p_data.has("active")) {
			Array arr = p_data["active"];
			for (int i = 0; i < arr.size(); i++) {
				Dictionary e = arr[i];
				StringName id = e["id"];
				if (!find_def(id)) continue; // 定义已删的 buff 不恢复
				Active a;
				a.id = id;
				a.remaining = e["remaining"];
				a.potency = e.has("potency") ? (float)(double)e["potency"] : 1.0f; // 老档缺省全效
				_active.push_back(a);
			}
		}
		if (p_data.has("dose_time")) _time = (double)p_data["dose_time"];
		if (p_data.has("doses")) {
			Dictionary doses = p_data["doses"];
			Array keys = doses.keys();
			for (int i = 0; i < keys.size(); i++) {
				StringName id = StringName(String(keys[i]));
				Array ts = doses[keys[i]];
				std::vector<double> v;
				for (int j = 0; j < ts.size(); j++) v.push_back((double)ts[j]);
				if (!v.empty()) _doses[id] = v;
			}
		}
		if (p_data.has("dan_du_source")) _dan_du_source = StringName(String(p_data["dan_du_source"]));
		_prune_doses();
		_recalc();
		_emit_changed();
	}

	void BuffSystem::_recalc() {
		_sum_atk = 0.0f;
		_sum_def = 0.0f;
		for (int i = 0; i < ELEM_CAPACITY; i++) _sum_elem[i] = 0.0f;
		for (const Active &a : _active) {
			const Def *def = find_def(a.id);
			if (!def) continue;
			_sum_atk += def->atk_mult * a.potency;   // potency：丹毒递减只影响本次实例数值
			_sum_def += def->def_mult * a.potency;
			if (def->elem_all) {
				// 全元素抗性（旃檀佛光）：elem_resist 作用于全部元素
				for (int i = 1; i < ELEM_CAPACITY; i++) _sum_elem[i] += def->elem_resist * a.potency;
			} else if (def->elem != ELEM_NONE && def->elem < ELEM_CAPACITY) {
				_sum_elem[def->elem] += def->elem_resist * a.potency;
			}
		}
	}

	void BuffSystem::_emit_changed() {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("buffs_changed", get_active_list());
		}
	}

} // namespace godot
