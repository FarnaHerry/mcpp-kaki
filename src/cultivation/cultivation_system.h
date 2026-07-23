#ifndef CPP_KAKI_CULTIVATION_SYSTEM_H
#define CPP_KAKI_CULTIVATION_SYSTEM_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../utils/text.h"

namespace godot {

	class CultivationSystem : public Object {
		GDCLASS(CultivationSystem, Object);

	public:
		// 凡尘九境（+凡人/渡劫）：修为经验全局累计（int64，9系门槛），不清零。
		// 仙阶：真仙 → 金仙（混元一气为金仙之上的特殊解锁）；天尊 NPC 专属。
		enum Realm {
			MORTAL = 0,           // 凡人
			QI_REFINING = 1,      // 炼气
			FOUNDATION = 2,       // 筑基
			GOLDEN_CORE = 3,      // 金丹
			NASCENT_SOUL = 4,     // 元婴
			SPIRIT_SEVERING = 5,  // 化神
			LIAN_XU = 6,          // 炼虚
			HE_TI = 7,            // 合体
			DA_CHENG = 8,         // 大乘
			DU_JIE = 9,           // 渡劫（大乘圆满后的过渡态，三灾连考）
			TRUE_IMMORTAL = 10,   // 真仙（大罗真仙/太乙真仙）
			GOLDEN_IMMORTAL = 11, // 金仙（大罗金仙/太乙金仙 → 混元一气）
			TIAN_ZUN = 12         // 天尊（NPC 专属，玩家不可达）
		};

		static const int REALM_COUNT = 13;

		// 境内分期（全境界通用）：经验区间平分三段，满 = 大圆满
		enum RealmStage {
			STAGE_EARLY = 0,      // 前期
			STAGE_MID = 1,        // 中期
			STAGE_LATE = 2,       // 后期
			STAGE_DA_YUANMAN = 3  // 大圆满
		};

		// 五仙身份 — 生活方式，与修为交叉（人仙 = 凡尘默认）
		enum ImmortalType {
			TYPE_NONE = 0,
			TYPE_GHOST = 1,   // 鬼仙
			TYPE_HUMAN = 2,   // 人仙
			TYPE_EARTH = 3,   // 地仙
			TYPE_SPIRIT = 4,  // 神仙
			TYPE_CELE = 5     // 天仙
		};

		// 门派 — 出身标签，纯称号无数值差异（大罗正统/太乙分支/散仙无门派）
		enum Sect {
			SECT_NONE = 0,
			SECT_DA_LUO = 1,   // 大罗（三清正统）
			SECT_TAI_YI = 2,   // 太乙（分支教派）
			SECT_SANXIAN = 3   // 散仙（无门无派）
		};

		// 出身 — 生死簿原始数据
		enum Origin {
			ORIGIN_MORTAL = 0,  // 后天修炼（根基扎实）
			ORIGIN_INNATE = 1   // 先天神圣（修为虚浮）
		};

		// 佛门果位 — 转修后的平行称号体系
		enum BuddhistRank {
			RANK_NONE = 0,
			RANK_LUO_HAN = 1,  // 罗汉（真仙~金仙）
			RANK_PU_SA = 2,    // 菩萨（金仙~混元）
			RANK_FO = 3        // 佛（混元~天尊级）
		};

		// 元婴分叉（元婴~炼虚，合体汇合）：肉身成圣 vs 元神修炼
		enum CultivationFocus {
			FOCUS_NONE = 0,
			FOCUS_BODY = 1,    // 肉身成圣（硬抗三灾倾向）
			FOCUS_SPIRIT = 2   // 元神修炼（躲避三灾倾向）
		};

		CultivationSystem();

		// ---- Current state ----
		Realm get_current_realm() const { return _current_realm; }
		String get_realm_name() const;
		String get_full_title() const; // 经 TitleComposer 组合的全称
		int get_realm_index() const { return (int)_current_realm; }
		bool is_immortal() const { return _current_realm >= TRUE_IMMORTAL; }

		// ---- 境内分期 ----
		RealmStage get_stage() const;
		String get_stage_name() const;

		// ---- 修为经验（全局累计 int64，9系门槛）----
		// 凡尘池 _lingqi，仙阶池 _xianyuan（九九归一重新起数）。HUD 只显示境内进度百分比。
		int64_t get_spiritual_energy() const { return _lingqi; }
		int64_t get_xianyuan() const { return _xianyuan; }
		int64_t get_current_energy() const { return is_immortal() ? _xianyuan : _lingqi; }
		int64_t get_max_energy() const;
		// 当前境内进度 0.0~1.0（渡劫/天尊无经验条，恒为 1）
		float get_realm_progress() const;

		// 获取经验（打坐/杀敌/丹药），路由到活跃池
		void accumulate_energy(double p_amount);

