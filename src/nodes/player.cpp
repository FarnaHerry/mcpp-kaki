#include "player.h"
#include "enemy.h"
#include "dongtian_manager.h"


#include "../utils/text.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

import mcpp_kaki.combat;
import mcpp_kaki.cultivation;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;
namespace godot {

	Player::~Player() {
		// 释放 memnew 的 Object 成员（Object 非 RefCounted，不随 Node 释放）
		if (_cultivation) memdelete(_cultivation);
		if (_abilities) memdelete(_abilities);
		if (_gongfa) memdelete(_gongfa);
		if (_skills) memdelete(_skills);
		if (_artifacts) memdelete(_artifacts);
		if (_buffs) memdelete(_buffs);
		if (_sect) memdelete(_sect);
		if (_alchemy) memdelete(_alchemy);
		if (_inventory) memdelete(_inventory);
	}

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
		inline constexpr const char *Meditate = "meditate";
	} // namespace PlayerStates

	// ------- Idle -------
	class PlayerIdleState : public State<Player> {
	public:
		void enter(Player *p) override { p->was_flying = false; p->air_jump_used = false; }
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
			// 冰面（北俱芦洲）：摩擦骤减，惯性滑行
			float fric = p->is_slippery() ? 1.5f : 10.0f;
			vel.x = Math::move_toward(vel.x, 0.0f, float(p->move_speed * fric * delta));
			p->set_velocity(vel);
			p->move_and_slide();
		}
	};

	// ------- Run -------
	class PlayerRunState : public State<Player> {
	public:
		void enter(Player *p) override { p->air_jump_used = false; }
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
			if (p->is_slippery()) {
				// 冰面：渐进加速（动量保持，松键滑行而非骤停）
				vel.x = Math::move_toward(vel.x, input * p->move_speed, float(p->move_speed * 8.0 * delta));
			} else {
				vel.x = input * p->move_speed;
			}
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

			// 空中再按跳：先用二段跳（炼气），已用则进入飞行（筑基借法器 / 金丹以上）
			if (p->jump_just_pressed()) {
				if (p->can_air_jump()) {
					p->air_jump_used = true;
					p->state_machine->transition_to(PlayerStates::Jump);
					return;
				}
				if (p->can_fly()) {
					p->state_machine->transition_to(PlayerStates::Fly);
					return;
				}
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
				} else if (p->can_air_jump()) {
					// 二段跳（炼气解锁，每离地一次；攀墙刷新）
					p->air_jump_used = true;
					p->state_machine->transition_to(PlayerStates::Jump);
					return;
				} else if (p->can_fly()) {
					// 二段跳已用，再按跳：进入飞行
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

			// 弱水区禁飞：进入即坠（流沙河）
			if (p->is_flight_blocked()) {
				p->was_flying = false;
				p->state_machine->transition_to(PlayerStates::Fall);
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
				float spent = cost * float(delta);
				if (!p->get_cultivation()->consume_mana(spent)) {
					p->was_flying = false;
					p->state_machine->transition_to(PlayerStates::Fall);
					return;
				}
				// 练气行为：御剑耗灵喂养功法
				if (p->get_gongfa()) {
					p->get_gongfa()->feed(GongfaSystem::SCHOOL_QI, spent);
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
			float top_speed = p->fly_speed * spd_mult *
				(p->get_skills() ? p->get_skills()->get_passive_fly_mult() : 1.0f); // 被动（风雷双翼）
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

	// ------- Meditate（打坐：平常修炼 + 突破入口，Q 入坐/收功）-------
	class PlayerMeditateState : public State<Player> {
		double _sit_time = 0.0;    // 入坐时长（1s 后自动请求突破）
		double _energy_frac = 0.0; // 修为小数积累（每帧增量可能 <1）
		bool _bt_fired = false;    // 每次入坐最多请求一次突破（失败需重新入坐）
	public:
		void enter(Player *p) override {
			_sit_time = 0.0;
			_energy_frac = 0.0;
			_bt_fired = false;
			Vector2 vel = p->get_velocity();
			vel.x = 0.0f;
			p->set_velocity(vel);
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				double jlz = p->get_dongtian_meditate_mult();
				String jlz_text = jlz > 1.0 ?
					LOC(" · 聚灵阵×") + String::num(jlz, 2) : String();
				bus->emit_signal("interaction_prompt",
					LOC("打坐中 · 修为+") + String::num(p->get_meditate_rate(), 1) +
					LOC("/s") + jlz_text + LOC(" · 灵力回复×3（移动/受击收功）"), true);
			}
		}
		void exit(Player *p) override {
			p->set_modulate(Color(1, 1, 1));
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("interaction_prompt", String(), false);
			}
		}

		void physics_update(Player *p, double delta) override {
			// 收功：离开地面 / 移动 / 跳 / 冲 / 攻（技能键本帧 _process 已施放，这里只负责收功）
			if (!p->is_on_floor()) {
				p->state_machine->transition_to(PlayerStates::Fall);
				return;
			}
			if (Math::abs(p->get_move_input()) > 0.01f || p->jump_just_pressed() ||
				p->dash_just_pressed() || p->attack_just_pressed()) {
				p->state_machine->transition_to(PlayerStates::Idle);
				return;
			}

			CultivationSystem *c = p->get_cultivation();
			if (c) {
				// 修为：纯打坐约 8 分钟满一境，击杀/丹药仍是主来源
				_energy_frac += p->get_meditate_rate() * delta;
				if (_energy_frac >= 1.0) {
					int64_t whole = int64_t(_energy_frac);
					_energy_frac -= double(whole);
					c->accumulate_energy(whole);
				}
				// 灵力回复 ×3（基础 tick 在 _physics_process 已 ×1，这里补 ×2）
				c->tick_mana_regen(delta * 2.0);

				// 修为封顶（或 F5 调试无门槛）→ 入定 1s 后自动请求机缘突破
				_sit_time += delta;
				if (!_bt_fired && _sit_time >= 1.0 &&
					(c->is_free_breakthrough() || (!c->is_max_realm() && c->get_realm_progress() >= 1.0))) {
					_bt_fired = true;
					SignalBus *bus = SignalBus::get_singleton();
					if (bus) {
						bus->emit_signal("breakthrough_requested");
					}
				}
			}

			// 灵气呼吸：靛蓝脉冲
			float pulse = 0.5f + 0.5f * Math::sin(float(p->get_time()) * 3.0f);
			p->set_modulate(Color(0.7f + 0.3f * pulse, 0.8f + 0.2f * pulse, 1.0f));

			p->move_and_slide();
		}
	};

	// ------- Wall Cling -------
	class PlayerWallClingState : public State<Player> {
	public:
		void enter(Player *p) override {
			// Reset dash + air jump on wall cling (classic metroidvania feel)
			p->dash_cooldown_end = 0.0;
			p->air_jump_used = false;
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
				hb->damage_category = DMG_PHYSICAL;
				hb->element = ELEM_NONE;
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
					// 后摇可取消：跳跃/冲刺优先（被贴身围攻时不再"按不出跳"）
					if (p->is_on_floor() && p->jump_just_pressed()) {
						HitBox *hb = Object::cast_to<HitBox>(p->get_node_or_null("HitBox"));
						if (hb) hb->set_active(false);
						p->state_machine->transition_to(PlayerStates::Jump);
						return;
					}
					if (p->dash_just_pressed() && p->can_dash()) {
						HitBox *hb = Object::cast_to<HitBox>(p->get_node_or_null("HitBox"));
						if (hb) hb->set_active(false);
						p->state_machine->transition_to(PlayerStates::Dash);
						return;
					}

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
		ClassDB::bind_method(D_METHOD("get_current_health"), &Player::get_current_health);
		ClassDB::bind_method(D_METHOD("get_max_health"), &Player::get_max_health);
		ClassDB::bind_method(D_METHOD("set_current_health", "v"), &Player::set_current_health);
		ClassDB::bind_method(D_METHOD("set_last_damage_source", "v"), &Player::set_last_damage_source);
		ClassDB::bind_method(D_METHOD("get_last_damage_source"), &Player::get_last_damage_source);
		ClassDB::bind_method(D_METHOD("is_invulnerable"), &Player::is_invulnerable);
		ClassDB::bind_method(D_METHOD("is_meditating"), &Player::is_meditating);
		ClassDB::bind_method(D_METHOD("get_meditate_rate"), &Player::get_meditate_rate);
		ClassDB::bind_method(D_METHOD("get_dongtian_meditate_mult"), &Player::get_dongtian_meditate_mult);
		ClassDB::bind_method(D_METHOD("get_state_name"), &Player::get_state_name);
		ClassDB::bind_method(D_METHOD("get_time"), &Player::get_time);
		ClassDB::bind_method(D_METHOD("_on_gongfa_changed"), &Player::_on_gongfa_changed);
		ClassDB::bind_method(D_METHOD("_on_enemy_killed", "enemy", "killer"), &Player::_on_enemy_killed);
		ClassDB::bind_method(D_METHOD("_on_skills_changed"), &Player::_on_skills_changed);
	ClassDB::bind_method(D_METHOD("take_damage_typed", "amount", "cat", "elem", "source"), &Player::take_damage_typed);
		ClassDB::bind_method(D_METHOD("gain_spiritual_energy", "amount"), &Player::gain_spiritual_energy);
		ClassDB::bind_method(D_METHOD("on_attack_landed", "victim", "damage"), &Player::on_attack_landed);
		ClassDB::bind_method(D_METHOD("_on_hurtbox_hit", "hitbox", "source"), &Player::_on_hurtbox_hit);
		ClassDB::bind_method(D_METHOD("pickup_item", "item_id", "qty"), &Player::pickup_item, DEFVAL(1));
		ClassDB::bind_method(D_METHOD("join_sect", "sect_id"), &Player::join_sect);
		ClassDB::bind_method(D_METHOD("leave_sect"), &Player::leave_sect);
		ClassDB::bind_method(D_METHOD("get_sect_system"), &Player::get_sect_system);
		ClassDB::bind_method(D_METHOD("cast_wei_pressure"), &Player::cast_wei_pressure);
		ClassDB::bind_method(D_METHOD("cast_lin_pressure"), &Player::cast_lin_pressure);
		ClassDB::bind_method(D_METHOD("get_wei_cooldown_left"), &Player::get_wei_cooldown_left);
		ClassDB::bind_method(D_METHOD("get_lin_cooldown_left"), &Player::get_lin_cooldown_left);
		ClassDB::bind_method(D_METHOD("use_consumable", "item_id"), &Player::use_consumable);
		ClassDB::bind_method(D_METHOD("get_consumable_bar_slot", "idx"), &Player::get_consumable_bar_slot);
		ClassDB::bind_method(D_METHOD("use_consumable_bar_slot", "idx"), &Player::use_consumable_bar_slot);
		ClassDB::bind_method(D_METHOD("get_fullness"), &Player::get_fullness);
		ClassDB::bind_method(D_METHOD("get_max_fullness"), &Player::get_max_fullness);
		ClassDB::bind_method(D_METHOD("set_fullness", "v"), &Player::set_fullness);
		ClassDB::bind_method(D_METHOD("is_bigu"), &Player::is_bigu);
		ClassDB::bind_method(D_METHOD("can_fly"), &Player::can_fly);
		ClassDB::bind_method(D_METHOD("get_food_mult"), &Player::get_food_mult);
		ClassDB::bind_method(D_METHOD("set_flight_blocked", "v"), &Player::set_flight_blocked);
		ClassDB::bind_method(D_METHOD("is_flight_blocked"), &Player::is_flight_blocked);
		ClassDB::bind_method(D_METHOD("set_slippery", "v"), &Player::set_slippery);
		ClassDB::bind_method(D_METHOD("is_slippery"), &Player::is_slippery);
		ClassDB::bind_method(D_METHOD("set_chilled", "v"), &Player::set_chilled);
		ClassDB::bind_method(D_METHOD("is_chilled"), &Player::is_chilled);
		ClassDB::bind_method(D_METHOD("get_inventory"), &Player::get_inventory);
		ClassDB::bind_method(D_METHOD("get_cultivation"), &Player::get_cultivation);
	ClassDB::bind_method(D_METHOD("get_gongfa"), &Player::get_gongfa);
	ClassDB::bind_method(D_METHOD("get_skills"), &Player::get_skills);
	ClassDB::bind_method(D_METHOD("get_artifacts"), &Player::get_artifacts);
	ClassDB::bind_method(D_METHOD("get_buffs"), &Player::get_buffs);
	ClassDB::bind_method(D_METHOD("get_alchemy"), &Player::get_alchemy);
	ClassDB::bind_method(D_METHOD("get_skill_page"), &Player::get_skill_page);
	ClassDB::bind_method(D_METHOD("toggle_skill_page"), &Player::toggle_skill_page);
	ClassDB::bind_method(D_METHOD("_on_interaction_prompt", "text", "show"), &Player::_on_interaction_prompt);
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
		state_machine->add_state(PlayerStates::Meditate, new PlayerMeditateState());

		state_machine->set_initial_state(PlayerStates::Idle);
	}

	void Player::_physics_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		// 灵力（法力）缓慢自动回复
		if (_cultivation) {
			_cultivation->tick_mana_regen(p_delta);
			_cultivation->tick_law_regen(p_delta); // 法则之力（化神后）
		}
		if (_buffs) {
			_buffs->tick(p_delta); // buff 计时（到期自动消失）
		}

		_update_fullness(p_delta);

		state_machine->physics_update(p_delta);
	}

	bool Player::is_bigu() const {
		// 筑基辟谷：不再需要用食维生
		return _cultivation && _cultivation->get_realm_index() >= CultivationSystem::FOUNDATION;
	}

	float Player::get_food_mult() const {
		// 食物效果倍率：凡人 1.0 / 炼气起 1.2（design/cultivation-realms.md L167）
		if (_cultivation && _cultivation->get_realm_index() >= CultivationSystem::QI_REFINING)
			return 1.2f;
		return 1.0f;
	}

	void Player::_update_fullness(double p_delta) {
		float before = _fullness;
		if (is_bigu()) {
			// 辟谷：饱食度锁定满（条隐藏由 HUD 处理）
			if (before != _max_fullness)
				_fullness = _max_fullness;
			return;
		}
		// 凡人/炼气：随时间衰减（满→空约 5.5 分钟）
		if (_fullness > 0.0f) {
			_fullness = Math::max(0.0f, _fullness - 0.3f * float(p_delta));
		}
		// 饥饿 debuff（force-managed）：归零 apply / 回正 remove
		if (_buffs) {
			bool hungry = _fullness <= 0.0f;
			bool has_hunger = _buffs->has("buff_hunger");
			if (hungry && !has_hunger)
				_buffs->apply("buff_hunger");
			else if (!hungry && has_hunger)
				_buffs->remove("buff_hunger");
		}
		if (before != _fullness)
			_emit_fullness();
	}

	void Player::_emit_fullness() {
		SignalBus *bus = SignalBus::get_singleton();
		if (bus)
			bus->emit_signal("fullness_changed", _fullness, _max_fullness);
	}

	void Player::_process(double p_delta) {
		if (Engine::get_singleton()->is_editor_hint())
			return;

		_time += p_delta;
		_update_buffers();
		_update_facing();
		combo_chain.update(_time);

		// B 键整页切换：战斗页 ↔ 法宝页（页机制通用，后续可扩技能多页）
		if (Input::get_singleton()->is_action_just_pressed("artifact_page")) {
			toggle_skill_page();
		}

		// 技能槽输入（DNF 式字母区）：战斗页=A/S武技 D/F法术；法宝页=A~H=法宝槽0..5
		Input *input = Input::get_singleton();
		static const char *SLOT_ACTIONS[8] = {
			"skill_a", "skill_s", "skill_d", "skill_f", "skill_g", "skill_h", "skill_t", "skill_y"
		};
		for (int i = 0; i < 8; i++) {
			if (!input->is_action_just_pressed(SLOT_ACTIONS[i])) continue;
			if (_skill_page == 1 && i < 6) {
				// 法宝页：A~H = 法宝槽 0..5（T/Y 神通/仙法两页通用）
				if (_artifacts) _artifacts->activate_slot(i);
			} else {
				if (_skills) _skills->cast_slot(i);
			}
		}

		// 数字键消耗品栏（1~6 直接磕，绕过背包）
		static const char *BAR_ACTIONS[6] = {
			"consume_1", "consume_2", "consume_3", "consume_4", "consume_5", "consume_6"
		};
		for (int i = 0; i < 6; i++) {
			if (input->is_action_just_pressed(BAR_ACTIONS[i])) {
				use_consumable_bar_slot(i);
			}
		}

		// 技能借用 HitBox 的时间窗关闭（攻击态自身的相位逻辑不干预）
		if (_skill_hitbox_until > 0.0 && _time >= _skill_hitbox_until) {
			_skill_hitbox_until = 0.0;
			if (_hitbox) {
				_hitbox->set_active(false);
				_hitbox->damage_category = DMG_PHYSICAL;
				_hitbox->element = ELEM_NONE;
				if (_skill_hitbox_aoe) { // 旋风斩借用后还原变换
					_skill_hitbox_aoe = false;
					_hitbox->set_position(Vector2(0, 0));
					_hitbox->set_scale(Vector2((float)facing_direction, 1.0f));
				}
			}
		}

		// Q 打坐/收功：地面入坐修炼（修为+灵力回复）；修为封顶后打坐中自动请求机缘突破
		if (Input::get_singleton()->is_action_just_pressed("cultivate")) {
			if (is_meditating()) {
				state_machine->transition_to(PlayerStates::Idle);
			} else if (is_on_floor()) {
				state_machine->transition_to(PlayerStates::Meditate);
			}
		}

		// V 威压 / R 灵压（design/sect-pressure.md §二）
		if (Input::get_singleton()->is_action_just_pressed("pressure_wei")) {
			cast_wei_pressure();
		}
		if (Input::get_singleton()->is_action_just_pressed("pressure_lin")) {
			cast_lin_pressure();
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
		if (input_inverted)
			val = -val; // 赑风袭魂，左右颠倒
		return Math::clamp(val, -1.0f, 1.0f);
	}

	// ---- 二段跳 / 飞行 ----

	bool Player::can_air_jump() const {
		// 炼气解锁二段跳：每离地一次（落地/攀墙刷新）
		return !air_jump_used && _abilities &&
			_abilities->has_ability(StringName(AbilityManager::ABILITY_DOUBLE_JUMP));
	}

	bool Player::can_fly() const {
		if (_flight_blocked)
			return false; // 弱水区禁飞（流沙河）
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
		// X = 普攻+交互合一：附近有交互物时按 X（interact 含 X 键）交互优先，不出刀
		if (_interact_prompt_active &&
		    Input::get_singleton()->is_action_just_pressed("interact")) {
			return false;
		}
		return Input::get_singleton()->is_action_just_pressed("attack");
	}

	void Player::_on_interaction_prompt(const String &p_text, bool p_show) {
		_interact_prompt_active = p_show;
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
		// 无类别入口（投射物/三灾/环境等）：按物理无元素结算
		_take_damage_typed(p_amount, DMG_PHYSICAL, ELEM_NONE, p_source);
	}

	void Player::take_damage_typed(float p_amount, int p_cat, int p_elem, Node *p_source) {
		_take_damage_typed(p_amount, DamageCategory(p_cat), Element(p_elem), p_source);
	}

	void Player::take_hit(const HitBox *p_hitbox, Node *p_source) {
		if (!p_hitbox) return;
		_take_damage_typed(p_hitbox->damage, p_hitbox->damage_category, p_hitbox->element, p_source);
	}

	void Player::_take_damage_typed(float p_amount, DamageCategory p_cat, Element p_elem, Node *p_source) {
		if (is_invulnerable()) return; // 金刚不坏：无敌窗口全免
		last_damage_source = p_source; // 记录来源（勾魂死亡路由判定用）
		// DamageCalculator 统一结算：defense 来自装备 × 境界系数
		float defense = get_equip_bonus_defense();
		if (_cultivation) {
			defense *= _cultivation->get_defense_multiplier();
		}
		if (_gongfa) {
			defense *= _gongfa->get_def_mult(); // 功法（炼体）乘区
		}
		if (_artifacts) {
			defense *= 1.0f + _artifacts->get_passive_def_bonus(); // 辅助型法宝常驻被动
		}
		if (_buffs) {
			defense *= _buffs->get_def_mult(); // buff（金刚丹等）乘区
		}
		if (_skills) {
			defense *= _skills->get_passive_def_mult(); // 被动（铁布衫）乘区
		}
		if (_sect) {
			defense *= _sect->get_def_mult(); // 宗门（蓬莱）乘区
		}
		DamageInfo info;
		info.base_amount = p_amount;
		info.category = p_cat;
		info.element = p_elem;
		DefenseProfile def;
		def.defense = defense;
		def.spell_resist = spell_resist;
		def.self_element = self_element;
		for (int i = 0; i < ELEM_CAPACITY; i++) {
			def.elem_resist[i] = elem_resist[i] + (_buffs ? _buffs->get_elem_resist_bonus(i) : 0.0f)
				+ (_skills ? _skills->get_passive_elem_resist() : 0.0f); // 被动（菩提心法）全元素抗性
		}
		float actual_damage = DamageCalculator::compute(info, def);

		current_health -= actual_damage;

		// 受击收功（打坐被打断）
		if (state_machine && state_machine->is_state(PlayerStates::Meditate)) {
			state_machine->transition_to(PlayerStates::Idle);
		}

		// 炼体行为：承受伤害喂养功法熟练
		if (_gongfa) {
			_gongfa->feed(GongfaSystem::SCHOOL_BODY, actual_damage);
		}

		// Broadcast through global signal bus
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("player_health_changed", current_health, max_health);
			bus->emit_signal("player_damaged", p_amount, p_source);
			bus->emit_signal("damage_dealt", get_global_position(), actual_damage, true);
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

		take_hit(hb, p_source);
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
		// 46×24 @ (12,0)：覆盖自身胶囊（-8..8）+ 身后少许（-11）+ 身前 35px 判定
		hb_rect->set_size(Vector2(46, 24));
		hb_shape->set_shape(hb_rect);
		hb_shape->set_position(Vector2(12, 0));
		_hitbox->add_child(hb_shape);

		// Visual for the hitbox（与判定形状一致：中心对齐）
		Polygon2D *hb_visual = memnew(Polygon2D);
		hb_visual->set_name("HitBoxVisual");
		hb_visual->set_visible(false);
		hb_visual->set_color(Color(0.3, 1.0, 0.3, 0.4));
		PackedVector2Array hb_poly;
		hb_poly.append(Vector2(-23, -12));
		hb_poly.append(Vector2(23, -12));
		hb_poly.append(Vector2(23, 12));
		hb_poly.append(Vector2(-23, 12));
		hb_visual->set_polygon(hb_poly);
		hb_visual->set_position(Vector2(12, 0));
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
		_gongfa = memnew(GongfaSystem);
		_gongfa->connect("gongfa_changed", Callable(this, "_on_gongfa_changed"));
		_skills = memnew(SkillSystem);
		_skills->set_player(this);
		_skills->connect("skills_changed", Callable(this, "_on_skills_changed"));
		_artifacts = memnew(ArtifactSystem);
		_artifacts->set_player(this);
		_buffs = memnew(BuffSystem);
		_sect = memnew(SectSystem);
		_alchemy = memnew(AlchemySystem);
		_alchemy->set_player(this);
		// 凡人起步即会的基础武技（拳脚刀剑是凡人的本事）
		_skills->learn(StringName("po_kong_zhan"));
		_skills->learn(StringName("tu_jin_zhan"));
		_skills->assign(0, StringName("po_kong_zhan")); // A
		_skills->assign(1, StringName("tu_jin_zhan"));  // S
		_abilities->set_cultivation(_cultivation);
		_abilities->connect("ability_unlocked", Callable(this, "_on_ability_unlocked"));
		_cultivation->connect("realm_changed", Callable(this, "_on_cultivation_realm_changed"));
		_abilities->check_realm_unlocks();

		// Store base speed before cultivation multiplier
		base_move_speed = move_speed;
		_update_move_speed();

		// 生命上限随境界 × 功法（初始：凡人 100）
		_refresh_max_health(true);

		// 击杀喂养功法（炼体行为：近战击杀）
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->connect("enemy_killed", Callable(this, "_on_enemy_killed"));
			bus->connect("interaction_prompt", Callable(this, "_on_interaction_prompt"));
		}
	}

	void Player::_refresh_max_health(bool p_refill) {
		float old_max = max_health;
		max_health = float(_cultivation ? _cultivation->get_max_health() : 100.0);
		if (_gongfa) {
			max_health *= _gongfa->get_hp_mult();
		}
		if (_sect) {
			max_health *= _sect->get_hp_mult(); // 宗门（蓬莱）乘区
		}
		if (p_refill) {
			current_health = max_health;
		} else if (max_health > old_max) {
			current_health += max_health - old_max; // 上限涨的差额补给当前血
		}
		current_health = Math::min(current_health, max_health);
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("player_health_changed", current_health, max_health);
		}
	}

	void Player::_on_gongfa_changed() {
		_refresh_max_health(false);
		_update_move_speed();
		_refresh_regen_mults();
	}

	// 灵力/法则回复乘区 = 功法 × 被动（灵台清明/道法自然）；功法或技能变化时刷新
	void Player::_refresh_regen_mults() {
		if (!_cultivation) return;
		float gm = _gongfa ? _gongfa->get_mana_mult() : 1.0f;
		float gr = _gongfa ? _gongfa->get_regen_mult() : 1.0f;
		float pr = _skills ? _skills->get_passive_mana_regen_mult() : 1.0f;
		float pl = _skills ? _skills->get_passive_law_regen_mult() : 1.0f;
		float sm = _sect ? _sect->get_mana_mult() : 1.0f;  // 宗门（昆仑）灵力上限
		float sr = _sect ? _sect->get_regen_mult() : 1.0f; // 宗门（昆仑）回灵
		_cultivation->set_mana_max_mult(gm * sm);
		_cultivation->set_mana_regen_mult(gr * pr * sr);
		_cultivation->set_law_regen_mult(pl);
	}

	void Player::_on_skills_changed() {
		_refresh_regen_mults();
	}

	void Player::_on_enemy_killed(Object *p_enemy, Object *p_killer) {
		// 炼体行为：近战击杀喂养（主系 100%/副系 20%，GongfaSystem 内部处理）
		if (p_killer == this && _gongfa) {
			_gongfa->feed(GongfaSystem::SCHOOL_BODY, 15.0f);
		}
		// 战斗推进法宝温养（本命 + 装备中的次要）
		if (p_killer == this && _artifacts) {
			_artifacts->nurture_equipped(2.0f);
		}
		// 战斗行为回复法则之力
		if (p_killer == this && _cultivation) {
			_cultivation->restore_law_power(10.0);
		}
		// 宗门：贡献 + 魔罗教杀伐修为（base 15/150 的加值部分，Enemy 处已发基础修为）
		if (p_killer == this && _sect && _sect->in_sect()) {
			Enemy *e = Object::cast_to<Enemy>(p_enemy);
			bool boss = e ? e->is_boss : false;
			_sect->on_kill(boss);
			float bonus_mult = _sect->get_kill_xp_mult() - 1.0f;
			if (bonus_mult > 0.0f && _cultivation) {
				float base = boss ? 150.0f : 15.0f;
				_cultivation->accumulate_energy(base * bonus_mult);
			}
		}
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
		_refresh_max_health(true);

		// 引气入体（炼气）：授予入门功法（炼体《莽牛劲》+ 练气《吐纳诀》）
		if (_gongfa && p_old_realm < CultivationSystem::QI_REFINING &&
		    p_new_realm >= CultivationSystem::QI_REFINING) {
			_gongfa->grant(StringName("mang_niu_jin"));
			_gongfa->grant(StringName("tu_na_jue"));
			// 灵根显现：授予入门法术（火弹/冰锥）
			if (_skills) {
				_skills->learn(StringName("huo_dan_shu"));
				_skills->learn(StringName("bing_zhui_shu"));
				if (_skills->get_slot_skill(2) == StringName())
					_skills->assign(2, StringName("huo_dan_shu")); // D
				if (_skills->get_slot_skill(3) == StringName())
					_skills->assign(3, StringName("bing_zhui_shu")); // F
				// 炼气武技精进：旋风斩/升龙击（v1 境界授予，后续改师傅/掉落）
				_skills->learn(StringName("xuan_feng_zhan"));
				_skills->learn(StringName("sheng_long_ji"));
				_skills->learn(StringName("shen_xing")); // 被动：神行百变
			}
		}

		// 筑基：飞剑升级为法宝（飞行道具 → 可祭出作战）
		if (_artifacts && p_old_realm < CultivationSystem::FOUNDATION &&
		    p_new_realm >= CultivationSystem::FOUNDATION) {
			_artifacts->acquire(StringName("fei_jian"));
			_artifacts->equip(1, StringName("fei_jian"));
		}
		// 筑基：辟谷（不再需进食，饱食度锁定满，食物转纯 buff）
		if (p_old_realm < CultivationSystem::FOUNDATION &&
		    p_new_realm >= CultivationSystem::FOUNDATION) {
			if (_fullness != _max_fullness) {
				_fullness = _max_fullness;
				_emit_fullness();
			}
			if (_buffs && _buffs->has("buff_hunger"))
				_buffs->remove("buff_hunger"); // 辟谷自动解除饥饿
			SignalBus *bus = SignalBus::get_singleton();
			if (bus)
				bus->emit_signal("bigu_changed", true);
		}
		// 筑基：雷咒术/土盾术
		if (_skills && p_old_realm < CultivationSystem::FOUNDATION &&
		    p_new_realm >= CultivationSystem::FOUNDATION) {
			_skills->learn(StringName("lei_zhou_shu"));
			_skills->learn(StringName("tu_dun_shu"));
			_skills->learn(StringName("jian_xin")); // 被动：剑心通明
		}
		// 金丹：赐照妖葫
		if (_artifacts && p_old_realm < CultivationSystem::GOLDEN_CORE &&
		    p_new_realm >= CultivationSystem::GOLDEN_CORE) {
			_artifacts->acquire(StringName("zhao_yao_hu"));
			_artifacts->equip(2, StringName("zhao_yao_hu"));
		}
		// 金丹：御剑术（剑扇三连）
		if (_skills && p_old_realm < CultivationSystem::GOLDEN_CORE &&
		    p_new_realm >= CultivationSystem::GOLDEN_CORE) {
			_skills->learn(StringName("yu_jian_shu"));
			_skills->learn(StringName("tie_bu_shan")); // 被动：铁布衫
			_skills->learn(StringName("ling_tai"));    // 被动：灵台清明
		}
		// 元婴：赐玄铁塔（辅助型；次要栏满则持而未装，待换）
		if (_artifacts && p_old_realm < CultivationSystem::NASCENT_SOUL &&
		    p_new_realm >= CultivationSystem::NASCENT_SOUL) {
			_artifacts->acquire(StringName("xuan_tie_ta"));
		}
		// 元婴：被动 风雷双翼
		if (_skills && p_old_realm < CultivationSystem::NASCENT_SOUL &&
		    p_new_realm >= CultivationSystem::NASCENT_SOUL) {
			_skills->learn(StringName("feng_lei_yi"));
		}

		// 化神「初触法则」：授予首个神通（缩地成寸·空间法则），装神通槽 T
		if (_skills && p_old_realm < CultivationSystem::SPIRIT_SEVERING &&
		    p_new_realm >= CultivationSystem::SPIRIT_SEVERING) {
			_skills->learn(StringName("suo_di_cheng_cun"));
			if (_skills->get_slot_skill(6) == StringName())
				_skills->assign(6, StringName("suo_di_cheng_cun")); // T
			// 金刚不坏（金之法则）/ 三昧真火（火之法则）
			_skills->learn(StringName("jin_gang_bu_huai"));
			_skills->learn(StringName("san_mei_zhen_huo"));
			_skills->learn(StringName("dao_fa_zi_ran")); // 被动：道法自然
		}

		// 渡劫成仙：本命法宝觉醒（150% → 200%）+ 首个仙法（天雷引）
		if (p_old_realm < CultivationSystem::TRUE_IMMORTAL &&
		    p_new_realm >= CultivationSystem::TRUE_IMMORTAL) {
			awaken_benming_artifact();
			if (_skills) {
				_skills->learn(StringName("tian_lei_yin"));
				if (_skills->get_slot_skill(7) == StringName())
					_skills->assign(7, StringName("tian_lei_yin")); // Y
			}
		}
	}

	void Player::toggle_skill_page() {
		_skill_page = (_skill_page == 0) ? 1 : 0;
		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			bus->emit_signal("skill_page_changed", _skill_page);
		}
	}

	// ---- 技能施放出口（SkillSystem 调用）----

	void Player::exec_skill_melee(float p_power, DamageCategory p_cat, Element p_elem) {
		if (!_hitbox) return;
		_hitbox->damage = get_effective_attack() * p_power;
		_hitbox->damage_category = p_cat;
		_hitbox->element = p_elem;
		_hitbox->set_knockback_from_facing(facing_direction);
		_hitbox->set_scale(Vector2((float)facing_direction, 1.0f));
		_hitbox->set_active(true);
		_skill_hitbox_until = _time + 0.18;
	}

	void Player::exec_skill_lunge(float p_power, DamageCategory p_cat, Element p_elem) {
		// 突进：向面朝方向冲一小段（不动状态机，直接给速度）
		Vector2 v = get_velocity();
		v.x = (float)facing_direction * 420.0f;
		set_velocity(v);
		exec_skill_melee(p_power, p_cat, p_elem);
		_skill_hitbox_until = _time + 0.25; // 突进挥击窗口略长
	}

	void Player::exec_skill_projectile(float p_power, DamageCategory p_cat, Element p_elem,
	                                   float p_speed, const Color &p_color) {
		// 法术强度：攻击面板 × 功法法强乘区 × 技能倍率
		float spell_mult = _gongfa ? _gongfa->get_spell_mult() : 1.0f;
		Projectile *proj = memnew(Projectile);
		proj->set_position(get_global_position() + Vector2((float)facing_direction * 14.0f, -4.0f));
		proj->direction = Vector2((float)facing_direction, 0.0f);
		proj->speed = p_speed;
		proj->damage = get_effective_attack() * spell_mult * p_power;
		proj->damage_category = p_cat;
		proj->element = p_elem;
		proj->visual_color = p_color;
		proj->set_source(this);
		proj->set_collision_mask_value(3, false); // 不打自己（Player body）
		proj->set_collision_mask_value(4, true);  // 打敌人 body
		proj->set_collision_mask_value(1, true);  // 撞墙消失
		get_parent()->add_child(proj);
	}

	void Player::exec_skill_blink(float p_distance) {
		// 碰撞安全瞬移：test 模式先探路，撞墙则停在墙前
		Vector2 motion((float)facing_direction * p_distance, 0.0f);
		Ref<KinematicCollision2D> col = move_and_collide(motion, true);
		if (col.is_valid()) {
			set_global_position(get_global_position() + col->get_travel() * 0.9f);
		} else {
			set_global_position(get_global_position() + motion);
		}
	}

	void Player::exec_skill_aoe(float p_power, DamageCategory p_cat, Element p_elem) {
		// 旋风斩：HitBox 双向放大（约 ±37px），窗口后还原变换
		if (!_hitbox) return;
		_skill_hitbox_aoe = true;
		_hitbox->set_position(Vector2((float)-facing_direction * 18.0f, 0.0f));
		exec_skill_melee(p_power, p_cat, p_elem);
		_hitbox->set_scale(Vector2((float)facing_direction * 1.6f, 1.6f));
		_skill_hitbox_until = _time + 0.22;
	}

	void Player::exec_skill_rising(float p_power, DamageCategory p_cat, Element p_elem) {
		// 升龙击：向上跃起 + 一挥（对空手段）
		Vector2 v = get_velocity();
		v.y = -380.0f;
		v.x = (float)facing_direction * 80.0f;
		set_velocity(v);
		exec_skill_melee(p_power, p_cat, p_elem);
		_skill_hitbox_until = _time + 0.25;
	}

	void Player::exec_skill_self_buff(const StringName &p_buff_id) {
		if (_buffs) {
			_buffs->apply(p_buff_id); // 同名刷新不叠加
		}
	}

	void Player::exec_skill_proj_fan(float p_power, DamageCategory p_cat, Element p_elem,
	                                 float p_speed, const Color &p_color) {
		// 御剑术：3 发扇形（±15°）
		float base = (facing_direction > 0) ? 0.0f : (float)Math_PI;
		static const float OFF[3] = { -0.26f, 0.0f, 0.26f };
		float spell_mult = _gongfa ? _gongfa->get_spell_mult() : 1.0f;
		for (int i = 0; i < 3; i++) {
			Projectile *proj = memnew(Projectile);
			proj->set_position(get_global_position() + Vector2((float)facing_direction * 14.0f, -4.0f));
			proj->direction = Vector2(Math::cos(base + OFF[i]), Math::sin(base + OFF[i]));
			proj->speed = p_speed;
			proj->damage = get_effective_attack() * spell_mult * p_power;
			proj->damage_category = p_cat;
			proj->element = p_elem;
			proj->visual_color = p_color;
			proj->set_source(this);
			proj->set_collision_mask_value(3, false);
			proj->set_collision_mask_value(4, true);
			proj->set_collision_mask_value(1, true);
			get_parent()->add_child(proj);
		}
	}

	void Player::exec_skill_invuln(float p_seconds) {
		_invuln_until = _time + double(p_seconds);
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
		if (_gongfa) {
			move_speed *= _gongfa->get_speed_mult(); // 功法（练气）乘区
		}
		if (_skills) {
			move_speed *= _skills->get_passive_spd_mult(); // 被动（神行百变）乘区
		}
		if (_chilled) {
			move_speed *= 0.7f; // 极寒减速（玄冰窟 ColdZone）
		}
	}

	// ---- Equipment ----

	float Player::get_effective_attack() const {
		float atk = attack_damage;
		atk += get_equip_bonus_attack();
		if (_cultivation) {
			atk *= _cultivation->get_damage_multiplier();
		}
		if (_gongfa) {
			atk *= _gongfa->get_atk_mult(); // 功法（炼体）乘区
		}
		if (_buffs) {
			atk *= _buffs->get_atk_mult(); // buff（赤焰丹等）乘区
		}
		if (_skills) {
			atk *= _skills->get_passive_atk_mult(); // 被动（剑心通明）乘区
		}
		if (_sect) {
			atk *= _sect->get_atk_mult(); // 宗门（蜀山/魔罗）乘区
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

		// 消耗品自动入快捷栏：已存在→不动；否则找空位或已耗尽槽（首个）
		if (def->type == Item::CONSUMABLE) {
			bool in_bar = false;
			for (int i = 0; i < CONSUMABLE_BAR_SLOTS; i++) {
				if (_consumable_bar[i] == p_item_id) { in_bar = true; break; }
			}
			if (!in_bar) {
				for (int i = 0; i < CONSUMABLE_BAR_SLOTS; i++) {
					if (_consumable_bar[i] == StringName() ||
					    _inventory->get_item_count(_consumable_bar[i]) == 0) {
						_consumable_bar[i] = p_item_id;
						break;
					}
				}
			}
		}

		// Auto-use consumables if conditions are met
		if (def->type == Item::CONSUMABLE) {
			bool should_use = false;

			// Auto-use healing pills when below 70% health
			if ((def->heal_amount > 0.0f || def->heal_pct > 0.0f) && current_health < max_health * 0.7f) {
				should_use = true;
			}
			// Auto-use energy pills when below 50% max energy
			if (def->energy_amount > 0.0f && _cultivation &&
			    _cultivation->get_current_energy() < _cultivation->get_max_energy() / 2) {
				should_use = true;
			}

			if (should_use) {
				use_consumable(p_item_id);
			}
		}
	}

	// ---- 宗门（design/sect-pressure.md）----

	bool Player::join_sect(const StringName &p_sect_id) {
		if (!_sect || !_cultivation) return false;
		if (!_sect->join(p_sect_id, _cultivation->get_realm_index())) return false;
		// 入门专属技：拜入即授
		const SectSystem::Def *def = SectSystem::find_def(p_sect_id);
		if (def && _skills) {
			_skills->learn(StringName(def->skill_id));
		}
		_refresh_max_health(false);
		_refresh_regen_mults();
		return true;
	}

	void Player::leave_sect() {
		if (!_sect) return;
		_sect->leave(); // 贡献清零；已学专属技保留（逐出师门不夺修为）
		_refresh_max_health(false);
		_refresh_regen_mults();
	}

	bool Player::use_consumable(const StringName &p_item_id) {
		if (!_inventory) return false;

		const Item *def = ItemDatabase::get_singleton()->get_item(p_item_id);
		if (!def || def->type != Item::CONSUMABLE) return false;

		// Find the item slot
		int slot = -1;
		for (int i = 0; i < _inventory->get_capacity(); i++) {
			Dictionary sd = _inventory->get_slot(i);
			if (!sd.is_empty() && StringName(sd["id"]) == p_item_id) {
				slot = i;
				break;
			}
		}
		if (slot < 0) return false;

		if (!_inventory->use_item(slot)) return false;

		// Apply effects
		bool health_changed = false;
		if (def->heal_amount > 0.0f) {
			current_health = Math::min(current_health + def->heal_amount, max_health);
			health_changed = true;
		}
		if (def->heal_pct > 0.0f) {
			current_health = Math::min(current_health + max_health * def->heal_pct, max_health);
			health_changed = true;
		}
		if (def->mana_amount > 0.0f && _cultivation) {
			_cultivation->restore_mana(def->mana_amount);
		}
		if (def->energy_amount > 0.0f && _cultivation) {
			_cultivation->accumulate_energy(def->energy_amount);
		}
		// 食物：回饱食度（按境界倍率）；辟谷后不再回（食物转纯 buff）
		if (def->fullness_amount > 0.0f && !is_bigu()) {
			_fullness = Math::min(_fullness + def->fullness_amount * get_food_mult(), _max_fullness);
			if (_buffs && _buffs->has("buff_hunger"))
				_buffs->remove("buff_hunger"); // 进食解除饥饿
		}
		if (def->buff_id != StringName() && _buffs) {
			_buffs->apply(def->buff_id); // 同名刷新不叠加
		}
		if (def->learn_skill != StringName() && _skills) {
			_skills->learn(def->learn_skill); // 秘籍/残卷：使用即悟（已会则 no-op）
		}
		if (def->learn_artifact != StringName() && _artifacts) {
			_artifacts->acquire(def->learn_artifact); // 法宝残篇：使用获得法宝
		}

		SignalBus *bus = SignalBus::get_singleton();
		if (bus) {
			if (health_changed) {
				bus->emit_signal("player_health_changed", current_health, max_health);
			}
			if (def->fullness_amount > 0.0f) {
				bus->emit_signal("fullness_changed", _fullness, _max_fullness);
			}
			bus->emit_signal("item_used", String(p_item_id), 1);
		}
		return true;
	}

	StringName Player::get_consumable_bar_slot(int p_idx) const {
		if (p_idx < 0 || p_idx >= CONSUMABLE_BAR_SLOTS) return StringName();
		return _consumable_bar[p_idx];
	}

	bool Player::use_consumable_bar_slot(int p_idx) {
		if (p_idx < 0 || p_idx >= CONSUMABLE_BAR_SLOTS) return false;
		if (_consumable_bar[p_idx] == StringName()) return false;
		return use_consumable(_consumable_bar[p_idx]);
	}

	// ---- Cultivation ----

	void Player::gain_spiritual_energy(float p_amount) {
		if (_cultivation) {
			_cultivation->accumulate_energy(p_amount);
		}
	}

	bool Player::is_meditating() const {
		return state_machine && state_machine->is_state(PlayerStates::Meditate);
	}

	double Player::get_meditate_rate() const {
		if (!_cultivation) return 0.0;
		// max(5, 当前境界封顶×0.2%)每秒 —— 纯打坐约 8 分钟满一境
		double rate = Math::max(5.0, double(_cultivation->get_max_energy()) * 0.002);
		return rate * get_dongtian_meditate_mult();
	}

	double Player::get_dongtian_meditate_mult() const {
		// 聚灵阵：仅在洞天内生效，倍率随境界增强
		SceneTree *tree = get_tree();
		if (!tree) return 1.0;
		Node *root = tree->get_current_scene();
		if (!root) return 1.0;
		DongtianManager *dt = Object::cast_to<DongtianManager>(root->find_child("DongtianManager", false, false));
		if (!dt || !dt->is_inside()) return 1.0;
		int realm = _cultivation ? _cultivation->get_realm_index() : 0;
		return 2.0 + 0.25 * Math::max(0, realm - CultivationSystem::LIAN_XU);
	}

	String Player::get_state_name() const {
		return state_machine ? String(state_machine->get_current_name()) : String();
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
		// 饱食度（辟谷从境界推导，读档时境界已恢复）
		if (p_data.has("fullness")) {
			set_fullness(float(p_data["fullness"]));
		}

		// 功法
		if (p_data.has("gongfa") && _gongfa) {
			_gongfa->load_from_dict(p_data["gongfa"]);
		}
		if (p_data.has("skills") && _skills) {
			_skills->load_from_dict(p_data["skills"]);
		}
		if (p_data.has("artifacts") && _artifacts) {
			_artifacts->load_from_dict(p_data["artifacts"]);
		}
		if (p_data.has("buffs") && _buffs) {
			_buffs->load_from_dict(p_data["buffs"]);
		}
		if (p_data.has("sect") && _sect) {
			_sect->load_from_dict(p_data["sect"]);
			_refresh_max_health(false);
			_refresh_regen_mults();
		}
		if (p_data.has("consumable_bar")) {
			Array bar = p_data["consumable_bar"];
			for (int i = 0; i < CONSUMABLE_BAR_SLOTS && i < bar.size(); i++) {
				_consumable_bar[i] = StringName(bar[i]);
			}
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

	// ============================================================
	// 威压 / 灵压（design/sect-pressure.md §二）
	// ============================================================

	Vector<Object*> Player::_find_guardians(float p_radius, int p_player_realm) {
		Vector<Object*> guardians;
		TypedArray<Node> enemies = get_tree()->get_nodes_in_group("enemies");
		for (int i = 0; i < enemies.size(); i++) {
			Enemy *e = Object::cast_to<Enemy>(enemies[i]);
			if (!e || e->is_dead()) continue;
			float dist = get_global_position().distance_to(e->get_global_position());
			if (dist > p_radius) continue;
			if (e->realm >= p_player_realm) {
				guardians.push_back(e);
			}
		}
		return guardians;
	}

	bool Player::_is_guarded(Node *p_enemy, const Vector<Object*> &p_guardians) {
		if (p_guardians.is_empty()) return false;
		Vector2 ep = Object::cast_to<Node2D>(p_enemy)->get_global_position();
		for (int i = 0; i < p_guardians.size(); i++) {
			Node2D *g = Object::cast_to<Node2D>(p_guardians[i]);
			if (!g) continue;
			if (ep.distance_to(g->get_global_position()) <= 300.0f) {
				return true;
			}
		}
		return false;
	}

	bool Player::cast_wei_pressure() {
		if (_time < _wei_cd_until) return false;
		if (!_cultivation || !_cultivation->consume_mana(30.0f)) return false;
		_wei_cd_until = _time + 8.0;

		int prealm = _cultivation->get_realm_index();
		int hit = 0;
		Vector<Object*> guardians = _find_guardians(240.0f, prealm);

		TypedArray<Node> enemies = get_tree()->get_nodes_in_group("enemies");
		for (int i = 0; i < enemies.size(); i++) {
			Enemy *e = Object::cast_to<Enemy>(enemies[i]);
			if (!e || e->is_dead()) continue;
			float dist = get_global_position().distance_to(e->get_global_position());
			if (dist > 240.0f) continue;
			// realm < player 方可慑服
			if (e->realm >= prealm) continue;
			// 护佑：高阶敌人在场 → 其身边 300px 低阶全免
			if (_is_guarded(e, guardians)) continue;

			float duration = 2.0f + 0.5f * float(prealm - e->realm);
			if (duration > 5.0f) duration = 5.0f;
			e->suppress(duration);
			hit++;
		}

		// 护佑反弹
		if (!guardians.is_empty()) {
			float rebound = max_health * 0.05f;
			current_health -= rebound;
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("interaction_prompt", LOC("对方有高人坐镇，威压反噬！"), true);
				bus->emit_signal("player_health_changed", current_health, max_health);
			}
			if (current_health <= 0.0f) {
				current_health = 0.0f;
				emit_signal("player_died");
				if (bus) bus->emit_signal("player_died");
			}
		} else if (hit > 0) {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("interaction_prompt",
					vformat(LOC("威压：慑服 %d 名敌人"), hit), true);
			}
		} else {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("interaction_prompt", LOC("威压：范围内无低阶敌人"), true);
			}
		}

		return hit > 0;
	}

	bool Player::cast_lin_pressure() {
		if (_time < _lin_cd_until) return false;
		// 平衡：60→45 蓝，与同代技能性价比相称（单目标法伤 3~3.5×atk，15s 大技能）
		if (!_cultivation || !_cultivation->consume_mana(45.0f)) return false;
		_lin_cd_until = _time + 15.0;

		int prealm = _cultivation->get_realm_index();
		int hit = 0;
		int zhen_sha = 0;
		Vector<Object*> guardians = _find_guardians(200.0f, prealm);

		TypedArray<Node> enemies = get_tree()->get_nodes_in_group("enemies");
		for (int i = 0; i < enemies.size(); i++) {
			Enemy *e = Object::cast_to<Enemy>(enemies[i]);
			if (!e || e->is_dead()) continue;
			float dist = get_global_position().distance_to(e->get_global_position());
			if (dist > 200.0f) continue;
			// realm ≤ player-2 方可生效（境界接近则灵压不侵）
			int gap = prealm - e->realm;
			if (gap < 2) continue;
			// 护佑
			if (_is_guarded(e, guardians)) continue;

			if (gap >= 4) {
				// 镇杀：大境界碾压（元婴镇凡人/炼气）
				e->take_damage_typed(99999.0f, int(DMG_SPELL), int(ELEM_NONE), this);
				zhen_sha++;
			} else {
				// 法术伤害 = 攻击力 × (2 + 0.5×差)，走法抗结算
				float atk = get_effective_attack();
				float dmg = atk * (2.0f + 0.5f * float(gap));
				e->take_damage_typed(dmg, int(DMG_SPELL), int(ELEM_NONE), this);
			}
			hit++;
		}

		// 护佑反弹
		if (!guardians.is_empty()) {
			float rebound = max_health * 0.08f;
			current_health -= rebound;
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("interaction_prompt", LOC("对方有高人坐镇，灵压反噬！"), true);
				bus->emit_signal("player_health_changed", current_health, max_health);
			}
			if (current_health <= 0.0f) {
				current_health = 0.0f;
				emit_signal("player_died");
				if (bus) bus->emit_signal("player_died");
			}
		} else if (hit > 0) {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				if (zhen_sha > 0) {
					bus->emit_signal("interaction_prompt",
						vformat(LOC("灵压：%d 伤 · %d 镇杀"), hit - zhen_sha, zhen_sha), true);
				} else {
					bus->emit_signal("interaction_prompt",
						vformat(LOC("灵压：%d 名敌人受创"), hit), true);
				}
			}
		} else {
			SignalBus *bus = SignalBus::get_singleton();
			if (bus) {
				bus->emit_signal("interaction_prompt", LOC("灵压：无有效目标（境界差≥2 方可生效）"), true);
			}
		}

		return hit > 0;
	}

} // namespace godot
