#include "register_types.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

#include "combat/hitbox.h"
#include "combat/hurtbox.h"
#include "combat/projectile.h"
#include "combat/skill_system.h"
#include "core/game_manager.h"
#include "core/drop_system.h"
#include "core/save_system.h"
#include "cultivation/ability_manager.h"
#include "cultivation/artifact_system.h"
#include "cultivation/breakthrough_manager.h"
#include "cultivation/cultivation_system.h"
#include "cultivation/gongfa_system.h"
#include "cultivation/tribulation_controller.h"
#include "inventory/inventory.h"
#include "inventory/item_database.h"
#include "nodes/camera_room_2d.h"
#include "nodes/damage_numbers.h"
#include "nodes/enemy.h"
#include "nodes/game_hud.h"
#include "nodes/inventory_panel.h"
#include "nodes/item_pickup.h"
#include "nodes/player.h"
#include "nodes/portal.h"
#include "nodes/game_menu.h"
#include "nodes/telemetry_panel.h"
#include "utils/signal_bus.h"

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
	GDREGISTER_CLASS(Projectile);
	GDREGISTER_CLASS(CultivationSystem);
	GDREGISTER_CLASS(GongfaSystem);
	GDREGISTER_CLASS(SkillSystem);
	GDREGISTER_CLASS(ArtifactSystem);
	GDREGISTER_CLASS(AbilityManager);
	GDREGISTER_CLASS(SignalBus);
	GDREGISTER_CLASS(GameManager);
	GDREGISTER_CLASS(GameHUD);
	GDREGISTER_CLASS(TelemetryPanel);
	GDREGISTER_CLASS(InventoryPanel);
	GDREGISTER_CLASS(ItemDatabase);
	GDREGISTER_CLASS(ItemPickup);
	GDREGISTER_CLASS(SaveSystem);
	GDREGISTER_CLASS(Inventory);
	GDREGISTER_CLASS(DropSystem);
	GDREGISTER_CLASS(BreakthroughManager);
	GDREGISTER_CLASS(TribulationController);
	GDREGISTER_CLASS(DamageNumbers);
	GDREGISTER_CLASS(GameMenu);
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