		// ---- 灵力（法力资源：放技能、催动法宝；凡人没有灵力）----
		// 仙阶同名仙元。上限随境界，缓慢自动回复（Player 每帧调 tick_mana_regen）。
		double get_mana() const { return _mana; }
		double get_max_mana() const;
		String get_mana_name() const { return is_immortal() ? TXT("仙元") : TXT("灵力"); }
		bool consume_mana(double p_cost);   // 不足则失败返回 false
		void restore_mana(double p_amount);
		void set_mana(double p_amount);     // save/load
		void tick_mana_regen(double p_delta);

		// Save/load 直写
		void set_spiritual_energy(int64_t p_amount);
		void set_xianyuan(int64_t p_amount);
		void set_realm(int p_realm);

		// 机缘突破：经验满级即可（无概率；三灾/心魔等事件内容后续做）
		bool attempt_breakthrough();

		// 调试开关：解除突破的经验限制（正式上线时关闭，恢复经验门槛）
		void set_free_breakthrough(bool p_enabled) { _free_breakthrough = p_enabled; }
		bool is_free_breakthrough() const { return _free_breakthrough; }

		// ---- 混元一气 / 天尊（特殊解锁）----
		bool is_hunyuan() const { return _hunyuan; }
		bool attain_hunyuan();   // 金仙大圆满 + 特殊结合（事件，暂直接解锁）
		void set_hunyuan(bool p_hunyuan); // save/load
		bool attain_tianzun();   // 需混元一气（NPC 级，隐藏）

		// ---- 五仙身份 ----
		ImmortalType get_immortal_type() const { return _immortal_type; }
		String get_immortal_type_name() const;
		bool choose_immortal_type(int p_type);
		void set_immortal_type(int p_type);

		// ---- 门派 ----
		Sect get_sect() const { return _sect; }
		String get_sect_name() const;
		bool choose_sect(int p_sect);   // 真仙起可选
		void set_sect(int p_sect);      // save/load

		// ---- 出身 ----
		Origin get_origin() const { return _origin; }
		String get_origin_name() const;
		void set_origin(int p_origin);

		// ---- 佛门果位 ----
		BuddhistRank get_buddhist_rank() const { return _buddhist_rank; }
		String get_buddhist_rank_name() const;
		void set_buddhist_rank(int p_rank);

		// ---- 元婴分叉 ----
		CultivationFocus get_focus() const { return _focus; }
		String get_focus_name() const;
		bool choose_focus(int p_focus);  // 元婴起可选
		void set_focus(int p_focus);     // save/load

		// ---- Stat multipliers（境界基础 × 期数加成；混元一气/天尊为固定高值）----
		float get_damage_multiplier() const;
		float get_defense_multiplier() const;
		float get_speed_multiplier() const;
		// 生命上限：100 × 防御倍率（随境界/期数成长）
		double get_max_health() const { return 100.0 * get_defense_multiplier(); }

		int64_t energy_to_next_realm() const;
		// 金仙起常规突破停止（混元一气特殊解锁）；天尊不可达
		bool is_max_realm() const { return _current_realm >= GOLDEN_IMMORTAL; }

		// 境界圆满门槛（累计经验上限）
		static int64_t get_realm_cap(Realm p_realm);

	protected:
		static void _bind_methods();

	private:
		Realm _current_realm = MORTAL;
		int64_t _lingqi = 0;     // 凡尘修为经验（全局累计）
		int64_t _xianyuan = 0;   // 仙阶修为经验（九九归一）
		double _mana = 0.0;      // 灵力（法力资源，非经验）
		ImmortalType _immortal_type = TYPE_HUMAN; // 凡尘默认人仙
		Sect _sect = SECT_NONE;
		Origin _origin = ORIGIN_MORTAL;
		BuddhistRank _buddhist_rank = RANK_NONE;
		CultivationFocus _focus = FOCUS_NONE;
		bool _hunyuan = false;   // 混元一气（金仙之上的特殊成就）
		bool _free_breakthrough = true; // 调试：突破不查经验（上线前改回 false）

		void _set_realm_internal(Realm p_realm);
		void _emit_energy_changed();
		void _emit_mana_changed();
		void _notify_name_changed();
	};

} // namespace godot

VARIANT_ENUM_CAST(godot::CultivationSystem::RealmStage);
VARIANT_ENUM_CAST(godot::CultivationSystem::ImmortalType);
VARIANT_ENUM_CAST(godot::CultivationSystem::Sect);
VARIANT_ENUM_CAST(godot::CultivationSystem::Origin);
VARIANT_ENUM_CAST(godot::CultivationSystem::BuddhistRank);
VARIANT_ENUM_CAST(godot::CultivationSystem::CultivationFocus);

#endif // CPP_KAKI_CULTIVATION_SYSTEM_H
