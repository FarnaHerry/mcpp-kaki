#include "enemy.h"

#include "../core/enemy_database.h"
#include "../utils/text.h"
#include "safe_zone.h"


#include <cmath>
#include <cstdlib>

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

import mcpp_kaki.combat;
import mcpp_kaki.utils;
namespace godot {

	namespace EnemyStates {
		inline constexpr const char *Idle = "idle";
		inline constexpr const char *Patrol = "patrol";
		inline constexpr const char *Chase = "chase";
		inline constexpr const char *Attack = "attack";
		inline constexpr const char *Hurt = "hurt";
		inline constexpr const char *Death = "death";
		inline constexpr const char *Flee = "flee";
		inline constexpr const char *BossSpecial = "boss_special";
	}

	// ============================================================
	// Enemy states
	// ============================================================

	class EnemyIdleState : public State<Enemy> {
		double _idle_timer = 0.0;
		double _idle_duration = 1.5;

	public:
		void enter(Enemy *e) override {
			_idle_timer = 0.0;
			_idle_duration = 1.0 + ((float(std::rand()) / float(RAND_MAX)) * 1.0);
		}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			_idle_timer += delta;

			if (e->can_see_player()) {
				if (e->wants_flee()) {
					e->state_machine->transition_to(EnemyStates::Flee);
				} else if (e->player_in_attack_range() && e->can_attack()) {
					e->state_machine->transition_to(EnemyStates::Attack);
				} else {
					e->state_machine->transition_to(EnemyStates::Chase);
				}
				return;
			}

			if (_idle_timer >= _idle_duration) {
				e->state_machine->transition_to(EnemyStates::Patrol);
				return;
			}

