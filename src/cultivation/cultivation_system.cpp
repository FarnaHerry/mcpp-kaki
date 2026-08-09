module;

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include "../utils/text.h"

module mcpp_kaki.cultivation;
import mcpp_kaki.utils;
namespace godot {

	// 渡劫成功、灵力转仙元：仙元清零重起数（九九归一，凡尘修为尽数归一）
	static constexpr int64_t INITIAL_XIANYUAN = 0;

	// 境界圆满门槛（9 为尊，累计经验上限；0 = 无经验条）
	// 凡尘九境全局累计；仙阶九九归一（仙元重新起数）
	// 平衡（session 011）：金丹起每境 ~3.3× 而非 ×10，打怪/清图路线可持续
	static const int64_t REALM_CAPS[CultivationSystem::REALM_COUNT] = {
		9,          // 凡人
		99,         // 炼气
		999,        // 筑基
		3999,       // 金丹
		13999,      // 元婴
		43999,      // 化神
		143999,     // 炼虚
		443999,     // 合体
		1443999,    // 大乘
		0,          // 渡劫（过渡态，无经验条）
		99999,      // 真仙（仙元 9 系门槛）
		999999,     // 金仙（仙元 9 系门槛；满=大圆满，混元一气为特殊解锁非经验堆叠）
		0           // 天尊
	};

	struct RealmStats { const char *name; float dmg, def, spd; };
	// 各境基础攻防速倍率（草案数值，后续平衡）
	static const RealmStats REALM_STATS[CultivationSystem::REALM_COUNT] = {
		{ "凡人", 1.0f, 1.0f, 1.0f },
		{ "炼气", 1.3f, 1.2f, 1.1f },
		{ "筑基", 1.7f, 1.5f, 1.2f },
		{ "金丹", 2.3f, 2.0f, 1.35f },
		{ "元婴", 3.2f, 2.8f, 1.5f },
		{ "化神", 4.5f, 4.0f, 1.7f },
		{ "炼虚", 6.0f, 5.5f, 1.9f },
		{ "合体", 8.5f, 8.0f, 2.1f },
		{ "大乘", 12.0f, 11.0f, 2.4f },
		{ "渡劫", 14.0f, 12.0f, 2.5f },
		{ "真仙", 20.0f, 18.0f, 2.8f },
		{ "金仙", 32.0f, 30.0f, 3.4f },  // 平衡：42/38 → 32/30，消除 2.1× 跳变
		{ "天尊", 60.0f, 50.0f, 4.5f },  // 平衡：100/100 → 60/50，HP 压回四位数
	};

	// 混元一气（玩家上限）固定倍率
	static constexpr float HUNYUAN_DMG = 70.0f;
	static constexpr float HUNYUAN_DEF = 65.0f;
	static constexpr float HUNYUAN_SPD = 4.5f;

	// 期数加成：中期×1.05 后期×1.10 大圆满×1.20
	static const float STAGE_FACTOR[4] = { 1.0f, 1.05f, 1.10f, 1.20f };

	CultivationSystem::CultivationSystem() {
	}

