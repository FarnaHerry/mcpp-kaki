#include "enemy.h"

#include "../combat/hitbox.h"
#include "../combat/hurtbox.h"
#include "../utils/signal_bus.h"

#include <cstdlib>

#include <godot_cpp/classes/collision_shape2d.hpp>
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
    } // namespace EnemyStates

    // ============================================================
    // Enemy states
    // ============================================================

    class EnemyIdleState : public State<Enemy> {
        double _idle_timer = 0.0;
        double _idle_duration = 1.5;

    public:
        void enter(Enemy *e) override {
            _idle_timer = 0.0;
            _idle_duration = 1.0 + ((float(std::rand()) / float(RAND_MAX)) * 1.0); // 1-2 seconds
        }
        void exit(Enemy *e) override {}

        void physics_update(Enemy *e, double delta) override {
            _idle_timer += delta;

            if (e->can_see_player()) {
                e->state_machine->transition_to(EnemyStates::Chase);
                return;
            }

            if (_idle_timer >= _idle_duration) {
                e->state_machine->transition_to(EnemyStates::Patrol);
                return;
            }

            // Apply gravity and stop
            Vector2 vel = e->get_velocity();
            vel.x = 0.0f;
            vel.y += e->get_gravity() * delta;
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
                e->state_machine->transition_to(EnemyStates::Chase);
                return;
            }

            // Change direction periodically or on wall hit
            if (_patrol_timer > 2.5 || e->is_on_wall()) {
                _patrol_dir *= -1;
                e->facing_direction = _patrol_dir;
                _patrol_timer = 0.0;
            }

            Vector2 vel = e->get_velocity();
            vel.x = _patrol_dir * e->move_speed * 0.4f;
            vel.y += e->get_gravity() * delta;
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

            if (e->player_in_attack_range() && e->can_attack()) {
                e->state_machine->transition_to(EnemyStates::Attack);
                return;
            }

            e->update_facing_to_player();

            Node2D *target = e->get_player_target();
            float dir = (target->get_global_position().x > e->get_global_position().x) ? 1.0f : -1.0f;

            Vector2 vel = e->get_velocity();
            vel.x = dir * e->move_speed;
            vel.y += e->get_gravity() * delta;
            e->set_velocity(vel);
            e->move_and_slide();
        }
    };

    class EnemyAttackState : public State<Enemy> {
    public:
        void enter(Enemy *e) override {
            // Activate hitbox and flip to facing direction
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

            // Transition immediately — attack is instant hitbox activation
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
            vel.y = -100.0f;
            e->set_velocity(vel);
        }
        void exit(Enemy *e) override {}

        void physics_update(Enemy *e, double delta) override {
            _hurt_timer += delta;

            if (e->is_dead()) {
                e->state_machine->transition_to(EnemyStates::Death);
                return;
            }

            if (_hurt_timer > 0.3f && e->is_on_floor()) {
                if (e->can_see_player()) {
                    e->state_machine->transition_to(EnemyStates::Chase);
                } else {
                    e->state_machine->transition_to(EnemyStates::Idle);
                }
                return;
            }

            Vector2 vel = e->get_velocity();
            vel.y += e->get_gravity() * delta;
            e->set_velocity(vel);
            e->move_and_slide();
        }
    };

    class EnemyDeathState : public State<Enemy> {
    public:
        void enter(Enemy *e) override {
            // Disable collision, queue removal
            e->set_process(false);
            e->set_physics_process(false);
            // Fade out / play death anim — for now just queue_free
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
        ClassDB::bind_method(D_METHOD("_on_hurtbox_hit", "hitbox", "source"), &Enemy::_on_hurtbox_hit);

        ADD_SIGNAL(MethodInfo("enemy_died"));
    }

    void Enemy::_ready() {
        if (Engine::get_singleton()->is_editor_hint())
            return;

        _setup_collision();
        _create_hitboxes();
        _find_player();

        current_health = max_health;

        state_machine = new StateMachine<Enemy>(this);
        state_machine->add_state(EnemyStates::Idle, new EnemyIdleState());
        state_machine->add_state(EnemyStates::Patrol, new EnemyPatrolState());
        state_machine->add_state(EnemyStates::Chase, new EnemyChaseState());
        state_machine->add_state(EnemyStates::Attack, new EnemyAttackState());
        state_machine->add_state(EnemyStates::Hurt, new EnemyHurtState());
        state_machine->add_state(EnemyStates::Death, new EnemyDeathState());

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
    }

    void Enemy::_setup_collision() {
        // Enemy body is layer 4, collides with Ground and One Way Platform
        set_collision_layer_value(4, true);  // layer 4 = Enemy
        set_collision_mask_value(1, true);   // Ground
        set_collision_mask_value(2, true);   // One Way Platform
    }

    void Enemy::_create_hitboxes() {
        // HitBox for dealing damage
        _hitbox = memnew(HitBox);
        _hitbox->set_name("HitBox");
        _hitbox->damage = attack_damage;
        _hitbox->set_active(false);
        // HitBox IS layer 6, DETECTS layer 3 (Player) for direct damage
        _hitbox->set_collision_layer_value(6, true);
        _hitbox->set_collision_mask_value(3, true);
        _hitbox->connect("area_entered", Callable(_hitbox, "_on_area_entered"));

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
        hb_visual->set_color(Color(1.0, 0.3, 0.3, 0.4));
        PackedVector2Array hb_poly;
        hb_poly.append(Vector2(0, -10));
        hb_poly.append(Vector2(30, -10));
        hb_poly.append(Vector2(30, 10));
        hb_poly.append(Vector2(0, 10));
        hb_visual->set_polygon(hb_poly);
        hb_visual->set_position(Vector2(15, 0));
        _hitbox->add_child(hb_visual);

        add_child(_hitbox);

        // HurtBox for receiving damage
        _hurtbox = memnew(HurtBox);
        _hurtbox->set_name("HurtBox");
        // HurtBox: layer 4 (Enemy), monitors Player HitBox on layer 5
        _hurtbox->set_collision_layer_value(4, true); // layer 4 = Enemy
        _hurtbox->set_collision_mask_value(5, true);  // layer 5 = Player HitBox

        CollisionShape2D *hu_shape = memnew(CollisionShape2D);
        Ref<RectangleShape2D> hu_rect;
        hu_rect.instantiate();
        hu_rect->set_size(Vector2(32, 32));
        hu_shape->set_shape(hu_rect);
        _hurtbox->add_child(hu_shape);
        add_child(_hurtbox);

        // Connect hurt signal
        _hurtbox->connect("hurtbox_hit", Callable(this, "_on_hurtbox_hit"));
    }

    void Enemy::_find_player() {
        // Walk up to the scene root and find the Player node
        Node *root = get_tree()->get_current_scene();
        if (root) {
            _player_target = Object::cast_to<Node2D>(root->get_node_or_null("Player"));
        }
    }

    void Enemy::take_damage(float p_amount, Node *p_source) {
        current_health -= p_amount;

        if (current_health <= 0.0f) {
            current_health = 0.0f;

            // Give spiritual energy to the player who killed this enemy
            if (p_source) {
                p_source->call("gain_spiritual_energy", 15.0f);
            }

            // Broadcast kill through signal bus
            SignalBus *bus = SignalBus::get_singleton();
            if (bus) {
                bus->emit_signal("enemy_killed", this, p_source);
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

        float dmg = hb->damage;
        take_damage(dmg, p_source);
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
        if (!_player_target)
            return false;
        float dist = get_global_position().distance_to(_player_target->get_global_position());
        return dist <= detection_radius;
    }

    bool Enemy::player_in_attack_range() const {
        if (!_player_target)
            return false;
        float dist = get_global_position().distance_to(_player_target->get_global_position());
        return dist <= attack_range;
    }

    bool Enemy::can_attack() const {
        return (_time - last_attack_time) >= attack_cooldown;
    }

    void Enemy::update_facing_to_player() {
        if (!_player_target)
            return;
        facing_direction = (_player_target->get_global_position().x > get_global_position().x) ? 1 : -1;
    }

} // namespace godot
