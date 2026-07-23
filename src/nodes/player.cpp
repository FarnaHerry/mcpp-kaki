#include "player.h"

#include "../combat/combo_chain.h"
#include "../combat/hitbox.h"
#include "../combat/hurtbox.h"
#include "../cultivation/ability_manager.h"
#include "../cultivation/cultivation_system.h"
#include "../utils/signal_bus.h"

#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/polygon2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/core/class_db.hpp>

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
    } // namespace PlayerStates

    // ------- Idle -------
    class PlayerIdleState : public State<Player> {
    public:
        void enter(Player *p) override {}
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

            // Dash in air
            if (p->dash_just_pressed() && p->can_dash()) {
                p->state_machine->transition_to(PlayerStates::Dash);
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
                // Jump away from wall
                int wall_normal_x = p->get_last_slide_collision()->get_normal().x > 0 ? 1 : -1;
                vel.x = -wall_normal_x * p->wall_jump_horizontal;
                p->set_velocity(vel);
                p->facing_direction = -wall_normal_x;
                p->left_ground_time = p->get_time(); // allow coyote from wall jump
                p->state_machine->transition_to(PlayerStates::Fall);
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

            // Apply minimal movement during attack (allow gravity in air)
            Vector2 vel = p->get_velocity();
            if (!p->is_on_floor()) {
                vel.y += p->get_gravity() * delta;
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
        ClassDB::bind_method(D_METHOD("on_attack_landed", "victim"), &Player::on_attack_landed);
        ClassDB::bind_method(D_METHOD("_on_hurtbox_hit", "hitbox", "source"), &Player::_on_hurtbox_hit);

        ADD_SIGNAL(MethodInfo("player_died"));
        ADD_SIGNAL(MethodInfo("player_damaged", PropertyInfo(Variant::FLOAT, "amount")));
    }

    void Player::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        current_health = max_health;
        _setup_collision();
        _create_hitboxes();
        _create_cultivation();

        state_machine = new StateMachine<Player>(this);

        state_machine->add_state(PlayerStates::Idle, new PlayerIdleState());
        state_machine->add_state(PlayerStates::Run, new PlayerRunState());
        state_machine->add_state(PlayerStates::Jump, new PlayerJumpState());
        state_machine->add_state(PlayerStates::Fall, new PlayerFallState());
        state_machine->add_state(PlayerStates::WallCling, new PlayerWallClingState());
        state_machine->add_state(PlayerStates::Dash, new PlayerDashState());
        state_machine->add_state(PlayerStates::Attack, new PlayerAttackState());

        state_machine->set_initial_state(PlayerStates::Idle);
    }

    void Player::_physics_process(double p_delta) {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        state_machine->physics_update(p_delta);
    }

    void Player::_process(double p_delta) {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        _time += p_delta;
        _update_buffers();
        _update_facing();
        combo_chain.update(_time);
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
        current_health -= p_amount;

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
        _abilities->check_realm_unlocks();

        // Apply realm speed bonus
        move_speed *= _cultivation->get_speed_multiplier();
    }

    void Player::gain_spiritual_energy(float p_amount) {
        if (_cultivation) {
            _cultivation->accumulate_energy(p_amount);
        }
    }

    void Player::on_attack_landed(Node *p_victim) {
        combo_chain.on_hit_landed(_time);

        SignalBus *bus = SignalBus::get_singleton();
        if (bus) {
            bus->emit_signal("combo_changed", combo_chain.get_hit_count());
        }
    }

} // namespace godot