	void CultivationSystem::_bind_methods() {
		ClassDB::bind_method(D_METHOD("accumulate_energy", "amount"), &CultivationSystem::accumulate_energy);
		ClassDB::bind_method(D_METHOD("attempt_breakthrough"), &CultivationSystem::attempt_breakthrough);
		ClassDB::bind_method(D_METHOD("set_free_breakthrough", "enabled"), &CultivationSystem::set_free_breakthrough);
		ClassDB::bind_method(D_METHOD("is_free_breakthrough"), &CultivationSystem::is_free_breakthrough);
		ClassDB::bind_method(D_METHOD("get_realm_name"), &CultivationSystem::get_realm_name);
		ClassDB::bind_method(D_METHOD("get_full_title"), &CultivationSystem::get_full_title);
		ClassDB::bind_method(D_METHOD("get_realm_index"), &CultivationSystem::get_realm_index);
		ClassDB::bind_method(D_METHOD("is_immortal"), &CultivationSystem::is_immortal);
		ClassDB::bind_method(D_METHOD("get_stage"), &CultivationSystem::get_stage);
		ClassDB::bind_method(D_METHOD("get_stage_name"), &CultivationSystem::get_stage_name);
		ClassDB::bind_method(D_METHOD("get_spiritual_energy"), &CultivationSystem::get_spiritual_energy);
		ClassDB::bind_method(D_METHOD("get_xianyuan"), &CultivationSystem::get_xianyuan);
		ClassDB::bind_method(D_METHOD("get_current_energy"), &CultivationSystem::get_current_energy);
		ClassDB::bind_method(D_METHOD("get_max_energy"), &CultivationSystem::get_max_energy);
		ClassDB::bind_method(D_METHOD("get_realm_progress"), &CultivationSystem::get_realm_progress);
		ClassDB::bind_method(D_METHOD("get_mana"), &CultivationSystem::get_mana);
		ClassDB::bind_method(D_METHOD("get_max_mana"), &CultivationSystem::get_max_mana);
		ClassDB::bind_method(D_METHOD("get_mana_name"), &CultivationSystem::get_mana_name);
		ClassDB::bind_method(D_METHOD("consume_mana", "cost"), &CultivationSystem::consume_mana);
		ClassDB::bind_method(D_METHOD("restore_mana", "amount"), &CultivationSystem::restore_mana);
		ClassDB::bind_method(D_METHOD("set_mana", "amount"), &CultivationSystem::set_mana);
		ClassDB::bind_method(D_METHOD("tick_mana_regen", "delta"), &CultivationSystem::tick_mana_regen);
		ClassDB::bind_method(D_METHOD("get_law_power"), &CultivationSystem::get_law_power);
		ClassDB::bind_method(D_METHOD("get_law_power_max"), &CultivationSystem::get_law_power_max);
		ClassDB::bind_method(D_METHOD("consume_law_power", "cost"), &CultivationSystem::consume_law_power);
		ClassDB::bind_method(D_METHOD("restore_law_power", "amount"), &CultivationSystem::restore_law_power);
		ClassDB::bind_method(D_METHOD("set_law_power", "amount"), &CultivationSystem::set_law_power);
		ClassDB::bind_method(D_METHOD("tick_law_regen", "delta"), &CultivationSystem::tick_law_regen);
		ClassDB::bind_method(D_METHOD("get_damage_multiplier"), &CultivationSystem::get_damage_multiplier);
		ClassDB::bind_method(D_METHOD("get_defense_multiplier"), &CultivationSystem::get_defense_multiplier);
		ClassDB::bind_method(D_METHOD("get_speed_multiplier"), &CultivationSystem::get_speed_multiplier);
		ClassDB::bind_method(D_METHOD("get_max_health"), &CultivationSystem::get_max_health);
		ClassDB::bind_method(D_METHOD("energy_to_next_realm"), &CultivationSystem::energy_to_next_realm);
		ClassDB::bind_method(D_METHOD("is_max_realm"), &CultivationSystem::is_max_realm);
		ClassDB::bind_method(D_METHOD("set_spiritual_energy", "amount"), &CultivationSystem::set_spiritual_energy);
		ClassDB::bind_method(D_METHOD("set_xianyuan", "amount"), &CultivationSystem::set_xianyuan);
		ClassDB::bind_method(D_METHOD("set_realm", "realm"), &CultivationSystem::set_realm);
		ClassDB::bind_method(D_METHOD("is_hunyuan"), &CultivationSystem::is_hunyuan);
		ClassDB::bind_method(D_METHOD("attain_hunyuan"), &CultivationSystem::attain_hunyuan);
		ClassDB::bind_method(D_METHOD("set_hunyuan", "hunyuan"), &CultivationSystem::set_hunyuan);
		ClassDB::bind_method(D_METHOD("attain_tianzun"), &CultivationSystem::attain_tianzun);
		ClassDB::bind_method(D_METHOD("get_immortal_type"), &CultivationSystem::get_immortal_type);
		ClassDB::bind_method(D_METHOD("get_immortal_type_name"), &CultivationSystem::get_immortal_type_name);
		ClassDB::bind_method(D_METHOD("choose_immortal_type", "type"), &CultivationSystem::choose_immortal_type);
		ClassDB::bind_method(D_METHOD("set_immortal_type", "type"), &CultivationSystem::set_immortal_type);
		ClassDB::bind_method(D_METHOD("get_sect"), &CultivationSystem::get_sect);
		ClassDB::bind_method(D_METHOD("get_sect_name"), &CultivationSystem::get_sect_name);
		ClassDB::bind_method(D_METHOD("choose_sect", "sect"), &CultivationSystem::choose_sect);
		ClassDB::bind_method(D_METHOD("set_sect", "sect"), &CultivationSystem::set_sect);
		ClassDB::bind_method(D_METHOD("get_origin"), &CultivationSystem::get_origin);
		ClassDB::bind_method(D_METHOD("get_origin_name"), &CultivationSystem::get_origin_name);
		ClassDB::bind_method(D_METHOD("set_origin", "origin"), &CultivationSystem::set_origin);
		ClassDB::bind_method(D_METHOD("get_buddhist_rank"), &CultivationSystem::get_buddhist_rank);
		ClassDB::bind_method(D_METHOD("get_buddhist_rank_name"), &CultivationSystem::get_buddhist_rank_name);
		ClassDB::bind_method(D_METHOD("set_buddhist_rank", "rank"), &CultivationSystem::set_buddhist_rank);
		ClassDB::bind_method(D_METHOD("get_focus"), &CultivationSystem::get_focus);
		ClassDB::bind_method(D_METHOD("get_focus_name"), &CultivationSystem::get_focus_name);
		ClassDB::bind_method(D_METHOD("choose_focus", "focus"), &CultivationSystem::choose_focus);
		ClassDB::bind_method(D_METHOD("set_focus", "focus"), &CultivationSystem::set_focus);

		ADD_SIGNAL(MethodInfo("realm_changed",
		                      PropertyInfo(Variant::INT, "old_realm"),
		                      PropertyInfo(Variant::INT, "new_realm")));
		ADD_SIGNAL(MethodInfo("breakthrough_success",
		                      PropertyInfo(Variant::INT, "new_realm")));
		ADD_SIGNAL(MethodInfo("breakthrough_failed"));
		ADD_SIGNAL(MethodInfo("energy_changed",
		                      PropertyInfo(Variant::INT, "current"),
		                      PropertyInfo(Variant::INT, "max")));
		ADD_SIGNAL(MethodInfo("immortal_type_changed",
		                      PropertyInfo(Variant::INT, "type")));
		ADD_SIGNAL(MethodInfo("sect_changed",
		                      PropertyInfo(Variant::INT, "sect")));
	}

