#include "buff_system.h"

#include "../utils/signal_bus.h"
#include "../utils/text.h"

namespace godot {

	// 静态 buff 定义表（design/alchemy.md：buff 时长 300s，同名刷新不叠加）
	static const BuffSystem::Def BUFF_DEFS[] = {
		// id              name        dur    atk    def    elem        抗性
		{ "buff_bing_xin", "冰心",     300.0f, 0.0f,  0.0f,  ELEM_SHUI,  0.15f }, // 冰心丹：水抗+15%
		{ "buff_chi_yan",  "赤焰",     300.0f, 0.15f, 0.0f,  ELEM_NONE,  0.0f  }, // 赤焰丹：攻击+15%
		{ "buff_jin_gang", "金刚",     300.0f, 0.0f,  0.20f, ELEM_NONE,  0.0f  }, // 金刚丹：防御+20%
		{ "buff_tu_dun",   "土盾",     12.0f,  0.0f,  0.30f, ELEM_NONE,  0.0f  }, // 土盾术：防御+30%（法术自buff）
	};

	const BuffSystem::Def *BuffSystem::find_def(const StringName &p_id) {
		for (const Def &d : BUFF_DEFS) {
			if (StringName(d.id) == p_id) return &d;
		}
		return nullptr;
	}

	void BuffSystem::_bind_methods() {
		ClassDB::bind_method(D_METHOD("apply", "id"), &BuffSystem::apply);
		ClassDB::bind_method(D_METHOD("remove", "id"), &BuffSystem::remove);
		ClassDB::bind_method(D_METHOD("clear"), &BuffSystem::clear);
		ClassDB::bind_method(D_METHOD("has", "id"), &BuffSystem::has);
		ClassDB::bind_method(D_METHOD("get_atk_mult"), &BuffSystem::get_atk_mult);
		ClassDB::bind_method(D_METHOD("get_def_mult"), &BuffSystem::get_def_mult);
		ClassDB::bind_method(D_METHOD("get_elem_resist_bonus", "elem"), &BuffSystem::get_elem_resist_bonus);
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
			d["name"] = def ? TXT(def->name) : String(a.id);
			d["remaining"] = a.remaining;
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
			arr.push_back(e);
		}
		d["active"] = arr;
		return d;
	}

	void BuffSystem::load_from_dict(const Dictionary &p_data) {
		_active.clear();
		if (p_data.has("active")) {
			Array arr = p_data["active"];
			for (int i = 0; i < arr.size(); i++) {
				Dictionary e = arr[i];
				StringName id = e["id"];
				if (!find_def(id)) continue; // 定义已删的 buff 不恢复
				Active a;
				a.id = id;
				a.remaining = e["remaining"];
				_active.push_back(a);
			}
		}
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
			_sum_atk += def->atk_mult;
			_sum_def += def->def_mult;
			if (def->elem != ELEM_NONE && def->elem < ELEM_CAPACITY) {
				_sum_elem[def->elem] += def->elem_resist;
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
