#ifndef CPP_KAKI_PLAYER_H
#define CPP_KAKI_PLAYER_H

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/classes/input.hpp>

#include "../combat/combo_chain.h"
#include "../utils/state_machine.h"
#include "../utils/input_buffer.h"

namespace godot {

    class HitBox;
    class HurtBox;
    class CultivationSystem;
    class AbilityManager;

    class Player : public CharacterBody2D {
        GDCLASS(Player, CharacterBody2D);

    public:
        // Movement
        float move_speed = 180.0f;
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
        float attack_damage = 1.0f;

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

        void take_damage(float p_amount, Node *p_source);
        bool is_dead() const { return current_health <= 0.0f; }

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
        void gain_spiritual_energy(float p_amount);

        // Called when HitBox lands a hit (for combo tracking)
        void on_attack_landed(Node *p_victim);

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

        void _update_buffers();
        void _update_facing();
        void _create_hitboxes();
        void _setup_collision();
        void _create_cultivation();
    };

} // namespace godot

#endif // CPP_KAKI_PLAYER_H
