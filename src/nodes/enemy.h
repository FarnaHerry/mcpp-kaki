#ifndef CPP_KAKI_ENEMY_H
#define CPP_KAKI_ENEMY_H

#include <godot_cpp/classes/character_body2d.hpp>

#include "../utils/state_machine.h"

namespace godot {

    class HitBox;
    class HurtBox;
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

        // Facing (-1 or 1)
        int facing_direction = -1;

        // References
        StateMachine<Enemy> *state_machine = nullptr;
        Node2D *_player_target = nullptr;

        // Attack cooldown timer
        double last_attack_time = -999.0;

        // Accessors for states
        float get_gravity() const;
        Node2D *get_player_target() const;
        bool can_see_player() const;
        bool player_in_attack_range() const;
        bool can_attack() const;
        void update_facing_to_player();

        void take_damage(float p_amount, Node *p_source);
        bool is_dead() const { return current_health <= 0.0f; }

        double get_time() const { return _time; }

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

        void _setup_collision();
        void _find_player();
        void _create_hitboxes();
    };

} // namespace godot

#endif // CPP_KAKI_ENEMY_H
