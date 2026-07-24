#include "enemy.h"

#include "../combat/damage_calculator.h"
#include "../combat/hitbox.h"
#include "../combat/hurtbox.h"
#include "../combat/projectile.h"
#include "../utils/signal_bus.h"

#include <cmath>
#include <cstdlib>

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

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
				if (e->is_ranged && e->player_too_close()) {
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
			if (!e->is_flying) vel.y += e->get_gravity() * delta;
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
				if (e->is_ranged && e->player_too_close()) {
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
			if (e->is_flying) {
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
		void enter(Enemy *e) override {}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			if (!e->can_see_player()) {
				e->state_machine->transition_to(EnemyStates::Idle);
				return;
			}

			// Ranged: flee if player gets too close
			if (e->is_ranged && e->player_too_close()) {
				e->state_machine->transition_to(EnemyStates::Flee);
				return;
			}

			// Check attack conditions
			if (e->can_attack()) {
				if (e->is_ranged && e->player_at_preferred_range()) {
					e->state_machine->transition_to(EnemyStates::Attack);
					return;
				} else if (!e->is_ranged && e->player_in_attack_range()) {
					e->state_machine->transition_to(EnemyStates::Attack);
					return;
				}
			}

			e->update_facing_to_player();

			Node2D *target = e->get_player_target();
			float dir = (target->get_global_position().x > e->get_global_position().x) ? 1.0f : -1.0f;

			Vector2 vel = e->get_velocity();

			if (e->is_ranged && e->player_at_preferred_range()) {
				// Stop at preferred range — don't close further
				vel.x = Math::move_toward(vel.x, 0.0f, float(e->move_speed * 5.0 * delta));
			} else if (e->is_ranged) {
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

			if (e->is_flying) {
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
			if (!e->is_flying) vel.y += e->get_gravity() * delta;
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
			if (e->is_ranged) {
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
			if (e->is_boss && e->can_special() && (std::rand() % 3 == 0)) {
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

			// Spawn a fan of projectiles
			int count = (e->boss_phase >= 2) ? 5 : 3;
			float spread = Math_PI * 0.3f; // ~54 degree spread
			float start_angle = -spread * 0.5f;

			for (int i = 0; i < count; i++) {
				float angle = start_angle + (spread / (count - 1)) * i;
				Vector2 dir(std::cos(angle) * e->facing_direction, std::sin(angle));

				Projectile *proj = memnew(Projectile);
				proj->set_position(e->get_global_position());
				proj->direction = dir;
				proj->speed = 200.0f + (e->boss_phase >= 2 ? 80.0f : 0.0f);
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
			vel.y = e->is_flying ? -50.0f : -100.0f;
			e->set_velocity(vel);

			// Boss phase transition
			if (e->is_boss && e->current_health <= e->max_health * e->boss_phase2_threshold
			    && e->boss_phase < 2) {
				e->boss_phase = 2;
				e->move_speed *= 1.3f;
				e->attack_cooldown *= 0.7f;
			}
		}
		void exit(Enemy *e) override {}

		void physics_update(Enemy *e, double delta) override {
			_hurt_timer += delta;

			if (e->is_dead()) {
				e->state_machine->transition_to(EnemyStates::Death);
				return;
			}

			if (_hurt_timer > 0.3f && (e->is_on_floor() || e->is_flying)) {
				if (e->can_see_player()) {
					e->state_machine->transition_to(EnemyStates::Chase);
				} else {
					e->state_machine->transition_to(EnemyStates::Idle);
				}
				return;
			}

			Vector2 vel = e->get_velocity();
			if (!e->is_flying) vel.y += e->get_gravity() * delta;
			e->set_velocity(vel);
			e->move_and_slide();
		}
	};

	class EnemyDeathState : public State<Enemy> {
	public:
		void enter(Enemy *e) override {
			// Boss: big energy reward
			float energy = e->is_boss ? 150.0f : 15.0f;

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
		ClassDB::bind_method(D_METHOD("get_preferred_distance"), &Enemy::get_preferred_distance);

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
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "preferred_distance"), "set_preferred_distance", "get_preferred_distance");

		ADD_SIGNAL(MethodInfo("enemy_died"));
		ADD_SIGNAL(MethodInfo("boss_died"));
	}

	void Enemy::_ready() {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		current_health = max_health;

		// Boss gets 5x health
		if (is_boss) {
			current_health *= 5.0f;
			max_health *= 5.0f;
		}

		_setup_collision();
		_create_hitboxes();
		if (show_hp_bar)
			_create_hp_bar();
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

		state_machine->physics_update(p_delta);
	}

	void Enemy::_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint() || !state_machine)
			return;

		_time += p_delta;
		state_machine->process_update(p_delta);

		// 头顶血条（秘境劫敌/Boss）
		if (_hp_bar_fill) {
			float frac = (max_health > 0.0f) ? (current_health / max_health) : 0.0f;
			Vector2 s = _hp_bar_fill->get_size();
			s.x = 28.0f * Math::clamp(frac, 0.0f, 1.0f);
			_hp_bar_fill->set_size(s);
		}
	}

	void Enemy::_create_hp_bar() {
		ColorRect *bg = memnew(ColorRect);
		bg->set_position(Vector2(-14, -26));
		bg->set_size(Vector2(28, 4));
		bg->set_color(Color(0.1f, 0.1f, 0.1f, 0.85f));
		add_child(bg);

		_hp_bar_fill = memnew(ColorRect);
		_hp_bar_fill->set_position(Vector2(-14, -26));
		_hp_bar_fill->set_size(Vector2(28, 4));
		_hp_bar_fill->set_color(Color(0.85f, 0.15f, 0.15f, 1));
		add_child(_hp_bar_fill);
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

		// 伤害数字显示
		{
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("damage_dealt", get_global_position(), actual, false);
			}
		}

		if (current_health <= 0.0f) {
			current_health = 0.0f;

			// Give spiritual energy
			if (p_source) {
				float energy = is_boss ? 150.0f : 15.0f;
				p_source->call("gain_spiritual_energy", energy);
			}

			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("enemy_killed", this, p_source);
				if (is_boss) {
					bus->emit_signal("boss_died");
				}
			}

			if (state_machine) {
				state_machine->transition_to(EnemyStates::Death);
			}
			emit_signal("enemy_died");
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
		proj->damage = attack_damage * (is_boss ? 1.5f : 1.0f);
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

} // namespace godot