	int64_t CultivationSystem::get_realm_cap(Realm p_realm) {
		if (p_realm < 0 || p_realm >= REALM_COUNT)
			return 0;
		return REALM_CAPS[p_realm];
	}

	String CultivationSystem::get_realm_name() const {
		const char *base = REALM_STATS[_current_realm].name;

		if (_current_realm == TRUE_IMMORTAL || _current_realm == GOLDEN_IMMORTAL) {
			// 混元一气：混元 + 门派 + 金仙
			if (_hunyuan && _current_realm == GOLDEN_IMMORTAL) {
				if (_sect == SECT_SANXIAN)
					return LOC("混元散仙");
				return LOC("混元") + get_sect_name() + LOC("金仙");
			}
			// 门派称号：大罗真仙 / 太乙金仙；散仙无门派
			if (_sect == SECT_SANXIAN)
				return LOC("散仙");
			if (_sect != SECT_NONE)
				return get_sect_name() + LOC(base);
		}
		return LOC(base);
	}

	String CultivationSystem::get_full_title() const {
		return TitleComposer::compose(*this);
	}

	// 当前境界的经验区间 [lo, hi)：hi <= 0 表示无经验条（渡劫/天尊）
	static void _realm_xp_band(CultivationSystem::Realm p_realm, int64_t &r_lo, int64_t &r_hi) {
		r_hi = REALM_CAPS[p_realm];
		r_lo = 0;
		if (p_realm == CultivationSystem::GOLDEN_IMMORTAL) {
			r_lo = REALM_CAPS[CultivationSystem::TRUE_IMMORTAL];
		} else if (p_realm > CultivationSystem::MORTAL && p_realm != CultivationSystem::TRUE_IMMORTAL) {
			r_lo = REALM_CAPS[p_realm - 1];
		}
	}

