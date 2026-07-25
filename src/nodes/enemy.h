#ifndef CPP_KAKI_ENEMY_H
#define CPP_KAKI_ENEMY_H

#include <godot_cpp/classes/character_body2d.hpp>

#include "../combat/damage_types.h"
#include "../utils/state_machine.h"

namespace godot {

	class HitBox;
	class HurtBox;
	class ColorRect;
	class Node2D;

	class Enemy : public CharacterBody2D {
		GDCLASS(Enemy, CharacterBody2D);

	public:
		// Stats
		float max_health = 1.0f;
		float current_health = 1.0f;
		float move_speed = 60.0f;
		float detection_radius = 200.0f;
		float attack_range = 40.0f;
		float attack_damage = 10.0f;
		float attack_cooldown = 0.8f;
		float knockback_resistance = 1.0f;

		// 抗性剖面（DamageCalculator 统一结算）
		float defense = 0.0f;                    // 物理平减
		float spell_resist = 0.0f;               // 法术抗性（比例）
		float elem_resist[ELEM_CAPACITY] = {};   // 元素抗性（比例）
		Element self_element = ELEM_NONE;        // 自身五行（被克制判定）

		// Behavior flags
		bool is_ranged = false;       // true = archer type, keeps distance & shoots projectiles
		bool is_flying = false;       // true = ignores gravity, hovers
		bool is_boss = false;         // true = boss: more HP, phases, special attacks
		bool no_drops = false;        // true = 死亡不掉落（心魔/三尸等幻境之敌）
		bool show_hp_bar = false;     // true = Boss 血条上 HUD（秘境劫敌用；is_boss 同效）
		String display_name;          // Boss 血条标题（空则回退节点名）
		float preferred_distance = 0.0f; // ideal combat range (0 = melee)

		// Boss
		int boss_phase = 1;
		float boss_phase2_threshold = 0.5f;
		float special_attack_cooldown = 3.0f;

		// Facing (-1 or 1)
		int facing_direction = -1;

		// References
		StateMachine<Enemy> *state_machine = nullptr;
		Node2D *_player_target = nullptr;

		// Attack cooldown timers
		double last_attack_time = -999.0;
		double last_special_time = -999.0;
		double fly_phase_time = 0.0;     // for sine-wave hover
		double last_dive_time = -999.0;

		// Accessors for states
		float get_gravity() const;
		Node2D *get_player_target() const;
		bool can_see_player() const;
		bool player_in_attack_range() const;
		bool player_too_close() const;        // true if player is inside flee distance
		bool player_at_preferred_range() const; // true if at ideal ranged distance
		bool can_attack() const;
		bool can_special() const;
		void update_facing_to_player();

		void _activate_boss_hud();         // aggro/受击时上报 HUD Boss 条（状态类需访问，公开）
		bool _boss_hud_active = false;     // 已上报 HUD Boss 条（避免重复 started）

		void take_damage(float p_amount, Node *p_source);
		void take_damage_typed(float p_amount, int p_cat, int p_elem, Node *p_source); // 投射物用（Variant 可传 int）
		void take_hit(const HitBox *p_hitbox, Node *p_source); // HitBox 驱动（含伤害类别/元素）
		bool is_dead() const { return current_health <= 0.0f; }
		float get_current_health() const { return current_health; }
		float get_max_health() const { return max_health; }

		// 属性 setter（GDScript .set() 必须走注册属性，否则静默失败——
		// bootstrap 曾因此全部配置失效：弓手不射箭、Boss 只有 5 血）
		void set_max_health(float v) { max_health = v; }
		void set_current_health(float v) { current_health = v; }
		void set_move_speed(float v) { move_speed = v; }
		void set_detection_radius(float v) { detection_radius = v; }
		void set_attack_range(float v) { attack_range = v; }
		void set_attack_damage(float v) { attack_damage = v; }
		void set_attack_cooldown(float v) { attack_cooldown = v; }
		void set_is_ranged(bool v) { is_ranged = v; }
		void set_is_flying(bool v) { is_flying = v; }
		void set_is_boss(bool v) { is_boss = v; }
		void set_no_drops(bool v) { no_drops = v; }
		void set_show_hp_bar(bool v) { show_hp_bar = v; }
		void set_display_name(const String &v) { display_name = v; }
		String get_display_name() const { return display_name; }
		void set_preferred_distance(float v) { preferred_distance = v; }
		float get_move_speed() const { return move_speed; }
		float get_detection_radius() const { return detection_radius; }
		float get_attack_range() const { return attack_range; }
		float get_attack_damage() const { return attack_damage; }
		float get_attack_cooldown() const { return attack_cooldown; }
		bool get_is_ranged() const { return is_ranged; }
		bool get_is_flying() const { return is_flying; }
		bool get_is_boss() const { return is_boss; }
		bool get_no_drops() const { return no_drops; }
		bool get_show_hp_bar() const { return show_hp_bar; }
		float get_preferred_distance() const { return preferred_distance; }

		double get_time() const { return _time; }

		void _ready() override;
		void _physics_process(double p_delta) override;
		void _process(double p_delta) override;
		void _on_hurtbox_hit(Object *p_hitbox, Node *p_source);

	// Spawn a projectile toward the player (called by Shoot state)
	void _spawn_projectile();

	protected:
		static void _bind_methods();

	private:
		double _time = 0.0;
		HitBox *_hitbox = nullptr;
		HurtBox *_hurtbox = nullptr;
		void _setup_collision();
		void _find_player();
		void _create_hitboxes();
		void _apply_damage(float p_amount, DamageCategory p_cat, Element p_elem, Node *p_source);
	};

} // namespace godot

#endif // CPP_KAKI_ENEMY_H
