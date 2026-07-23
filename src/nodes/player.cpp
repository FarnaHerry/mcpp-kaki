#include "player.h"

#include "../combat/combo_chain.h"
#include "../combat/hitbox.h"
#include "../combat/hurtbox.h"
#include "../cultivation/ability_manager.h"
#include "../cultivation/cultivation_system.h"
#include "../inventory/item.h"
#include "../inventory/item_database.h"
#include "../utils/signal_bus.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

	// ============================================================
	// Player states
	// ============================================================
	namespace PlayerStates {
		inline constexpr const char *Idle = "idle";
		inline constexpr const char *Run = "run";
		inline constexpr const char *Jump = "jump";
		inline constexpr const char *Fall = "fall";
		inline constexpr const char *WallCling = "wall_cling";
		inline constexpr const char *Dash = "dash";
		inline constexpr const char *Attack = "attack";
		inline constexpr const char *Fly = "fly";
	} // namespace PlayerStates

	// ------- Idle -------
	class PlayerIdleState : public State<Player> {
	public:
		void enter(Player *p) override { p->was_flying = false; }
		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			// Check ground — if we're not on floor, we fell off
			if (!p->is_on_floor()) {
				p->left_ground_time = p->get_time();
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			// Jump (with buffer check)
			if (p->jump_buffer.consume(p->get_time(), p->jump_buffer_time) || p->jump_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Jump);
				return;
			}

			// Dash
			if (p->dash_buffer.consume(p->get_time(), p->jump_buffer_time) || p->dash_just_pressed()) {
				if (p->can_dash()) {
					p->state_machine->transition_to(PlayerStates::Dash);
					return;
				}
			}

			// Attack
			if (p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Attack);
				return;
			}

			// Move input → Run
			float input = p->get_move_input();
			if (Math::abs(input) > 0.01f) {
				p->state_machine->transition_to(PlayerStates::Run);
				return;
			}

			// Stay idle — apply friction
			Vector2 vel = p->get_velocity();
			vel.x = Math::move_toward(vel.x, 0.0f, float(p->move_speed * 10.0 * delta));
			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Run -------
	class PlayerRunState : public State<Player> {
	public:
		void enter(Player *p) override {}
		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			if (!p->is_on_floor()) {
				p->left_ground_time = p->get_time();
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			if (p->jump_buffer.consume(p->get_time(), p->jump_buffer_time) || p->jump_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Jump);
				return;
			}

			if (p->dash_buffer.consume(p->get_time(), p->jump_buffer_time) || p->dash_just_pressed()) {
				if (p->can_dash()) {
					p->state_machine->transition_to(PlayerStates::Dash);
					return;
				}
			}

			// Attack
			if (p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Attack);
				return;
			}

			float input = p->get_move_input();
			if (Math::abs(input) < 0.01f) {
				p->state_machine->transition_to(PlayerStates::Idle);
				return;
			}

			Vector2 vel = p->get_velocity();
			vel.x = input * p->move_speed;
			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Jump -------
	class PlayerJumpState : public State<Player> {
	public:
		void enter(Player *p) override {
			Vector2 vel = p->get_velocity();
			vel.y = p->jump_velocity;
			p->set_velocity(vel);
			p->jump_buffer.reset();
		}

		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			// Variable jump height — cut velocity when button released
			if (!p->jump_held()) {
				Vector2 vel = p->get_velocity();
				if (vel.y < 0.0f) {
					vel.y *= p->jump_cut_multiplier;
					p->set_velocity(vel);
				}
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			// Horizontal air control
			float input = p->get_move_input();
			Vector2 vel = p->get_velocity();
			vel.x = Math::move_toward(vel.x, input * p->move_speed * p->air_horizontal_multiplier,
			                          float(p->move_speed * 5.0 * delta));

			// Apply gravity
			vel.y += p->get_gravity() * delta;

			// Transition to fall when descending
			if (vel.y >= 0.0f) {
				p->set_velocity(vel);
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			// Wall cling check
			if (p->is_on_wall() && !p->is_on_floor() && Math::abs(input) > 0.01f) {
				p->state_machine->transition_to(PlayerStates::WallCling);
				return;
			}

			// Attack in air
			if (p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Attack);
				return;
			}

			// Dash in air
			if (p->dash_just_pressed() && p->can_dash()) {
				p->state_machine->transition_to(PlayerStates::Dash);
				return;
			}

			// 空中再按跳：进入飞行（筑基借法器 / 金丹以上）
			if (p->jump_just_pressed() && p->can_fly()) {
				p->state_machine->transition_to(PlayerStates::Fly);
				return;
			}

			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Fall -------
	class PlayerFallState : public State<Player> {
	public:
		void enter(Player *p) override {}

		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			// Landed
			if (p->is_on_floor()) {
				// Jump buffer: player pressed jump just before landing
				if (p->jump_buffer.consume(p->get_time(), p->jump_buffer_time)) {
					p->state_machine->transition_to(PlayerStates::Jump);
				} else {
					p->state_machine->transition_to(PlayerStates::Idle);
				}
				return;
			}

			// Coyote time jump
			if (p->jump_just_pressed()) {
				if (p->left_ground_time >= 0.0 &&
				    (p->get_time() - p->left_ground_time) <= p->coyote_time) {
					p->state_machine->transition_to(PlayerStates::Jump);
					return;
				} else if (p->can_fly()) {
					// 空中再按跳：进入飞行
					p->state_machine->transition_to(PlayerStates::Fly);
					return;
				} else {
					p->jump_buffer.update("jump", p->get_time());
				}
			}

			// Wall cling
			float input = p->get_move_input();
			if (p->is_on_wall() && Math::abs(input) > 0.01f) {
				Vector2 vel = p->get_velocity();
				// Only cling if still next to wall in facing direction
				if ((input > 0 && p->get_last_slide_collision()->get_normal().x < 0) ||
				    (input < 0 && p->get_last_slide_collision()->get_normal().x > 0)) {
					p->state_machine->transition_to(PlayerStates::WallCling);
					return;
				}
			}

			// Attack in air
			if (p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Attack);
				return;
			}

			// Dash in air
			if (p->dash_just_pressed() && p->can_dash()) {
				p->state_machine->transition_to(PlayerStates::Dash);
				return;
			}

			// Horizontal air control
			Vector2 vel = p->get_velocity();
			vel.x = Math::move_toward(vel.x, input * p->move_speed * p->air_horizontal_multiplier,
			                          float(p->move_speed * 5.0 * delta));
			// Gravity
			vel.y += p->get_gravity() * delta;

			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Fly（筑基借法器耗灵力 / 金丹以上自由飞行）-------
	class PlayerFlyState : public State<Player> {
	public:
		void enter(Player *p) override { p->was_flying = true; }
		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			// 落地结束飞行
			if (p->is_on_floor()) {
				p->was_flying = false;
				p->state_machine->transition_to(PlayerStates::Idle);
				return;
			}

			// 再按跳跃退出飞行
			if (p->jump_just_pressed()) {
				p->was_flying = false;
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			// 筑基借法器飞行：持续消耗灵力，耗尽坠落
			float cost = p->flight_mana_cost_per_sec();
			if (cost > 0.0f && p->get_cultivation()) {
				if (!p->get_cultivation()->consume_mana(cost * delta)) {
					p->was_flying = false;
					p->state_machine->transition_to(PlayerStates::Fall);
					return;
				}
			}

			// 攻击 / 冲刺
			if (p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Attack);
				return;
			}
			if (p->dash_just_pressed() && p->can_dash()) {
				p->state_machine->transition_to(PlayerStates::Dash);
				return;
			}

			// 自由二维移动（无重力，渐加速；速度随境界速度倍率成长）
			float spd_mult = p->get_cultivation()
				? p->get_cultivation()->get_speed_multiplier() : 1.0f;
			float top_speed = p->fly_speed * spd_mult;
			float ix = p->get_move_input();
			float iy = p->get_fly_input();
			Vector2 vel = p->get_velocity();
			float accel = float(p->fly_acceleration * spd_mult * delta);
			vel.x = Math::move_toward(vel.x, ix * top_speed, accel);
			vel.y = Math::move_toward(vel.y, iy * top_speed, accel);
			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Wall Cling -------
	class PlayerWallClingState : public State<Player> {
	public:
		void enter(Player *p) override {
			// Reset dash on wall cling (classic metroidvania feel)
			p->dash_cooldown_end = 0.0;
		}

		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			// Landed on floor
			if (p->is_on_floor()) {
				p->state_machine->transition_to(PlayerStates::Idle);
				return;
			}

			// Wall jump
			if (p->jump_just_pressed()) {
				Vector2 vel = p->get_velocity();
				vel.y = p->wall_jump_vertical;
				int wall_normal_x = p->get_last_slide_collision()->get_normal().x > 0 ? 1 : -1;
				vel.x = -wall_normal_x * p->wall_jump_horizontal;
				p->set_velocity(vel);
				p->facing_direction = -wall_normal_x;
				p->left_ground_time = p->get_time();
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			// Attack from wall cling
			if (p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Attack);
				return;
			}

			// Stop clinging if moved away from wall or no wall contact
			float input = p->get_move_input();
			if (!p->is_on_wall() || Math::abs(input) < 0.01f) {
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}

			// Slide down
			Vector2 vel = p->get_velocity();
			vel.y = Math::min(vel.y, p->wall_slide_speed);
			vel.y += p->get_gravity() * delta * 0.3f; // reduced gravity while clinging

			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Dash -------
	class PlayerDashState : public State<Player> {
	public:
		void enter(Player *p) override {
			p->start_dash();
			Vector2 vel = p->get_velocity();
			vel.y = 0.0f; // cancel vertical momentum
			vel.x = p->facing_direction * p->dash_speed;
			p->set_velocity(vel);
		}

		void exit(Player *p) override {}

		void physics_update(Player *p, double delta) override {
			// Dash ends when timer runs out
			if (p->get_time() >= p->dash_end_time) {
				Vector2 vel = p->get_velocity();
				vel.x = p->get_move_input() * p->move_speed * p->air_horizontal_multiplier;
				p->set_velocity(vel);

				if (p->is_on_floor()) {
					p->state_machine->transition_to(PlayerStates::Idle);
				} else if (p->was_flying && p->can_fly()) {
					p->state_machine->transition_to(PlayerStates::Fly); // 空中冲刺后恢复飞行
				} else {
					p->state_machine->transition_to(PlayerStates::Fall);
				}
				return;
			}

			// Maintain dash velocity
			Vector2 vel = p->get_velocity();
			vel.x = p->facing_direction * p->dash_speed;
			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Attack (multi-phase with combo chaining) -------
	class PlayerAttackState : public State<Player> {
	public:
		void enter(Player *p) override {
			// Start combo chain and get current step
			int step = p->combo_chain.start_attack(p->get_time());

			// Setup HitBox with combo multipliers
			HitBox *hb = Object::cast_to<HitBox>(p->get_node_or_null("HitBox"));
			if (hb) {
				float realm_mult = p->get_cultivation() ? p->get_cultivation()->get_damage_multiplier() : 1.0f;
				float combo_mult = p->combo_chain.get_damage_multiplier();
				float kbr_mult = p->combo_chain.get_knockback_multiplier();
				hb->damage = p->attack_damage * realm_mult * combo_mult;
				hb->knockback_force = 200.0f * kbr_mult;
				hb->set_knockback_from_facing(p->facing_direction);
				hb->set_scale(Vector2((float)p->facing_direction, 1.0f));
				hb->set_active(false); // Will be activated during active phase
			}

			// Start in startup phase
			p->attack_phase = Player::STARTUP;
			p->attack_phase_end_time = p->get_time() + p->combo_chain.get_startup_time();

			// Broadcast combo step change
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("combo_changed", step + 1);
			}
		}

		void exit(Player *p) override {
			HitBox *hb = Object::cast_to<HitBox>(p->get_node_or_null("HitBox"));
			if (hb) hb->set_active(false);

			// If exiting without chaining to next attack, end the combo
			// (transitioned to non-attack state)
		}

		void physics_update(Player *p, double delta) override {
			double t = p->get_time();

			// Phase transitions
			switch (p->attack_phase) {
				case Player::STARTUP:
					if (t >= p->attack_phase_end_time) {
						// Activate hitbox
						HitBox *hb = Object::cast_to<HitBox>(p->get_node_or_null("HitBox"));
						if (hb) hb->set_active(true);
						p->attack_phase = Player::ACTIVE;
						p->attack_phase_end_time = t + p->combo_chain.get_active_time();
					}
					break;

				case Player::ACTIVE:
					if (t >= p->attack_phase_end_time) {
						// Deactivate hitbox
						HitBox *hb = Object::cast_to<HitBox>(p->get_node_or_null("HitBox"));
						if (hb) hb->set_active(false);
						p->attack_phase = Player::RECOVERY;
						p->attack_phase_end_time = t + p->combo_chain.get_recovery_time();
					}
					break;

				case Player::RECOVERY:
					// Check for combo chain: attack press during recovery buffers the next hit
					if (p->attack_just_pressed() && p->combo_chain.get_combo_step() < ComboChain::MAX_COMBO - 1) {
						// Chain to next attack — re-enter with same state
						p->state_machine->transition_to(PlayerStates::Attack);
						return;
					}

					if (t >= p->attack_phase_end_time) {
						// Recovery complete — transition out
						_exit_attack(p);
						return;
					}
					break;
			}

			// Apply minimal movement during attack (allow gravity in air, hover if flying)
			Vector2 vel = p->get_velocity();
			if (!p->is_on_floor()) {
				if (p->was_flying) {
					vel.y = Math::move_toward(vel.y, 0.0f, float(p->fly_acceleration * delta)); // 飞行中攻击保持悬浮
				} else {
					vel.y += p->get_gravity() * delta;
				}
			} else {
				vel.x = Math::move_toward(vel.x, 0.0f, float(p->move_speed * 8.0 * delta));
			}
			p->set_velocity(vel);
			p->move_and_slide();
		}

	private:
		void _exit_attack(Player *p) {
			// End combo and broadcast
			int final_count = p->combo_chain.get_hit_count();
			SignalBus *bus = SignalBus::get_singleton();
			if (bus && final_count > 0) {
				bus->emit_signal("combo_ended", final_count);
			}

			if (p->is_on_floor()) {
				if (Math::abs(p->get_move_input()) > 0.01f) {
					p->state_machine->transition_to(PlayerStates::Run);
				} else {
					p->state_machine->transition_to(PlayerStates::Idle);
				}
			} else if (p->was_flying && p->can_fly()) {
				p->state_machine->transition_to(PlayerStates::Fly); // 空中攻击后恢复飞行
			} else {
				p->state_machine->transition_to(PlayerStates::Fall);
			}
		}
	};

	// ============================================================
	// Player implementation
	// ============================================================

	void Player::_bind_methods() {
		ClassDB::bind_method(D_METHOD("get_move_input"), &Player::get_move_input);
		ClassDB::bind_method(D_METHOD("get_gravity"), &Player::get_gravity);
		ClassDB::bind_method(D_METHOD("take_damage", "amount", "source"), &Player::take_damage);
		ClassDB::bind_method(D_METHOD("gain_spiritual_energy", "amount"), &Player::gain_spiritual_energy);
		ClassDB::bind_method(D_METHOD("on_attack_landed", "victim", "damage"), &Player::on_attack_landed);
		ClassDB::bind_method(D_METHOD("_on_hurtbox_hit", "hitbox", "source"), &Player::_on_hurtbox_hit);
		ClassDB::bind_method(D_METHOD("pickup_item", "item_id", "qty"), &Player::pickup_item, DEFVAL(1));
		ClassDB::bind_method(D_METHOD("get_inventory"), &Player::get_inventory);
		ClassDB::bind_method(D_METHOD("get_cultivation"), &Player::get_cultivation);
		ClassDB::bind_method(D_METHOD("get_ability_manager"), &Player::get_ability_manager);
		ClassDB::bind_method(D_METHOD("equip_item", "inventory_slot"), &Player::equip_item);
		ClassDB::bind_method(D_METHOD("unequip_item", "equip_slot"), &Player::unequip_item);
		ClassDB::bind_method(D_METHOD("get_equipment_in_slot", "slot"), &Player::get_equipment_in_slot);
		ClassDB::bind_method(D_METHOD("get_equip_bonus_attack"), &Player::get_equip_bonus_attack);
		ClassDB::bind_method(D_METHOD("get_equip_bonus_defense"), &Player::get_equip_bonus_defense);
		ClassDB::bind_method(D_METHOD("get_equip_bonus_speed"), &Player::get_equip_bonus_speed);
		ClassDB::bind_method(D_METHOD("get_effective_attack"), &Player::get_effective_attack);
		ClassDB::bind_method(D_METHOD("_on_ability_unlocked", "ability_id"), &Player::_on_ability_unlocked);
		ClassDB::bind_method(D_METHOD("_on_cultivation_realm_changed", "old_realm", "new_realm"), &Player::_on_cultivation_realm_changed);
		ClassDB::bind_method(D_METHOD("set_benming_artifact", "item_id"), &Player::set_benming_artifact);
		ClassDB::bind_method(D_METHOD("get_benming_artifact"), &Player::get_benming_artifact);
		ClassDB::bind_method(D_METHOD("get_benming_coeff"), &Player::get_benming_coeff);
		ClassDB::bind_method(D_METHOD("get_benming_nurture"), &Player::get_benming_nurture);
		ClassDB::bind_method(D_METHOD("is_benming_awakened"), &Player::is_benming_awakened);
		ClassDB::bind_method(D_METHOD("nurture_benming", "amount"), &Player::nurture_benming);
		ClassDB::bind_method(D_METHOD("awaken_benming_artifact"), &Player::awaken_benming_artifact);
		ClassDB::bind_method(D_METHOD("get_artifact_slot_limit"), &Player::get_artifact_slot_limit);

		ADD_SIGNAL(MethodInfo("player_died"));
		ADD_SIGNAL(MethodInfo("player_damaged", PropertyInfo(Variant::FLOAT, "amount")));
	}

	void Player::_ready() {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		current_health = max_health;
		for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
			_equipment[i] = StringName();
		}
		_setup_collision();
		_create_hitboxes();
		_create_cultivation();
		_create_inventory();

		state_machine = new StateMachine<Player>(this);

		state_machine->add_state(PlayerStates::Idle, new PlayerIdleState());
		state_machine->add_state(PlayerStates::Run, new PlayerRunState());
		state_machine->add_state(PlayerStates::Jump, new PlayerJumpState());
		state_machine->add_state(PlayerStates::Fall, new PlayerFallState());
		state_machine->add_state(PlayerStates::WallCling, new PlayerWallClingState());
		state_machine->add_state(PlayerStates::Dash, new PlayerDashState());
		state_machine->add_state(PlayerStates::Attack, new PlayerAttackState());
		state_machine->add_state(PlayerStates::Fly, new PlayerFlyState());

		state_machine->set_initial_state(PlayerStates::Idle);
	}

	void Player::_physics_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		// 灵力（法力）缓慢自动回复
		if (_cultivation) {
			_cultivation->tick_mana_regen(p_delta);
		}

		state_machine->physics_update(p_delta);
	}

	void Player::_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		_time += p_delta;
		_update_buffers();
		_update_facing();
		combo_chain.update(_time);

		// Q 修炼：尝试突破（调试开关打开时无经验门槛）
		if (Input::get_singleton()->is_action_just_pressed("cultivate")) {
			if (_cultivation) {
				bool ok = _cultivation->attempt_breakthrough();
				UtilityFunctions::print(String("[DEBUG] Q pressed, breakthrough = ") +
					(ok ? "SUCCESS" : "FAILED") + ", realm=" +
					String::num_int64(_cultivation->get_realm_index()) +
					", free=" + (_cultivation->is_free_breakthrough() ? "ON" : "OFF"));
			} else {
				UtilityFunctions::print("[DEBUG] Q pressed, but cultivation is null");
			}
		}

		state_machine->process_update(p_delta);
	}

	void Player::_update_buffers() {
		jump_buffer.update("jump", _time);
		dash_buffer.update("dash", _time);
	}

	void Player::_update_facing() {
		float input = get_move_input();
		if (Math::abs(input) > 0.01f) {
			facing_direction = (input > 0) ? 1 : -1;
		}
	}

	// ---- Accessors ----

	float Player::get_gravity() const {
		// Use hardcoded fallback since ProjectSettings may not have 2d physics configured
		float default_gravity = 980.0f;
		Variant g = ProjectSettings::get_singleton()->get_setting("physics/2d/default_gravity");
		if (g.get_type() != Variant::NIL) {
			default_gravity = float(g);
		}
		float scale = 1.0f;
		Variant s = ProjectSettings::get_singleton()->get_setting("physics/2d/default_gravity_scale");
		if (s.get_type() != Variant::NIL) {
			scale = float(s);
		}
		return default_gravity * scale;
	}

	float Player::get_move_input() const {
		Input *input = Input::get_singleton();
		float val = 0.0f;
		if (input->is_action_pressed("right"))
			val += 1.0f;
		if (input->is_action_pressed("left"))
			val -= 1.0f;
		return Math::clamp(val, -1.0f, 1.0f);
	}

	// ---- 飞行 ----

	bool Player::can_fly() const {
		if (!_abilities)
			return false;
		// 金丹以上：无条件飞行
		if (_abilities->has_ability(StringName(AbilityManager::ABILITY_FREE_FLIGHT)))
			return true;
		// 筑基：需飞行法器协助（飞剑）
		if (_abilities->has_ability(StringName(AbilityManager::ABILITY_SHORT_FLIGHT))) {
			return _inventory && _inventory->has_item(StringName("flying_sword"));
		}
		return false;
	}

	float Player::get_fly_input() const {
		Input *input = Input::get_singleton();
		float val = 0.0f;
		if (input->is_action_pressed("down"))
			val += 1.0f;
		if (input->is_action_pressed("up"))
			val -= 1.0f;
		return Math::clamp(val, -1.0f, 1.0f);
	}

	float Player::flight_mana_cost_per_sec() const {
		// 金丹以上无条件飞行，不耗灵力；筑基借法器，消耗较高
		if (_abilities && _abilities->has_ability(StringName(AbilityManager::ABILITY_FREE_FLIGHT)))
			return 0.0f;
		return 10.0f;
	}

	bool Player::jump_just_pressed() const {
		return Input::get_singleton()->is_action_just_pressed("jump");
	}

	bool Player::jump_held() const {
		return Input::get_singleton()->is_action_pressed("jump");
	}

	bool Player::dash_just_pressed() const {
		return Input::get_singleton()->is_action_just_pressed("dash");
	}

	bool Player::attack_just_pressed() const {
		return Input::get_singleton()->is_action_just_pressed("attack");
	}

	bool Player::attack_held() const {
		return Input::get_singleton()->is_action_pressed("attack");
	}

	bool Player::can_dash() const {
		return _time >= dash_cooldown_end;
	}

	void Player::start_dash() {
		dash_end_time = _time + dash_duration;
		dash_cooldown_end = _time + dash_duration + dash_cooldown;
	}

	void Player::take_damage(float p_amount, Node *p_source) {
		// Apply defense from equipment + cultivation
		float defense = get_equip_bonus_defense();
		if (_cultivation) {
			defense *= _cultivation->get_defense_multiplier();
		}
		float actual_damage = Math::max(p_amount - defense, 1.0f);
		current_health -= actual_damage;

		// Broadcast through global signal bus
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("player_health_changed", current_health, max_health);
			bus->emit_signal("player_damaged", p_amount, p_source);
		}

		if (current_health <= 0.0f) {
			current_health = 0.0f;
			emit_signal("player_died");
			if (bus) {
				bus->emit_signal("player_died");
			}
			return;
		}

		emit_signal("player_damaged", p_amount);
	}

	void Player::_on_hurtbox_hit(Object *p_hitbox, Node *p_source) {
		HitBox *hb = Object::cast_to<HitBox>(p_hitbox);
		if (!hb) return;

		take_damage(hb->damage, p_source);
	}

	void Player::_setup_collision() {
		// Player body is layer 3, collides with layers 1,2,4
		set_collision_layer_value(3, true);  // layer 3 = Player
		set_collision_mask_value(1, true);   // Ground
		set_collision_mask_value(2, true);   // One Way Platform
	}

	void Player::_create_hitboxes() {
		// HitBox for dealing damage to enemies
		_hitbox = memnew(HitBox);
		_hitbox->set_name("HitBox");
		_hitbox->damage = attack_damage;
		_hitbox->set_active(false);
		// HitBox IS layer 5, DETECTS layer 4 (Enemy) for direct damage
		_hitbox->set_collision_layer_value(5, true);
		_hitbox->set_collision_mask_value(4, true);
		_hitbox->connect("area_entered", Callable(_hitbox, "_on_area_entered"));
		_hitbox->connect("hit_landed", Callable(this, "on_attack_landed"));

		CollisionShape2D *hb_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> hb_rect;
		hb_rect.instantiate();
		hb_rect->set_size(Vector2(30, 20));
		hb_shape->set_shape(hb_rect);
		hb_shape->set_position(Vector2(15, 0));
		_hitbox->add_child(hb_shape);

		// Visual for the hitbox
		Polygon2D *hb_visual = memnew(Polygon2D);
		hb_visual->set_name("HitBoxVisual");
		hb_visual->set_visible(false);
		hb_visual->set_color(Color(0.3, 1.0, 0.3, 0.4));
		PackedVector2Array hb_poly;
		hb_poly.append(Vector2(0, -10));
		hb_poly.append(Vector2(30, -10));
		hb_poly.append(Vector2(30, 10));
		hb_poly.append(Vector2(0, 10));
		hb_visual->set_polygon(hb_poly);
		hb_visual->set_position(Vector2(15, 0));
		_hitbox->add_child(hb_visual);

		add_child(_hitbox);

		// HurtBox for receiving damage from enemies
		_hurtbox = memnew(HurtBox);
		_hurtbox->set_name("HurtBox");
		// HurtBox: layer 3, monitors Enemy HitBox on layer 6
		_hurtbox->set_collision_layer_value(3, true); // layer 3 = Player
		_hurtbox->set_collision_mask_value(6, true);  // layer 6 = Enemy HitBox

		CollisionShape2D *hu_shape = memnew(CollisionShape2D);
		Ref<RectangleShape2D> hu_rect;
		hu_rect.instantiate();
		hu_rect->set_size(Vector2(24, 32));
		hu_shape->set_shape(hu_rect);
		_hurtbox->add_child(hu_shape);
		add_child(_hurtbox);

		// Connect hurt signal
		_hurtbox->connect("hurtbox_hit", Callable(this, "_on_hurtbox_hit"));
	}

	void Player::_create_cultivation() {
		_cultivation = memnew(CultivationSystem);
		_abilities = memnew(AbilityManager);
		_abilities->set_cultivation(_cultivation);
		_abilities->connect("ability_unlocked", Callable(this, "_on_ability_unlocked"));
		_cultivation->connect("realm_changed", Callable(this, "_on_cultivation_realm_changed"));
		_abilities->check_realm_unlocks();

		// Store base speed before cultivation multiplier
		base_move_speed = move_speed;
		_update_move_speed();

		// 生命上限随境界（初始：凡人 100）
		max_health = float(_cultivation->get_max_health());
		current_health = max_health;
	}

	void Player::_create_inventory() {
		_inventory = memnew(Inventory);
		// 纳戒已解锁（如读档恢复）则放开容量
		if (_abilities && _abilities->has_ability(StringName(AbilityManager::ABILITY_STORAGE_RING))) {
			_inventory->unlock_unlimited();
		}
	}

	void Player::_on_ability_unlocked(const StringName &p_ability_id) {
		// 纳戒/储物袋：背包容量 24 → 无限
		if (p_ability_id == StringName(AbilityManager::ABILITY_STORAGE_RING) && _inventory) {
			_inventory->unlock_unlimited();
		}
	}

	void Player::_on_cultivation_realm_changed(int p_old_realm, int p_new_realm) {
		_abilities->check_realm_unlocks();
		_update_move_speed();

		// 突破洗髓：生命上限提升并回满
		if (_cultivation) {
			max_health = float(_cultivation->get_max_health());
			current_health = max_health;
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("player_health_changed", current_health, max_health);
			}
		}

		// 渡劫成仙：本命法宝觉醒（150% → 200%）
		if (p_old_realm < CultivationSystem::TRUE_IMMORTAL &&
		    p_new_realm >= CultivationSystem::TRUE_IMMORTAL) {
			awaken_benming_artifact();
		}
	}

	// ---- 法宝系统（本命法宝）----

	// 温养满所需进度
	static constexpr float BENMING_NURTURE_MAX = 1000.0f;

	void Player::set_benming_artifact(const StringName &p_item_id) {
		// 飞升后不可更换（托塔李天王模式：终身绑定）
		if (_benming_awakened) return;
		_benming_item = p_item_id;
		_benming_nurture = 0.0f; // 换本命重新温养
	}

	float Player::get_benming_coeff() const {
		if (_benming_item.is_empty())
			return 1.0f;
		float t = Math::clamp(_benming_nurture / BENMING_NURTURE_MAX, 0.0f, 1.0f);
		if (_benming_awakened) {
			return 1.5f + 0.5f * t; // 觉醒：150% → 200%
		}
		return 1.2f + 0.3f * t; // 温养：120% → 150%
	}

	void Player::nurture_benming(float p_amount) {
		if (_benming_item.is_empty()) return;
		_benming_nurture = Math::min(_benming_nurture + p_amount, BENMING_NURTURE_MAX);
	}

	void Player::awaken_benming_artifact() {
		if (_benming_item.is_empty()) return;
		_benming_awakened = true;
	}

	int Player::get_artifact_slot_limit() const {
		bool immortal = _cultivation && _cultivation->is_immortal();
		return immortal ? 6 : 3; // 飞升前 1本命+2次要；飞升后 1本命+5次要
	}

	void Player::_update_move_speed() {
		move_speed = base_move_speed;
		if (_cultivation) {
			move_speed *= _cultivation->get_speed_multiplier();
		}
		move_speed *= (1.0f + get_equip_bonus_speed());
	}

	// ---- Equipment ----

	float Player::get_effective_attack() const {
		float atk = attack_damage;
		atk += get_equip_bonus_attack();
		if (_cultivation) {
			atk *= _cultivation->get_damage_multiplier();
		}
		atk *= get_benming_coeff(); // 本命法宝加成
		return atk;
	}

	bool Player::equip_item(int p_inventory_slot) {
		if (!_inventory) return false;

		Dictionary slot_data = _inventory->get_slot(p_inventory_slot);
		if (slot_data.is_empty()) return false;

		StringName item_id = slot_data["id"];
		const Item *def = ItemDatabase::get_singleton()->get_item(item_id);
		if (!def || def->type != Item::EQUIPMENT) return false;

		int equip_slot = static_cast<int>(def->equip_slot);
		if (equip_slot < 0 || equip_slot >= EQUIP_SLOT_COUNT) return false;

		// If something already equipped in this slot, swap it back to inventory
		if (!_equipment[equip_slot].is_empty()) {
			StringName old_id = _equipment[equip_slot];
			bool can_swap = _inventory->add_item(old_id, 1);
			if (!can_swap) return false; // No room to unequip old item
		}

		// Remove from inventory
		bool removed = _inventory->remove_item(item_id, 1);
		if (!removed) return false;

		// Equip
		_equipment[equip_slot] = item_id;
		_update_move_speed();

		return true;
	}

	bool Player::unequip_item(int p_equip_slot) {
		if (p_equip_slot < 0 || p_equip_slot >= EQUIP_SLOT_COUNT) return false;
		if (_equipment[p_equip_slot].is_empty()) return false;
		if (!_inventory) return false;

		StringName item_id = _equipment[p_equip_slot];
		bool added = _inventory->add_item(item_id, 1);
		if (!added) return false; // Inventory full

		_equipment[p_equip_slot] = StringName();
		_update_move_speed();

		return true;
	}

	StringName Player::get_equipment_in_slot(int p_slot) const {
		if (p_slot < 0 || p_slot >= EQUIP_SLOT_COUNT) return StringName();
		return _equipment[p_slot];
	}

	float Player::get_equip_bonus_attack() const {
		float bonus = 0.0f;
		for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
			if (!_equipment[i].is_empty()) {
				const Item *def = ItemDatabase::get_singleton()->get_item(_equipment[i]);
				if (def) bonus += def->attack_bonus;
			}
		}
		return bonus;
	}

	float Player::get_equip_bonus_defense() const {
		float bonus = 0.0f;
		for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
			if (!_equipment[i].is_empty()) {
				const Item *def = ItemDatabase::get_singleton()->get_item(_equipment[i]);
				if (def) bonus += def->defense_bonus;
			}
		}
		return bonus;
	}

	float Player::get_equip_bonus_speed() const {
		float bonus = 0.0f;
		for (int i = 0; i < EQUIP_SLOT_COUNT; i++) {
			if (!_equipment[i].is_empty()) {
				const Item *def = ItemDatabase::get_singleton()->get_item(_equipment[i]);
				if (def) bonus += def->speed_bonus;
			}
		}
		return bonus;
	}

	// ---- Inventory ----

	void Player::pickup_item(const StringName &p_item_id, int p_qty) {
		if (!_inventory) return;

		const Item *def = ItemDatabase::get_singleton()->get_item(p_item_id);
		if (!def) return;

		// Try to add to inventory
		bool added = _inventory->add_item(p_item_id, p_qty);
		if (!added) {
			return; // Inventory full
		}

		// Emit pickup signal
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("item_picked_up", String(p_item_id), p_qty);
		}

		// Auto-use consumables if conditions are met
		if (def->type == Item::CONSUMABLE) {
			bool should_use = false;

			// Auto-use healing pills when below 70% health
			if (def->heal_amount > 0.0f && current_health < max_health * 0.7f) {
				should_use = true;
			}
			// Auto-use energy pills when below 50% max energy
			if (def->energy_amount > 0.0f && _cultivation &&
			    _cultivation->get_current_energy() < _cultivation->get_max_energy() / 2) {
				should_use = true;
			}

			if (should_use) {
				// Find the item slot and use it
				for (int i = 0; i < _inventory->get_capacity(); i++) {
					Dictionary slot = _inventory->get_slot(i);
					if (!slot.is_empty() && StringName(slot["id"]) == p_item_id) {
						if (_inventory->use_item(i)) {
							// Apply effect
							if (def->heal_amount > 0.0f) {
								current_health = Math::min(current_health + def->heal_amount, max_health);
								if (bus) {
									bus->emit_signal("player_health_changed", current_health, max_health);
									bus->emit_signal("item_used", String(p_item_id), 1);
								}
							}
							if (def->energy_amount > 0.0f && _cultivation) {
								_cultivation->accumulate_energy(def->energy_amount);
								if (bus) {
									bus->emit_signal("item_used", String(p_item_id), 1);
								}
							}
						}
						break;
					}
				}
			}
		}
	}

	// ---- Cultivation ----

	void Player::gain_spiritual_energy(float p_amount) {
		if (_cultivation) {
			_cultivation->accumulate_energy(p_amount);
		}
	}

	// ---- Save / Load ----

	void Player::apply_save_data(const Dictionary &p_data) {
		// Health
		if (p_data.has("health")) {
			current_health = float(p_data["health"]);
		}
		if (p_data.has("max_health")) {
			max_health = float(p_data["max_health"]);
		}

		// Position
		if (p_data.has("position_x") && p_data.has("position_y")) {
			set_global_position(Vector2(
				float(p_data["position_x"]),
				float(p_data["position_y"])));
		}
	}

	// ---- Combat ----

	void Player::on_attack_landed(Node *p_victim, float p_damage) {
		combo_chain.on_hit_landed(_time);

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("combo_changed", combo_chain.get_hit_count());
		}
	}

} // namespace godot
