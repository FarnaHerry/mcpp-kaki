#ifndef CPP_KAKI_PLAYER_H
#define CPP_KAKI_PLAYER_H

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/variant/color.hpp>

#include "../combat/combo_chain.h"
#include "../combat/damage_types.h"
#include "../cultivation/gongfa_system.h"
#include "../cultivation/buff_system.h"
#include "../cultivation/alchemy_system.h"
#include "../inventory/inventory.h"
#include "../utils/state_machine.h"
#include "../utils/input_buffer.h"

namespace godot {

	class HitBox;
	class HurtBox;
	class CultivationSystem;
	class AbilityManager;
	class SkillSystem;
	class ArtifactSystem;

	class Player : public CharacterBody2D {
		GDCLASS(Player, CharacterBody2D);

	public:
		static constexpr int EQUIP_SLOT_WEAPON = 0;
		static constexpr int EQUIP_SLOT_ARMOR = 1;
		static constexpr int EQUIP_SLOT_ACCESSORY = 2;
		static constexpr int EQUIP_SLOT_COUNT = 3;

		// Movement
		float move_speed = 180.0f;
		float base_move_speed = 180.0f; // original speed before equipment bonuses
		float jump_velocity = -350.0f;
		float jump_cut_multiplier = 0.5f;
		float dash_speed = 500.0f;
		float dash_duration = 0.15f;
		float dash_cooldown = 0.4f;
		float wall_slide_speed = 80.0f;
		float wall_jump_horizontal = 200.0f;
		float wall_jump_vertical = -320.0f;
		float coyote_time = 0.08f;
		float jump_buffer_time = 0.1f;
		float air_horizontal_multiplier = 0.8f;

		// Combat
		float max_health = 100.0f;
		float current_health = 100.0f;
		float attack_damage = 10.0f;

		// 抗性剖面（DamageCalculator 统一结算；defense 来自装备×境界，见 take_damage）
		float spell_resist = 0.0f;               // 法术抗性（比例）
		float elem_resist[ELEM_CAPACITY] = {};   // 元素抗性（比例）
		Element self_element = ELEM_NONE;        // 自身五行（灵根挂钩预留）

		int facing_direction = 1;

		// Accessors
		float get_gravity() const;
		float get_move_input() const;
		bool jump_just_pressed() const;
		bool jump_held() const;
		bool dash_just_pressed() const;
		bool attack_just_pressed() const;
		bool attack_held() const;
		bool can_dash() const;
		void start_dash();
		double get_time() const { return _time; }

		// Flight — 筑基借飞行法器（耗灵力），金丹以上无条件飞行（无消耗）
		bool can_fly() const;
		float get_fly_input() const; // 垂直输入：up=-1 down=+1
		float flight_mana_cost_per_sec() const;
		float fly_speed = 260.0f;          // 飞行极速（渐加速到达）
		float fly_acceleration = 250.0f;   // 飞行加速度（px/s²，低 = 起步慢、逐渐加快）
		bool was_flying = false;           // 攻击/冲刺等动作后是否恢复飞行

		// 赑风（三灾之一）：神魂受扰，水平输入反转（由 TribulationController 设置）
		bool input_inverted = false;

		void take_damage(float p_amount, Node *p_source);
		void take_damage_typed(float p_amount, int p_cat, int p_elem, Node *p_source); // 投射物用
		void take_hit(const HitBox *p_hitbox, Node *p_source); // HitBox 驱动（含伤害类别/元素）
		float get_effective_attack() const;
		bool is_dead() const { return current_health <= 0.0f; }
		float get_current_health() const { return current_health; }
		float get_max_health() const { return max_health; }
		void set_current_health(float p_v) { current_health = p_v; }

		StateMachine<Player> *state_machine = nullptr;
		InputBuffer jump_buffer;
		InputBuffer dash_buffer;
		InputBuffer attack_buffer;

		double dash_end_time = 0.0;
		double dash_cooldown_end = 0.0;
		double left_ground_time = -1.0;

		// Combo system
		ComboChain combo_chain;
		double attack_phase_end_time = 0.0; // when current attack phase ends
		enum AttackPhase { STARTUP, ACTIVE, RECOVERY };
		AttackPhase attack_phase = ACTIVE;

		// Cultivation
		CultivationSystem *get_cultivation() const { return _cultivation; }
		AbilityManager *get_ability_manager() const { return _abilities; }
		GongfaSystem *get_gongfa() const { return _gongfa; }
		SkillSystem *get_skills() const { return _skills; }
		ArtifactSystem *get_artifacts() const { return _artifacts; }
		BuffSystem *get_buffs() const { return _buffs; }
		AlchemySystem *get_alchemy() const { return _alchemy; }
		// B 键技能页：0=战斗页(A/S武技 D/F法术) 1=法宝页(A~H=法宝槽0..5)
		int get_skill_page() const { return _skill_page; }
		void toggle_skill_page();
		void gain_spiritual_energy(float p_amount);