			Vector2 vel = e->get_velocity();
			vel.x = 0.0f;
			if (!e->hover_movement()) vel.y += e->get_gravity() * delta;
			e->set_velocity(vel);
			e->move_and_slide();
		}
	};

	class EnemyPatrolState : public State<Enemy> {
		double _patrol_timer = 0.0;
		int _patrol_dir = 1;

	public:
		void enter(Enemy *e) override {
			_patrol_timer = 0.0;
			_patrol_dir = ((float(std::rand()) / float(RAND_MAX)) > 0.5f) ? 1 : -1;
			e->facing_direction = _patrol_dir;
		}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			_patrol_timer += delta;

			if (e->can_see_player()) {
				if (e->wants_flee()) {
					e->state_machine->transition_to(EnemyStates::Flee);
				} else {
					e->state_machine->transition_to(EnemyStates::Chase);
				}
				return;
			}

			// Change direction periodically or on wall hit
			if (_patrol_timer > 2.5 || e->is_on_wall()) {
				_patrol_dir *= -1;
				e->facing_direction = _patrol_dir;
				_patrol_timer = 0.0;
			}

			Vector2 vel = e->get_velocity();
			if (e->hover_movement()) {
				// Sine-wave hover patrol
				e->fly_phase_time += delta;
				vel.x = _patrol_dir * e->move_speed * 0.3f;
				vel.y = std::sin(e->fly_phase_time * 2.0f) * 30.0f;
			} else {
				vel.x = _patrol_dir * e->move_speed * 0.4f;
				vel.y += e->get_gravity() * delta;
			}
			e->set_velocity(vel);
			e->move_and_slide();
		}
	};

	class EnemyChaseState : public State<Enemy> {
	public:
		void enter(Enemy *e) override { e->_activate_boss_hud(); }
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			if (!e->can_see_player()) {
				e->state_machine->transition_to(EnemyStates::Idle);
				return;
			}

			// Ranged: flee if player gets too close
			if (e->wants_flee()) {
				e->state_machine->transition_to(EnemyStates::Flee);
				return;
			}

			// Check attack conditions
			if (e->can_attack()) {
				if (e->can_ranged_attack()) {
					e->state_machine->transition_to(EnemyStates::Attack);
					return;
				} else if (!e->behavior.ranged && e->player_in_attack_range()) {
					e->state_machine->transition_to(EnemyStates::Attack);
					return;
				}
			}

			e->update_facing_to_player();

			Node2D *target = e->get_player_target();
			float dir = (target->get_global_position().x > e->get_global_position().x) ? 1.0f : -1.0f;

			Vector2 vel = e->get_velocity();

			if (e->can_ranged_attack()) {
				// Stop at preferred range — don't close further
				vel.x = Math::move_toward(vel.x, 0.0f, float(e->move_speed * 5.0 * delta));
			} else if (e->behavior.ranged) {
				// Move toward preferred range
				float dist = e->get_global_position().distance_to(target->get_global_position());
				if (dist > e->preferred_distance) {
					vel.x = dir * e->move_speed;
				} else {
					vel.x = -dir * e->move_speed; // Back up if too close
				}
			} else {
				// Melee: rush toward player
				vel.x = dir * e->move_speed;
			}

			if (e->hover_movement()) {
				// Hover toward player
				e->fly_phase_time += delta;
				float target_y = target->get_global_position().y - 40.0f;
				float dy = target_y - e->get_global_position().y;
				vel.y = Math::clamp(dy * 3.0f, -100.0f, 100.0f);
			} else {
				vel.y += e->get_gravity() * delta;
			}

			e->set_velocity(vel);
			e->move_and_slide();
		}
	};

	// Flee state — archer runs away when player is too close
	class EnemyFleeState : public State<Enemy> {
	public:
		void enter(Enemy *e) override {}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			if (!e->can_see_player()) {
				e->state_machine->transition_to(EnemyStates::Idle);
				return;
			}

			// Stop fleeing when back at preferred range
			if (!e->player_too_close() && e->player_at_preferred_range()) {
				if (e->can_attack()) {
					e->state_machine->transition_to(EnemyStates::Attack);
				} else {
					e->state_machine->transition_to(EnemyStates::Chase);
				}
				return;
			}

			// Run away from player
			Node2D *target = e->get_player_target();
			float dir = (target->get_global_position().x > e->get_global_position().x) ? -1.0f : 1.0f;

			Vector2 vel = e->get_velocity();
			vel.x = dir * e->move_speed * 1.2f;
			if (!e->hover_movement()) vel.y += e->get_gravity() * delta;
			e->set_velocity(vel);
			e->move_and_slide();

			// Face away (facing the direction we're running)
			e->facing_direction = (dir > 0) ? 1 : -1;
		}
	};

	// Normal melee Attack state
	class EnemyAttackState : public State<Enemy> {
	public:
		void enter(Enemy *e) override {
			if (e->behavior.ranged) {
				// 远程：直接放箭。不能 transition_to(Shoot)——pending 只会存一个，
				// 同帧 Attack::physics_update 的 Chase 会覆盖 Shoot，
				// Shoot::enter 永远执行不到（远程攻击从未生效的根因）
				e->update_facing_to_player();
				e->_spawn_projectile();
				return;
			}

			// Melee: activate hitbox
			HitBox *hb = Object::cast_to<HitBox>(e->get_node_or_null("HitBox"));
			if (hb) {
				hb->set_scale(Vector2((float)e->facing_direction, 1.0f));
				hb->set_knockback_from_facing(e->facing_direction);
				hb->set_active(true);
			}
		}
		void exit(Enemy *e) override {
			HitBox *hb = Object::cast_to<HitBox>(e->get_node_or_null("HitBox"));
			if (hb) hb->set_active(false);
		}

		void physics_update(Enemy *e, double delta) override {
			e->last_attack_time = e->get_time();

			// Boss: randomly use special attack
			if (e->use_boss_special() && (std::rand() % 3 == 0)) {
				e->state_machine->transition_to(EnemyStates::BossSpecial);
				return;
			}

			if (e->can_see_player()) {
				e->state_machine->transition_to(EnemyStates::Chase);
			} else {
				e->state_machine->transition_to(EnemyStates::Idle);
			}
		}
	};

	// Boss special attack — projectile burst
	class EnemyBossSpecialState : public State<Enemy> {
	public:
		void enter(Enemy *e) override {
			e->last_special_time = e->get_time();
			e->update_facing_to_player();

			// Spawn a fan of projectiles（阶段升高扇面更密更宽更快：一相3 / 二相5 / 三相7）
			int count = (e->boss_phase >= 3) ? 7 : (e->boss_phase >= 2) ? 5 : 3;
			float spread = Math_PI * ((e->boss_phase >= 3) ? 0.44f : 0.3f); // 三相 ~79°，其余 ~54°
			float start_angle = -spread * 0.5f;

			for (int i = 0; i < count; i++) {
				float angle = start_angle + (spread / (count - 1)) * i;
				Vector2 dir(std::cos(angle) * e->facing_direction, std::sin(angle));

				Projectile *proj = memnew(Projectile);
				proj->set_name(String("BossFan") + String::num_int64(i)); // 扇形弹标记（重名会被引擎回退为类名，显式带序号）
				proj->set_position(e->get_global_position());
				proj->direction = dir;
				proj->speed = 200.0f + (e->boss_phase >= 3 ? 120.0f : e->boss_phase >= 2 ? 80.0f : 0.0f);
				proj->damage = e->attack_damage * 0.6f;
				proj->set_source(e);
				proj->set_collision_mask_value(3, true); // Hit player
				e->get_parent()->add_child(proj);
			}
		}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			if (e->can_see_player()) {
				e->state_machine->transition_to(EnemyStates::Chase);
			} else {
				e->state_machine->transition_to(EnemyStates::Idle);
			}
		}
	};

	class EnemyHurtState : public State<Enemy> {
		double _hurt_timer = 0.0;
		float _knockback_x = 0.0f;

	public:
		void enter(Enemy *e) override {
			_hurt_timer = 0.0;
			_knockback_x = (e->facing_direction > 0) ? -200.0f : 200.0f;

			Vector2 vel = e->get_velocity();
			vel.x = _knockback_x * e->knockback_resistance;
			vel.y = e->hover_movement() ? -50.0f : -100.0f;
			e->set_velocity(vel);

			// Boss phase transition（阈值为 0 的段禁用——由外部驱动，如渡劫天罚使按 66%/33% 走控制器）
			if (e->behavior.boss) {
				if (e->boss_phase < 2 && e->boss_phase2_threshold > 0.0f
				    && e->current_health <= e->max_health * e->boss_phase2_threshold) {
					e->boss_phase = 2;
					e->move_speed *= 1.3f;
					e->attack_cooldown *= 0.7f;
				}
				if (e->boss_phase == 2 && e->boss_phase3_threshold > 0.0f
				    && e->current_health <= e->max_health * e->boss_phase3_threshold) {
					e->boss_phase = 3;
					e->move_speed *= 1.2f;
					e->attack_cooldown *= 0.8f;
				}
			}
		}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			_hurt_timer += delta;

			if (e->is_dead()) {
				e->state_machine->transition_to(EnemyStates::Death);
				return;
			}

			if (_hurt_timer > 0.3f && (e->is_on_floor() || e->hover_movement())) {
				if (e->can_see_player()) {
					e->state_machine->transition_to(EnemyStates::Chase);
				} else {
					e->state_machine->transition_to(EnemyStates::Idle);
				}
				return;
			}

			Vector2 vel = e->get_velocity();
			if (!e->hover_movement()) vel.y += e->get_gravity() * delta;
			e->set_velocity(vel);
			e->move_and_slide();
		}
	};

	class EnemyDeathState : public State<Enemy> {
	public:
		void enter(Enemy *e) override {
			// Boss 战结束：撤下 HUD Boss 条（带名字——多 Boss 同场各移除各的）
			if (e->_boss_hud_active) {
				SignalBus *bus = SignalBus::get_singleton();
				if (bus) {
					String nm = e->display_name.is_empty() ? String(e->get_name()) : e->display_name;
					bus->emit_signal("boss_fight_ended", nm);
				}
			}
			// Boss: big energy reward
			float energy = e->behavior.boss ? 150.0f : 15.0f;

			Node *source = e->get_node_or_null(".."); // parent is the scene
			if (source) {
				// Give extra energy via signal
			}

			e->set_process(false);
			e->set_physics_process(false);
			e->queue_free();
		}
		void exit(Enemy *e) override {}
		void physics_update(Enemy *e, double delta) override {}
	};

	// ============================================================
	// Enemy implementation
	// ============================================================

	void Enemy::_bind_methods() {
		ClassDB::bind_method(D_METHOD("take_damage", "amount", "source"), &Enemy::take_damage);
		ClassDB::bind_method(D_METHOD("take_damage_typed", "amount", "cat", "elem", "source"), &Enemy::take_damage_typed);
		ClassDB::bind_method(D_METHOD("_on_hurtbox_hit", "hitbox", "source"), &Enemy::_on_hurtbox_hit);
		ClassDB::bind_method(D_METHOD("get_current_health"), &Enemy::get_current_health);
		ClassDB::bind_method(D_METHOD("get_max_health"), &Enemy::get_max_health);

		// 属性注册——GDScript .set() 的生效前提
		ClassDB::bind_method(D_METHOD("set_max_health", "v"), &Enemy::set_max_health);
		ClassDB::bind_method(D_METHOD("set_current_health", "v"), &Enemy::set_current_health);
		ClassDB::bind_method(D_METHOD("set_move_speed", "v"), &Enemy::set_move_speed);
		ClassDB::bind_method(D_METHOD("set_detection_radius", "v"), &Enemy::set_detection_radius);
		ClassDB::bind_method(D_METHOD("set_attack_range", "v"), &Enemy::set_attack_range);
		ClassDB::bind_method(D_METHOD("set_attack_damage", "v"), &Enemy::set_attack_damage);
		ClassDB::bind_method(D_METHOD("set_attack_cooldown", "v"), &Enemy::set_attack_cooldown);
		ClassDB::bind_method(D_METHOD("set_is_ranged", "v"), &Enemy::set_is_ranged);
		ClassDB::bind_method(D_METHOD("set_is_flying", "v"), &Enemy::set_is_flying);
		ClassDB::bind_method(D_METHOD("set_is_boss", "v"), &Enemy::set_is_boss);
		ClassDB::bind_method(D_METHOD("set_no_drops", "v"), &Enemy::set_no_drops);
		ClassDB::bind_method(D_METHOD("set_show_hp_bar", "v"), &Enemy::set_show_hp_bar);
		ClassDB::bind_method(D_METHOD("set_is_soul_reaper", "v"), &Enemy::set_is_soul_reaper);
		ClassDB::bind_method(D_METHOD("set_display_name", "v"), &Enemy::set_display_name);
		ClassDB::bind_method(D_METHOD("get_display_name"), &Enemy::get_display_name);
		ClassDB::bind_method(D_METHOD("set_enemy_id", "v"), &Enemy::set_enemy_id);
		ClassDB::bind_method(D_METHOD("get_enemy_id"), &Enemy::get_enemy_id);
		ClassDB::bind_method(D_METHOD("set_drop_table", "v"), &Enemy::set_drop_table);
		ClassDB::bind_method(D_METHOD("get_drop_table"), &Enemy::get_drop_table);
		ClassDB::bind_method(D_METHOD("get_def_color"), &Enemy::get_def_color);
		ClassDB::bind_method(D_METHOD("get_def_size"), &Enemy::get_def_size);
		ClassDB::bind_method(D_METHOD("set_preferred_distance", "v"), &Enemy::set_preferred_distance);
		ClassDB::bind_method(D_METHOD("get_move_speed"), &Enemy::get_move_speed);
		ClassDB::bind_method(D_METHOD("get_detection_radius"), &Enemy::get_detection_radius);
		ClassDB::bind_method(D_METHOD("get_attack_range"), &Enemy::get_attack_range);
		ClassDB::bind_method(D_METHOD("get_attack_damage"), &Enemy::get_attack_damage);
		ClassDB::bind_method(D_METHOD("get_attack_cooldown"), &Enemy::get_attack_cooldown);
		ClassDB::bind_method(D_METHOD("get_is_ranged"), &Enemy::get_is_ranged);
		ClassDB::bind_method(D_METHOD("get_is_flying"), &Enemy::get_is_flying);
		ClassDB::bind_method(D_METHOD("get_is_boss"), &Enemy::get_is_boss);
		ClassDB::bind_method(D_METHOD("get_no_drops"), &Enemy::get_no_drops);
		ClassDB::bind_method(D_METHOD("get_show_hp_bar"), &Enemy::get_show_hp_bar);
		ClassDB::bind_method(D_METHOD("get_is_soul_reaper"), &Enemy::get_is_soul_reaper);
		ClassDB::bind_method(D_METHOD("get_preferred_distance"), &Enemy::get_preferred_distance);
		ClassDB::bind_method(D_METHOD("set_elite_tier", "v"), &Enemy::set_elite_tier);
		ClassDB::bind_method(D_METHOD("get_elite_tier"), &Enemy::get_elite_tier);
		ClassDB::bind_method(D_METHOD("set_affix_id", "v"), &Enemy::set_affix_id);
		ClassDB::bind_method(D_METHOD("get_affix_id"), &Enemy::get_affix_id);
		ClassDB::bind_method(D_METHOD("make_elite", "tier", "affix"), &Enemy::make_elite);
		ClassDB::bind_method(D_METHOD("make_elite_random", "tier"), &Enemy::make_elite_random);
		ClassDB::bind_method(D_METHOD("get_elite_chance"), &Enemy::get_elite_chance);
		ClassDB::bind_method(D_METHOD("get_defense"), &Enemy::get_defense);
		ClassDB::bind_method(D_METHOD("set_realm", "v"), &Enemy::set_realm);
		ClassDB::bind_method(D_METHOD("get_realm"), &Enemy::get_realm);
		ClassDB::bind_method(D_METHOD("suppress", "t"), &Enemy::suppress);
		ClassDB::bind_method(D_METHOD("set_boss_phase", "v"), &Enemy::set_boss_phase);
		ClassDB::bind_method(D_METHOD("get_boss_phase"), &Enemy::get_boss_phase);
		ClassDB::bind_method(D_METHOD("set_special_min_phase", "v"), &Enemy::set_special_min_phase);
		ClassDB::bind_method(D_METHOD("get_special_min_phase"), &Enemy::get_special_min_phase);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "realm"), "set_realm", "get_realm");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_health"), "set_max_health", "get_max_health");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "current_health"), "set_current_health", "get_current_health");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed"), "set_move_speed", "get_move_speed");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "detection_radius"), "set_detection_radius", "get_detection_radius");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attack_range"), "set_attack_range", "get_attack_range");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attack_damage"), "set_attack_damage", "get_attack_damage");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attack_cooldown"), "set_attack_cooldown", "get_attack_cooldown");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_ranged"), "set_is_ranged", "get_is_ranged");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_flying"), "set_is_flying", "get_is_flying");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_boss"), "set_is_boss", "get_is_boss");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "no_drops"), "set_no_drops", "get_no_drops");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_hp_bar"), "set_show_hp_bar", "get_show_hp_bar");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_soul_reaper"), "set_is_soul_reaper", "get_is_soul_reaper");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "enemy_id"), "set_enemy_id", "get_enemy_id");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "drop_table"), "set_drop_table", "get_drop_table");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "preferred_distance"), "set_preferred_distance", "get_preferred_distance");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "elite_tier"), "set_elite_tier", "get_elite_tier");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "affix_id"), "set_affix_id", "get_affix_id");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "boss_phase"), "set_boss_phase", "get_boss_phase");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "special_min_phase"), "set_special_min_phase", "get_special_min_phase");

		ADD_SIGNAL(MethodInfo("enemy_died"));
		ADD_SIGNAL(MethodInfo("boss_died"));
	}

	void Enemy::_apply_boss_hp_scale() {
		if (_boss_hp_scaled)
			return;
		_boss_hp_scaled = true;
		current_health *= 5.0f;
		max_health *= 5.0f;
	}

	void Enemy::set_is_boss(bool v) {
		is_boss = v;
		behavior.boss = v; // 兼容字段同步
		// 时序陷阱修复：脚本 add_child 后才 set("is_boss")（_ready 已跑完），
		// 在此补偿 ×5 血量；之后脚本再显式 set max_health 的以显式值为准（bootstrap/beijulu 现状）
		if (v && is_inside_tree() && !is_queued_for_deletion()) {
			_apply_boss_hp_scale();
		}
	}

	// ============================================================
	// 命名策略函数（行为语义集中，状态类调用）
	// ============================================================

	bool Enemy::wants_flee() const {
		return behavior.ranged && player_too_close();
	}

	bool Enemy::can_ranged_attack() const {
		return behavior.ranged && player_at_preferred_range();
	}

	bool Enemy::hover_movement() const {
		return behavior.flying;
	}

	bool Enemy::use_boss_special() const {
		return behavior.boss && can_special() && boss_phase >= special_min_phase;
	}

	void Enemy::set_enemy_id(const String &v) {
		enemy_id = v;
		const EnemyDef *def = EnemyDatabase::get_def(v);
		if (!def)
			return; // 未知 id：保持类默认值
		_def_color = def->color;
		_def_size = def->size;
		// 先应用普通属性（def.hp 是基础血，未 ×5）……
		// 数值重平衡：敌人攻/血随境界缩放（realm0=1.0× → realm11=6.5×），Boss ×5 在其后
		const float realm_scale = 1.0f + 0.5f * float(def->realm);
		max_health = def->hp * realm_scale;
		current_health = def->hp * realm_scale;
		move_speed = def->speed;
		detection_radius = def->detection;
		attack_range = def->attack_range;
		attack_cooldown = def->attack_cooldown;
		attack_damage = def->atk * realm_scale;
		preferred_distance = def->preferred;
		realm = def->realm;
		// 行为聚合（Wave4：ranged/flying/boss 兼容 + slow/heavy/summon 组合；乘区一次性算好）
		{
			EnemyBehavior b;
			b.ranged = def->ranged;
			b.flying = def->flying;
			b.boss = def->boss;
			b.slow = def->slow;
			b.heavy = def->heavy;
			b.summon = def->summon;
			b.move_mult = b.slow ? 0.6f : 1.0f;
			b.atk_mult = 1.0f;
			b.def_add = b.heavy ? 5.0f : 0.0f;
			behavior = b;
			is_ranged = b.ranged; // 兼容字段同步
			is_flying = b.flying;
		}
		// 数值乘区应用（slow 减速 / heavy 加防）
		move_speed *= behavior.move_mult;
		attack_damage *= behavior.atk_mult;
		defense += behavior.def_add;
		display_name = def->name;
		drop_table = def->drops;
		// ……最后才置 Boss（其 _apply_boss_hp_scale 幂等补偿把血量 ×5，
		// 与调用顺序无关：未入树时由 _ready 补偿，已入树时由 setter 补偿）
		if (def->boss) {
			set_is_boss(true);
		}
	}

	void Enemy::make_elite(int p_tier, const String &p_affix) {
		if (_elite_applied)
			return; // 幂等：重复调用不叠加
		if (p_tier <= 0 || is_boss)
			return; // 普通化无效；Boss 拒绝精英化
		_elite_applied = true;
		elite_tier = p_tier > 2 ? 2 : p_tier;

		const AffixDef *af = AffixDatabase::get_affix(p_affix);
		affix_id = af ? p_affix : String();

		// tier 基础倍率（首领更硬）× 词缀倍率
		const float tier_hp = (elite_tier >= 2) ? 2.5f : 1.5f;
		const float tier_atk = (elite_tier >= 2) ? 1.5f : 1.2f;
		max_health *= tier_hp * (af ? af->hp_mult : 1.0f);
		current_health *= tier_hp * (af ? af->hp_mult : 1.0f);
		attack_damage *= tier_atk * (af ? af->atk_mult : 1.0f);
		if (af) {
			move_speed *= af->speed_mult;
			detection_radius *= af->detect_mult;
			defense += af->def_add;
		}

		// display_name 前缀：「精英·/首领·」+ 词缀名
		String base = display_name.is_empty() ? String(get_name()) : display_name;
		String prefix = (elite_tier >= 2) ? TXT("首领·") : TXT("精英·");
		display_name = af ? (prefix + af->name + TXT(" ") + base) : (prefix + base);

		// 视觉：本体染词缀色 + 放大（优先 sprite 子节点，无则自身 scale）
		_elite_tint = af ? af->tint : Color(1.0f, 1.0f, 1.0f, 1.0f);
		set_modulate(_elite_tint);
		const float sc = (elite_tier >= 2) ? 1.3f : 1.15f;
		Node2D *sprite = Object::cast_to<Node2D>(get_node_or_null("Polygon2D"));
		if (sprite) {
			sprite->set_scale(sprite->get_scale() * sc);
		} else {
			set_scale(get_scale() * sc);
		}
	}

	void Enemy::make_elite_random(int p_tier) {
		const AffixDef *af = AffixDatabase::random_affix();
		make_elite(p_tier, af ? af->id : String());
	}

	float Enemy::get_elite_chance() const {
		if (enemy_id.is_empty())
			return 0.0f;
		const EnemyDef *def = EnemyDatabase::get_def(enemy_id);
		return def ? def->elite_chance : 0.0f;
	}

	void Enemy::_ready() {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		current_health = max_health;

		// 威压/灵压扫描用
		add_to_group("enemies");

		// Boss gets 5x health（仅在 _ready 前已置 is_boss 的场景直摆路径；
		// 脚本 add_child 后再 set("is_boss") 的由 set_is_boss 补偿，二者经 _boss_hp_scaled 幂等）
		if (is_boss) {
			_apply_boss_hp_scale();
		}

		_setup_collision();
		_create_hitboxes();
		// Boss 血条不再画在头顶——经 SignalBus 上报 GameHUD 顶部居中条
		_find_player();

		state_machine = new StateMachine<Enemy>(this);
		state_machine->add_state(EnemyStates::Idle, new EnemyIdleState());
		state_machine->add_state(EnemyStates::Patrol, new EnemyPatrolState());
		state_machine->add_state(EnemyStates::Chase, new EnemyChaseState());
		state_machine->add_state(EnemyStates::Attack, new EnemyAttackState());
		state_machine->add_state(EnemyStates::Hurt, new EnemyHurtState());
		state_machine->add_state(EnemyStates::Death, new EnemyDeathState());
		state_machine->add_state(EnemyStates::Flee, new EnemyFleeState());
		state_machine->add_state(EnemyStates::BossSpecial, new EnemyBossSpecialState());

		state_machine->set_initial_state(EnemyStates::Idle);
	}

	void Enemy::_physics_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint() || !state_machine)
			return;

		// 威压慑服：定身不动（跳过状态机），只受重力
		if (_suppress_t > 0.0) {
			Vector2 vel = get_velocity();
			vel.x = 0.0f;
			if (!hover_movement()) vel.y += get_gravity() * p_delta;
			set_velocity(vel);
			move_and_slide();
			return;
		}

		state_machine->physics_update(p_delta);
	}

	// 脱离场景（死亡 queue_free / 场景卸载）：若 Boss 血条仍挂 HUD，撤下——
	// 玩家出房/清场时 Boss 战未正常结束（无死亡 enter），避免血条残留
	void Enemy::_exit_tree() {
		if (_boss_hud_active) {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				String nm = display_name.is_empty() ? String(get_name()) : display_name;
				bus->emit_signal("boss_fight_ended", nm);
			}
			_boss_hud_active = false;
		}
	}

	void Enemy::_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint() || !state_machine)
			return;

		_time += p_delta;

		// 威压慑服倒计时
		if (_suppress_t > 0.0) {
			_suppress_t -= p_delta;
			if (_suppress_t <= 0.0) {
				_suppress_t = 0.0;
				set_modulate(_elite_applied ? _elite_tint : Color(1.0f, 1.0f, 1.0f, 1.0f)); // 复原（精英回词缀色）
			}
			return; // 慑服期间不跑状态机 process（_physics_process 也跳过）
		}

		state_machine->process_update(p_delta);

	}

	void Enemy::_activate_boss_hud() {
		// Boss 战触发（aggro 或首次受击）：上报 HUD 顶部 Boss 条
		if (_boss_hud_active || !(behavior.boss || show_hp_bar))
			return;
		_boss_hud_active = true;
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			String name = display_name.is_empty() ? String(get_name()) : display_name;
			bus->emit_signal("boss_fight_update", name, current_health, max_health);
		}
	}

	void Enemy::_setup_collision() {
		set_collision_layer_value(4, true);
		set_collision_mask_value(1, true);
		set_collision_mask_value(2, true);
	}

	void Enemy::_create_hitboxes() {
		// HitBox for dealing damage
		_hitbox = memnew(HitBox);
		_hitbox->set_name("HitBox");
		_hitbox->damage = attack_damage;
		_hitbox->set_active(false);
		_hitbox->set_collision_layer_value(6, true);
		_hitbox->set_collision_mask_value(3, true);
		_hitbox->connect("area_entered", Callable(_hitbox, "_on_area_entered"));

		CollisionShape2D *hb_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> hb_rect;
		hb_rect.instantiate();
		// 46×24 @ (12,0)：覆盖自身 + 身后少许 + 身前判定（贴身不再打空）
		hb_rect->set_size(Vector2(46, 24));
		hb_shape->set_shape(hb_rect);
		hb_shape->set_position(Vector2(12, 0));
		_hitbox->add_child(hb_shape);

		Polygon2D *hb_visual = memnew(Polygon2D);
		hb_visual->set_name("HitBoxVisual");
		hb_visual->set_visible(false);
		hb_visual->set_color(Color(1.0, 0.3, 0.3, 0.4));
		PackedVector2Array hb_poly;
		hb_poly.append(Vector2(-23, -12));
		hb_poly.append(Vector2(23, -12));
		hb_poly.append(Vector2(23, 12));
		hb_poly.append(Vector2(-23, 12));
		hb_visual->set_polygon(hb_poly);
		hb_visual->set_position(Vector2(12, 0));
		_hitbox->add_child(hb_visual);

		add_child(_hitbox);

		// HurtBox for receiving damage
		_hurtbox = memnew(HurtBox);
		_hurtbox->set_name("HurtBox");
		_hurtbox->set_collision_layer_value(4, true);
		_hurtbox->set_collision_mask_value(5, true);

		CollisionShape2D *hu_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> hu_rect;
		hu_rect.instantiate();
		hu_rect->set_size(Vector2(32, 32));
		hu_shape->set_shape(hu_rect);
		_hurtbox->add_child(hu_shape);
		add_child(_hurtbox);

		_hurtbox->connect("hurtbox_hit", Callable(this, "_on_hurtbox_hit"));
	}

	void Enemy::_find_player() {
		Node *root = get_tree()->get_current_scene();
		if (root) {
			_player_target = Object::cast_to<Node2D>(root->get_node_or_null("Player"));
		}
	}

	void Enemy::take_damage_typed(float p_amount, int p_cat, int p_elem, Node *p_source) {
		_apply_damage(p_amount, DamageCategory(p_cat), Element(p_elem), p_source);
	}

	void Enemy::take_damage(float p_amount, Node *p_source) {
		// 无类别入口（投射物/环境等）：按物理无元素结算
		_apply_damage(p_amount, DMG_PHYSICAL, ELEM_NONE, p_source);
	}

	void Enemy::take_hit(const HitBox *p_hitbox, Node *p_source) {
		if (!p_hitbox) return;
		_apply_damage(p_hitbox->damage, p_hitbox->damage_category, p_hitbox->element, p_source);
	}

	void Enemy::_apply_damage(float p_amount, DamageCategory p_cat, Element p_elem, Node *p_source) {
		_activate_boss_hud(); // 受击也触发（先下手为强）
		// DamageCalculator 统一结算（防御/法抗/元素抗 + 五行克制）
		DamageInfo info;
		info.base_amount = p_amount;
		info.category = p_cat;
		info.element = p_elem;
		DefenseProfile def;
		def.defense = defense;
		def.spell_resist = spell_resist;
		def.self_element = self_element;
		for (int i = 0; i < ELEM_CAPACITY; i++) def.elem_resist[i] = elem_resist[i];
		float actual = DamageCalculator::compute(info, def);

		current_health -= actual;
		if (_boss_hud_active) {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				String name = display_name.is_empty() ? String(get_name()) : display_name;
				bus->emit_signal("boss_fight_update", name, current_health, max_health);
			}
		}

		// 伤害数字显示
		{
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("damage_dealt", get_global_position(), actual, false);
			}
		}

		if (current_health <= 0.0f) {
			current_health = 0.0f;

			// Give spiritual energy（平衡：随境界成长，防高境击杀无用）
			if (p_source) {
				float energy = (behavior.boss ? 150.0f : 15.0f) * (1.0f + realm);
				p_source->call("gain_spiritual_energy", energy);
			}

			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("enemy_killed", this, p_source);
				if (behavior.boss) {
					bus->emit_signal("boss_died");
				}
				// 精英追加奖励掉落（幻境之敌 no_drops 不 emit）
				if (elite_tier > 0 && !no_drops) {
					bus->emit_signal("elite_killed", get_global_position(), elite_tier, realm);
				}
			}

			if (state_machine) {
				state_machine->transition_to(EnemyStates::Death);
			}
			emit_signal("enemy_died");
			if (behavior.boss) {
				emit_signal("boss_died"); // 自身信号（SignalBus boss_died 之外，供定点 connect——渡劫天罚使/天界巨灵神）
			}
			return;
		}

		if (state_machine && !state_machine->is_state(EnemyStates::Hurt)) {
			state_machine->transition_to(EnemyStates::Hurt);
		}
	}

	void Enemy::_on_hurtbox_hit(Object *p_hitbox, Node *p_source) {
		HitBox *hb = Object::cast_to<HitBox>(p_hitbox);
		if (!hb) return;
		take_hit(hb, p_source);
	}

	void Enemy::_spawn_projectile() {
		if (!_player_target) return;

		Vector2 to_player = _player_target->get_global_position() - get_global_position();
		Vector2 dir = to_player.normalized();

		Projectile *proj = memnew(Projectile);
		proj->set_position(get_global_position());
		proj->direction = dir;
		proj->speed = 250.0f;
		proj->damage = attack_damage * (behavior.boss ? 1.5f : 1.0f);
		proj->set_source(this);
		proj->set_collision_mask_value(3, true); // Hit player
		proj->set_collision_mask_value(1, true); // Hit ground walls
		get_parent()->add_child(proj);
	}

	// ---- Accessors ----

	float Enemy::get_gravity() const {
		float default_gravity = 980.0f;
		Variant g = ProjectSettings::get_singleton()->get_setting("physics/2d/default_gravity");
		if (g.get_type() != Variant::NIL) default_gravity = float(g);
		float scale = 1.0f;
		Variant s = ProjectSettings::get_singleton()->get_setting("physics/2d/default_gravity_scale");
		if (s.get_type() != Variant::NIL) scale = float(s);
		return default_gravity * scale;
	}

	Node2D *Enemy::get_player_target() const {
		return _player_target;
	}

	bool Enemy::can_see_player() const {
		if (!_player_target) return false;
		// 城镇安全区：玩家或自身在区内均不索敌（chase 自然退回 idle/patrol）。
		// 仅世界层生效——Portal 房间挂洲原点（坐标带重叠），房间内一律正常索敌。
		if (_player_target->get_parent() == get_tree()->get_current_scene() &&
				(SafeZone::is_point_safe(_player_target->get_global_position()) ||
						SafeZone::is_point_safe(get_global_position())))
			return false;
		float dist = get_global_position().distance_to(_player_target->get_global_position());
		return dist <= detection_radius;
	}

	bool Enemy::player_in_attack_range() const {
		if (!_player_target) return false;
		float dist = get_global_position().distance_to(_player_target->get_global_position());
		return dist <= attack_range;
	}

	bool Enemy::player_too_close() const {
		if (!_player_target) return false;
		float dist = get_global_position().distance_to(_player_target->get_global_position());
		// Too close if within 40% of preferred distance (min 40px)
		float threshold = Math::max(preferred_distance * 0.4f, 40.0f);
		return dist < threshold;
	}

	bool Enemy::player_at_preferred_range() const {
		if (!_player_target) return false;
		if (preferred_distance <= 0.0f) return player_in_attack_range();
		float dist = get_global_position().distance_to(_player_target->get_global_position());
		return dist <= preferred_distance * 1.2f && dist >= preferred_distance * 0.4f;
	}

	bool Enemy::can_attack() const {
		return (_time - last_attack_time) >= attack_cooldown;
	}

	bool Enemy::can_special() const {
		return (_time - last_special_time) >= special_attack_cooldown;
	}

	void Enemy::update_facing_to_player() {
		if (!_player_target) return;
		facing_direction = (_player_target->get_global_position().x > get_global_position().x) ? 1 : -1;
	}

	void Enemy::suppress(double t) {
		_suppress_t = t;
		set_modulate(Color(0.5f, 0.5f, 0.5f, 1.0f)); // 压灰
	}

} // namespace godot
