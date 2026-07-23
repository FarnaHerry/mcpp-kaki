#ifndef CPP_KAKI_COMBO_CHAIN_H
#define CPP_KAKI_COMBO_CHAIN_H

#include <godot_cpp/variant/string.hpp>

namespace godot {

// Manages a multi-hit combo chain.
// Tracks hit count, timing windows, and damage/knockback multipliers.
//
// Usage (per-attack):
//   combo.start_or_chain(time) -> returns current hit index (0, 1, 2)
//   combo.on_hit_landed(time)  -> increments hit count
//   combo.update(time)          -> call each frame; auto-resets on timeout
//
struct ComboHit {
    float damage_mult = 1.0f;
    float knockback_mult = 1.0f;
    float startup_time = 0.05f;   // seconds before hitbox activates
    float active_time = 0.1f;     // seconds hitbox stays active
    float recovery_time = 0.15f;  // seconds after active before next action
    int hit_count_required = 0;   // how many landed hits needed to unlock this step
};

class ComboChain {
public:
    static constexpr int MAX_COMBO = 3;
    static constexpr float CHAIN_WINDOW = 0.5f;   // time after previous attack to chain
    static constexpr float COMBO_TIMEOUT = 1.2f;   // time since last hit to reset combo

    ComboChain();

    // Call when the player presses attack. Returns the combo step to execute (0, 1, 2).
    // Advances internal state for the next attack.
    int start_attack(double p_time);

    // Call when a hit actually lands on an enemy. Advances the hit counter.
    void on_hit_landed(double p_time);

    // Call each frame — auto-resets combo if timeout exceeded.
    void update(double p_time);

    // Current state
    int get_hit_count() const { return _hit_count; }
    int get_combo_step() const { return _combo_step; }
    bool is_in_combo() const { return _hit_count >= 2; }

    // Get multipliers for current combo step
    float get_damage_multiplier() const;
    float get_knockback_multiplier() const;

    // Timing info for the current combo step
    float get_startup_time() const;
    float get_active_time() const;
    float get_recovery_time() const;

    // Manual reset
    void reset();

    // Hit definitions
    static const ComboHit &get_hit_definition(int p_step);

private:
    int _hit_count = 0;         // number of hits landed in this combo
    int _combo_step = 0;        // which step of the attack chain we're on (0-2)
    double _last_attack_time = 0.0;
    double _last_hit_time = 0.0;

    ComboHit _hits[MAX_COMBO];
    void _init_hits();
};

} // namespace godot

#endif // CPP_KAKI_COMBO_CHAIN_H