		// 法宝系统（本命法宝：120%→150%温养 → 渡劫觉醒200%）
		void set_benming_artifact(const StringName &p_item_id);
		StringName get_benming_artifact() const { return _benming_item; }
		float get_benming_coeff() const;
		float get_benming_nurture() const { return _benming_nurture; }
		bool is_benming_awakened() const { return _benming_awakened; }
		void nurture_benming(float p_amount);
		void awaken_benming_artifact();
		// 法宝栏位：飞升前 1本命+2次要，飞升后 1本命+5次要（次要法宝待物品定义）
		int get_artifact_slot_limit() const;

		// Inventory
		Inventory *get_inventory() const { return _inventory; }
		void pickup_item(const StringName &p_item_id, int p_qty = 1);
		// 消耗品唯一入口：扣数量 + 应用全部效果（回血/回灵/修为/buff），拾取自动用/背包面板/快捷栏统一走这里
		bool use_consumable(const StringName &p_item_id);

		// 数字键消耗品栏（design/alchemy.md S6：1~6 快捷栏，拾取消耗品自动入栏首个空位）
		static constexpr int CONSUMABLE_BAR_SLOTS = 6;
		StringName get_consumable_bar_slot(int p_idx) const;
		bool use_consumable_bar_slot(int p_idx); // 数字键直接磕（绕过背包）

		// Equipment
		bool equip_item(int p_inventory_slot);
		bool unequip_item(int p_equip_slot);
		StringName get_equipment_in_slot(int p_slot) const;
		float get_equip_bonus_attack() const;
		float get_equip_bonus_defense() const;
		float get_equip_bonus_speed() const;

		// 技能施放出口（SkillSystem 调用；伤害类别/元素经 DamageCalculator 结算）
		void exec_skill_melee(float p_power, DamageCategory p_cat, Element p_elem);
		void exec_skill_lunge(float p_power, DamageCategory p_cat, Element p_elem);
		void exec_skill_projectile(float p_power, DamageCategory p_cat, Element p_elem,
		                           float p_speed, const Color &p_color);
		void exec_skill_blink(float p_distance); // 神通·缩地成寸（碰撞安全瞬移）
		void exec_skill_aoe(float p_power, DamageCategory p_cat, Element p_elem);     // 旋风斩：双向大范围
		void exec_skill_rising(float p_power, DamageCategory p_cat, Element p_elem);  // 升龙击：上跃+一挥
		void exec_skill_self_buff(const StringName &p_buff_id);                       // 土盾术：BuffSystem
		void exec_skill_proj_fan(float p_power, DamageCategory p_cat, Element p_elem,
		                         float p_speed, const Color &p_color);                // 御剑术：3 发扇形
		void exec_skill_invuln(float p_seconds);                                      // 金刚不坏：短时无敌
		bool is_invulnerable() const { return _time < _invuln_until; }

		// Save / Load
		void apply_save_data(const Dictionary &p_data);

		// Called when HitBox lands a hit (for combo tracking)
		void on_attack_landed(Node *p_victim, float p_damage);

		void _ready() override;
		void _physics_process(double p_delta) override;
		void _process(double p_delta) override;
		void _on_hurtbox_hit(Object *p_hitbox, Node *p_source);

	protected:
		static void _bind_methods();

	private:
		double _time = 0.0;
		HitBox *_hitbox = nullptr;
		HurtBox *_hurtbox = nullptr;
		CultivationSystem *_cultivation = nullptr;
		AbilityManager *_abilities = nullptr;
		GongfaSystem *_gongfa = nullptr;
		SkillSystem *_skills = nullptr;
		ArtifactSystem *_artifacts = nullptr;
		BuffSystem *_buffs = nullptr;
		AlchemySystem *_alchemy = nullptr;
		StringName _consumable_bar[CONSUMABLE_BAR_SLOTS]; // 数字键快捷栏（空 = StringName()）
		int _skill_page = 0;
		Inventory *_inventory = nullptr;
		double _skill_hitbox_until = 0.0;
		double _invuln_until = 0.0;   // 金刚不坏无敌截止（神通）
		bool _skill_hitbox_aoe = false; // AOE 借用 HitBox 后需还原变换 // 技能借用 HitBox 的关闭时刻（_time 时基）
		bool _interact_prompt_active = false; // 附近有可交互物（X 交互优先于普攻）
		void _on_interaction_prompt(const String &p_text, bool p_show);

		void _refresh_max_health(bool p_refill);
		void _on_gongfa_changed();
		void _on_skills_changed();
		void _refresh_regen_mults();
		void _on_enemy_killed(Object *p_enemy, Object *p_killer);
		void _take_damage_typed(float p_amount, DamageCategory p_cat, Element p_elem, Node *p_source);

		// Equipment slots: 0=weapon, 1=armor, 2=accessory
		StringName _equipment[3];

		// 本命法宝
		StringName _benming_item;
		float _benming_nurture = 0.0f;   // 温养进度（0~1000）
		bool _benming_awakened = false;  // 渡劫成功觉醒（150%→200%）

		void _update_buffers();
		void _update_facing();
		void _update_move_speed();
		void _create_hitboxes();
		void _setup_collision();
		void _create_cultivation();
		void _create_inventory();
		void _on_ability_unlocked(const StringName &p_ability_id);
		void _on_cultivation_realm_changed(int p_old_realm, int p_new_realm);
	};

} // namespace godot

#endif // CPP_KAKI_PLAYER_H
