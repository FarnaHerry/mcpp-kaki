#include "combo_chain.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

ComboChain::ComboChain() {
    _init_hits();
}

void ComboChain::_init_hits() {
    // Hit 0: Quick jab
    _hits[0] = { 1.0f, 1.0f, 0.04f, 0.08f, 0.12f, 0 };
    // Hit 1: Strong follow-up
    _hits[1] = { 1.4f, 1.3f, 0.06f, 0.10f, 0.16f, 1 };
    // Hit 2: Finisher — heavy damage, strong knockback
    _hits[2] = { 2.0f, 2.0f, 0.08f, 0.12f, 0.22f, 2 };
}

int ComboChain::start_attack(double p_time) {
    // Check if we're within chain window
    double since_last = p_time - _last_attack_time;

    if (since_last > CHAIN_WINDOW || _combo_step >= MAX_COMBO - 1) {
        // Start fresh combo
        _combo_step = 0;
        _hit_count = 0;
    } else {
        // Chain to next step
        _combo_step++;
    }

    _last_attack_time = p_time;
    return _combo_step;
}

void ComboChain::on_hit_landed(double p_time) {
    _hit_count++;
    _last_hit_time = p_time;
}

void ComboChain::update(double p_time) {
    double since_hit = p_time - _last_hit_time;
    if (since_hit > COMBO_TIMEOUT && _hit_count > 0) {
        reset();
    }
}

float ComboChain::get_damage_multiplier() const {
    if (_combo_step < 0 || _combo_step >= MAX_COMBO) return 1.0f;
    return _hits[_combo_step].damage_mult;
}

float ComboChain::get_knockback_multiplier() const {
    if (_combo_step < 0 || _combo_step >= MAX_COMBO) return 1.0f;
    return _hits[_combo_step].knockback_mult;
}

float ComboChain::get_startup_time() const {
    if (_combo_step < 0 || _combo_step >= MAX_COMBO) return 0.05f;
    return _hits[_combo_step].startup_time;
}

float ComboChain::get_active_time() const {
    if (_combo_step < 0 || _combo_step >= MAX_COMBO) return 0.1f;
    return _hits[_combo_step].active_time;
}

float ComboChain::get_recovery_time() const {
    if (_combo_step < 0 || _combo_step >= MAX_COMBO) return 0.15f;
    return _hits[_combo_step].recovery_time;
}

void ComboChain::reset() {
    _hit_count = 0;
    _combo_step = 0;
    _last_attack_time = 0.0;
    _last_hit_time = 0.0;
}

const ComboHit &ComboChain::get_hit_definition(int p_step) {
    static ComboChain dummy;
    if (p_step < 0 || p_step >= MAX_COMBO) return dummy._hits[0];
    return dummy._hits[p_step];
}

} // namespace godot
