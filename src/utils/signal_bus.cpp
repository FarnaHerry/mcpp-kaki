#include "signal_bus.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

SignalBus *SignalBus::_singleton = nullptr;

void SignalBus::_ready() {
    _singleton = this;
}

void SignalBus::_bind_methods() {
    // ---- Player signals ----
    ADD_SIGNAL(MethodInfo("player_health_changed",
                          PropertyInfo(Variant::FLOAT, "current"),
                          PropertyInfo(Variant::FLOAT, "max")));
    ADD_SIGNAL(MethodInfo("player_damaged",
                          PropertyInfo(Variant::FLOAT, "amount"),
                          PropertyInfo(Variant::OBJECT, "source")));
    ADD_SIGNAL(MethodInfo("player_died"));
    ADD_SIGNAL(MethodInfo("player_respawned"));

    // ---- Combat signals ----
    ADD_SIGNAL(MethodInfo("enemy_killed",
                          PropertyInfo(Variant::OBJECT, "enemy"),
                          PropertyInfo(Variant::OBJECT, "killer")));
    ADD_SIGNAL(MethodInfo("combo_changed",
                          PropertyInfo(Variant::INT, "count")));
    ADD_SIGNAL(MethodInfo("combo_ended",
                          PropertyInfo(Variant::INT, "final_count")));

    // ---- Cultivation signals ----
    ADD_SIGNAL(MethodInfo("spiritual_energy_changed",
                          PropertyInfo(Variant::FLOAT, "current"),
                          PropertyInfo(Variant::FLOAT, "max")));
    ADD_SIGNAL(MethodInfo("realm_changed",
                          PropertyInfo(Variant::INT, "old_realm"),
                          PropertyInfo(Variant::INT, "new_realm"),
                          PropertyInfo(Variant::STRING, "realm_name")));

    // ---- Game state signals ----
    ADD_SIGNAL(MethodInfo("game_paused"));
    ADD_SIGNAL(MethodInfo("game_resumed"));
    ADD_SIGNAL(MethodInfo("checkpoint_set",
                          PropertyInfo(Variant::VECTOR2, "position"),
                          PropertyInfo(Variant::STRING, "scene_path")));
    ADD_SIGNAL(MethodInfo("scene_transition_start",
                          PropertyInfo(Variant::STRING, "from_scene"),
                          PropertyInfo(Variant::STRING, "to_scene")));
    ADD_SIGNAL(MethodInfo("scene_transition_end",
                          PropertyInfo(Variant::STRING, "scene_path")));

    // ---- Interaction signals ----
    ADD_SIGNAL(MethodInfo("interaction_prompt",
                          PropertyInfo(Variant::STRING, "text"),
                          PropertyInfo(Variant::BOOL, "show")));
    ADD_SIGNAL(MethodInfo("item_picked_up",
                          PropertyInfo(Variant::STRING, "item_id"),
                          PropertyInfo(Variant::INT, "quantity")));
}

} // namespace godot