	CultivationSystem::RealmStage CultivationSystem::get_stage() const {
		int64_t lo, hi;
		_realm_xp_band(_current_realm, lo, hi);
		if (hi <= 0)
			return STAGE_DA_YUANMAN; // 渡劫/天尊无经验条

		int64_t xp = get_current_energy();
		if (xp >= hi)
			return STAGE_DA_YUANMAN;

		int64_t range = hi - lo;
		int64_t pos = xp - lo;
		if (pos < 0) pos = 0;
		int64_t third = range / 3;
		if (pos < third)
			return STAGE_EARLY;
		if (pos < third * 2)
			return STAGE_MID;
		return STAGE_LATE;
	}

	float CultivationSystem::get_realm_progress() const {
		int64_t lo, hi;
		_realm_xp_band(_current_realm, lo, hi);
		if (hi <= 0)
			return 1.0f; // 渡劫/天尊无经验条

		int64_t range = hi - lo;
		if (range <= 0)
			return 1.0f;
		float pos = float(get_current_energy() - lo);
		return Math::clamp(pos / float(range), 0.0f, 1.0f);
	}

	String CultivationSystem::get_stage_name() const {
		switch (get_stage()) {
			case STAGE_EARLY:      return LOC("前期");
			case STAGE_MID:        return LOC("中期");
			case STAGE_LATE:       return LOC("后期");
			case STAGE_DA_YUANMAN: return LOC("大圆满");
		}
		return LOC("");
	}

	String CultivationSystem::get_immortal_type_name() const {
		switch (_immortal_type) {
			case TYPE_GHOST:  return LOC("鬼仙");
			case TYPE_HUMAN:  return LOC("人仙");
			case TYPE_EARTH:  return LOC("地仙");
			case TYPE_SPIRIT: return LOC("神仙");
			case TYPE_CELE:   return LOC("天仙");
			default:          return LOC("");
		}
	}

	String CultivationSystem::get_sect_name() const {
		switch (_sect) {
			case SECT_DA_LUO:  return LOC("大罗");
			case SECT_TAI_YI:  return LOC("太乙");
			case SECT_SANXIAN: return LOC("散仙");
			default:           return LOC("");
		}
	}

	String CultivationSystem::get_origin_name() const {
		switch (_origin) {
			case ORIGIN_INNATE: return LOC("先天神圣");
			default:            return LOC("后天修炼");
		}
	}

	String CultivationSystem::get_buddhist_rank_name() const {
		switch (_buddhist_rank) {
			case RANK_LUO_HAN: return LOC("罗汉");
			case RANK_PU_SA:   return LOC("菩萨");
			case RANK_FO:      return LOC("佛");
			default:           return LOC("");
		}
	}

	String CultivationSystem::get_focus_name() const {
		switch (_focus) {
			case FOCUS_BODY:   return LOC("肉身成圣");
			case FOCUS_SPIRIT: return LOC("元神修炼");
			default:           return LOC("");
		}
	}

	int64_t CultivationSystem::get_max_energy() const {
		return REALM_CAPS[_current_realm];
	}

