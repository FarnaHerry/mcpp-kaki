#include "register_types.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "combat/hitbox.h"
#include "combat/hurtbox.h"
#include "cultivation/ability_manager.h"
#include "cultivation/cultivation_system.h"
#include "nodes/camera_room_2d.h"
#include "nodes/enemy.h"
#include "nodes/player.h"
#include "nodes/portal.h"

using namespace godot;

void initialize_cpp_kaki_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    GDREGISTER_CLASS(Player);
    GDREGISTER_CLASS(Enemy);
    GDREGISTER_CLASS(CameraRoom2D);
    GDREGISTER_CLASS(Portal);
    GDREGISTER_CLASS(HitBox);
    GDREGISTER_CLASS(HurtBox);
    GDREGISTER_CLASS(CultivationSystem);
    GDREGISTER_CLASS(AbilityManager);
}

void uninitialize_cpp_kaki_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT cpp_kaki_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {

        GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_cpp_kaki_module);
        init_obj.register_terminator(uninitialize_cpp_kaki_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