	void CultivationSystem::_emit_energy_changed() {
		int64_t current = get_current_energy();
		int64_t max_e = get_max_energy();
		emit_signal("energy_changed", current, max_e);

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("spiritual_energy_changed", current, max_e, get_realm_progress());
		}
	}

	// ---- 灵力（法力资源）----

	double CultivationSystem::get_max_mana() const {
		// 凡人未引气入体，没有灵力；凡尘每境 +50；仙阶另起（草案数值）
		double base = 0.0;
		switch (_current_realm) {
			case MORTAL:          base = 0.0; break;
			case TRUE_IMMORTAL:   base = 1000.0; break;
			case GOLDEN_IMMORTAL: base = 2000.0; break;
			case TIAN_ZUN:        base = 9999.0; break;
			default:              base = double((int)_current_realm) * 50.0; break;
		}
		return base * _mana_max_mult; // 功法（练气）乘区
	}

	void CultivationSystem::_emit_law_changed() {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("law_power_changed", _law_power, get_law_power_max());
		}
	}

	bool CultivationSystem::consume_law_power(double p_cost) {
		if (p_cost <= 0.0) return true;
		if (_law_power < p_cost) return false;
		_law_power -= p_cost;
		_emit_law_changed();
		return true;
	}

	void CultivationSystem::restore_law_power(double p_amount) {
		double max_law = get_law_power_max();
		if (max_law <= 0.0) return;
		double v = Math::clamp(_law_power + p_amount, 0.0, max_law);
		if (v == _law_power) return;
		_law_power = v;
		_emit_law_changed();
	}

	void CultivationSystem::set_law_power(double p_amount) {
		_law_power = Math::clamp(p_amount, 0.0, get_law_power_max());
		_emit_law_changed();
	}

	void CultivationSystem::tick_law_regen(double p_delta) {
		if (get_law_power_max() <= 0.0) return;
		if (_law_power >= LAW_POWER_MAX) return;
		_law_power = Math::min(_law_power + LAW_REGEN_PER_SEC * _law_regen_mult * p_delta, LAW_POWER_MAX);
		_emit_law_changed();
	}

	void CultivationSystem::_emit_mana_changed() {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("mana_changed", _mana, get_max_mana());
		}
	}

	bool CultivationSystem::consume_mana(double p_cost) {
		if (p_cost <= 0.0)
			return true;
		if (_mana < p_cost)
			return false;
		double before = Math::floor(_mana);
		_mana -= p_cost;
		if (Math::floor(_mana) != before) {
			_emit_mana_changed(); // 整数值变化才发信号（飞行等持续消耗不刷屏）
		}
		return true;
	}

	void CultivationSystem::restore_mana(double p_amount) {
		double max_mana = get_max_mana();
		double new_mana = Math::clamp(_mana + p_amount, 0.0, max_mana);
		if (new_mana == _mana)
			return;
		_mana = new_mana;
		_emit_mana_changed();
	}

	void CultivationSystem::set_mana(double p_amount) {
		_mana = Math::clamp(p_amount, 0.0, get_max_mana());
		_emit_mana_changed();
	}

	void CultivationSystem::tick_mana_regen(double p_delta) {
		double max_mana = get_max_mana();
		if (max_mana <= 0.0 || _mana >= max_mana)
			return;
		// 缓慢回复：每秒 2% 上限（满蓝约 50 秒）；仅在整数值变化时发信号
		double before = Math::floor(_mana);
		_mana = Math::min(_mana + max_mana * 0.02 * _mana_regen_mult * p_delta, max_mana);
		if (Math::floor(_mana) != before) {
			_emit_mana_changed();
		}
	}

	// 境界未变但显示名变化（期数推进/门派/身份/混元）时通知 HUD 刷新
	void CultivationSystem::_notify_name_changed() {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("realm_changed", (int)_current_realm,
			                (int)_current_realm, get_full_title());
		}
	}

	void CultivationSystem::accumulate_energy(double p_amount) {
		RealmStage before = get_stage();
		// 经验到顶即卡在当前境界（设计：过机缘事件后才能继续累计）
		int64_t cap = REALM_CAPS[_current_realm];
		if (cap <= 0)
			return; // 渡劫/天尊无经验条
		if (is_immortal()) {
			_xianyuan = MIN(_xianyuan + (int64_t)p_amount, cap);
		} else {
			_lingqi = MIN(_lingqi + (int64_t)p_amount, cap);
		}
		_emit_energy_changed();
		if (get_stage() != before) {
			_notify_name_changed(); // 期数推进，刷新称号
		}
	}

	void CultivationSystem::set_spiritual_energy(int64_t p_amount) {
		_lingqi = p_amount;
		_emit_energy_changed();
	}

	void CultivationSystem::set_xianyuan(int64_t p_amount) {
		_xianyuan = p_amount;
		_emit_energy_changed();
	}

	void CultivationSystem::_set_realm_internal(Realm p_realm) {
		if (p_realm == _current_realm)
			return;

		Realm old = _current_realm;
		_current_realm = p_realm;

		// 渡劫成仙：灵力转仙元（九九归一——凡尘修为清零、仙元从零重起数），五仙身份重置待选择
		if (old < TRUE_IMMORTAL && _current_realm >= TRUE_IMMORTAL) {
			_lingqi = 0;
			_xianyuan = INITIAL_XIANYUAN;
			_immortal_type = TYPE_NONE;
		}

		// 突破后法力补满至新上限
		_mana = get_max_mana();

		// 化神「初触法则」：法则之力苏醒并补满
		if (old < SPIRIT_SEVERING && _current_realm >= SPIRIT_SEVERING) {
			_law_power = LAW_POWER_MAX;
			_emit_law_changed();
		}

		emit_signal("realm_changed", (int)old, (int)_current_realm);
		_emit_energy_changed();
		_emit_mana_changed();

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("realm_changed", (int)old, (int)_current_realm,
			                get_full_title());
		}
	}

	void CultivationSystem::set_realm(int p_realm) {
		if (p_realm < 0 || p_realm >= REALM_COUNT)
			return;
		_set_realm_internal(static_cast<Realm>(p_realm));
	}

	bool CultivationSystem::attempt_breakthrough() {
		// 渡劫期：三灾连考（独立事件，暂直接通过——后续做三灾关卡）
		if (_current_realm == DU_JIE) {
			emit_signal("breakthrough_success", (int)TRUE_IMMORTAL);
			_set_realm_internal(TRUE_IMMORTAL);
			return true;
		}

		if (is_max_realm())
			return false;

		// 经验门槛（调试开关 _free_breakthrough 打开时不检查，正式上线恢复）
		int64_t cap = REALM_CAPS[_current_realm];
		if (!_free_breakthrough && get_current_energy() < cap)
			return false;

		// 机缘突破：经验圆满即晋级（心魔劫/三尸劫等事件内容后续做）
		Realm next = (Realm)((int)_current_realm + 1);
		emit_signal("breakthrough_success", (int)next);
		_set_realm_internal(next);
		return true;
	}

	bool CultivationSystem::attain_hunyuan() {
		// 混元一气：金仙大圆满 + 某种结合（特殊事件，暂直接解锁）
		if (_current_realm != GOLDEN_IMMORTAL || _hunyuan)
			return false;
		if (get_stage() != STAGE_DA_YUANMAN)
			return false;
		_hunyuan = true;
		_notify_name_changed();
		return true;
	}

	void CultivationSystem::set_hunyuan(bool p_hunyuan) {
		if (_hunyuan == p_hunyuan)
			return;
		_hunyuan = p_hunyuan;
		_notify_name_changed();
	}

	bool CultivationSystem::attain_tianzun() {		// 天尊：规则级果位，需混元一气（隐藏内容，NPC 级）
		if (!_hunyuan)
			return false;
		_set_realm_internal(TIAN_ZUN);
		return true;
	}

	bool CultivationSystem::choose_immortal_type(int p_type) {
		// 五仙身份在真仙境选择
		if (_current_realm < TRUE_IMMORTAL)
			return false;
		if (p_type <= TYPE_NONE || p_type > TYPE_CELE)
			return false;
		set_immortal_type(p_type);
		return true;
	}

	void CultivationSystem::set_immortal_type(int p_type) {
		if (p_type < TYPE_NONE || p_type > TYPE_CELE)
			return;
		ImmortalType new_type = static_cast<ImmortalType>(p_type);
		if (new_type == _immortal_type)
			return;
		_immortal_type = new_type;
		emit_signal("immortal_type_changed", (int)_immortal_type);

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("immortal_type_changed", (int)_immortal_type,
			                get_immortal_type_name());
		}
		_notify_name_changed(); // 身份进入全称（如 太乙金仙·后期·地仙）
	}

	bool CultivationSystem::choose_sect(int p_sect) {
		// 门派在真仙境选择
		if (_current_realm < TRUE_IMMORTAL)
			return false;
		if (p_sect <= SECT_NONE || p_sect > SECT_SANXIAN)
			return false;
		set_sect(p_sect);
		return true;
	}

	void CultivationSystem::set_sect(int p_sect) {
		if (p_sect < SECT_NONE || p_sect > SECT_SANXIAN)
			return;
		Sect new_sect = static_cast<Sect>(p_sect);
		if (new_sect == _sect)
			return;
		_sect = new_sect;
		emit_signal("sect_changed", (int)_sect);

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("sect_changed", (int)_sect, get_sect_name());
		}
		// 门派改变称号（大罗金仙/太乙真仙），通知 HUD 刷新
		_notify_name_changed();
	}

	void CultivationSystem::set_origin(int p_origin) {
		if (p_origin < ORIGIN_MORTAL || p_origin > ORIGIN_INNATE)
			return;
		_origin = static_cast<Origin>(p_origin);
	}

	void CultivationSystem::set_buddhist_rank(int p_rank) {
		if (p_rank < RANK_NONE || p_rank > RANK_FO)
			return;
		BuddhistRank new_rank = static_cast<BuddhistRank>(p_rank);
		if (new_rank == _buddhist_rank)
			return;
		_buddhist_rank = new_rank;
		_notify_name_changed(); // 果位改变称号体系
	}

	bool CultivationSystem::choose_focus(int p_focus) {
		// 元婴期解锁元神修炼副本，开始分叉
		if (_current_realm < NASCENT_SOUL)
			return false;
		if (p_focus <= FOCUS_NONE || p_focus > FOCUS_SPIRIT)
			return false;
		set_focus(p_focus);
		return true;
	}

	void CultivationSystem::set_focus(int p_focus) {
		if (p_focus < FOCUS_NONE || p_focus > FOCUS_SPIRIT)
			return;
		_focus = static_cast<CultivationFocus>(p_focus);
	}

	float CultivationSystem::get_damage_multiplier() const {
		if (_hunyuan)
			return HUNYUAN_DMG;
		return REALM_STATS[_current_realm].dmg * STAGE_FACTOR[get_stage()];
	}

	float CultivationSystem::get_defense_multiplier() const {
		if (_hunyuan)
			return HUNYUAN_DEF;
		return REALM_STATS[_current_realm].def * STAGE_FACTOR[get_stage()];
	}

	float CultivationSystem::get_speed_multiplier() const {
		if (_hunyuan)
			return HUNYUAN_SPD;
		return REALM_STATS[_current_realm].spd * STAGE_FACTOR[get_stage()];
	}

	int64_t CultivationSystem::energy_to_next_realm() const {
		if (is_max_realm())
			return 0;
		return REALM_CAPS[_current_realm];
	}

} // namespace godot
