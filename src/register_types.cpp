#include "register_types.h"
#include "nodes/player.h"
#include "nodes/enemy.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

import mcpp_kaki.combat;
#include "core/game_manager.h"
import mcpp_kaki.core;
#include "core/drop_system.h"
import mcpp_kaki.core;
#include "core/soul_ledger_system.h"
#include "core/shop_system.h"
#include "nodes/camera_room_2d.h"
#include "nodes/dongtian_manager.h"
#include "nodes/farm_plot.h"
#include "nodes/storage_chest.h"
#include "nodes/underworld_interact.h"
#include "nodes/shop_keeper.h"
#include "nodes/item_pickup.h"
#include "nodes/herb_node.h"
#include "nodes/portal.h"
#include "nodes/telemetry_panel.h"
import mcpp_kaki.nodes;
import mcpp_kaki.cultivation;
import mcpp_kaki.inventory;
import mcpp_kaki.utils;

using namespace godot;

void initialize_mcpp_kaki_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(Player);
	GDREGISTER_CLASS(Enemy);
	GDREGISTER_CLASS(CameraRoom2D);
	GDREGISTER_CLASS(Portal);
	GDREGISTER_CLASS(DongtianManager);
	GDREGISTER_CLASS(FarmPlot);
	GDREGISTER_CLASS(StorageChest);
	GDREGISTER_CLASS(UnderworldInteractNode);
	GDREGISTER_CLASS(ShopKeeper);
	GDREGISTER_CLASS(HitBox);
	GDREGISTER_CLASS(HurtBox);
	GDREGISTER_CLASS(Projectile);
	GDREGISTER_CLASS(CultivationSystem);
	GDREGISTER_CLASS(GongfaSystem);
	GDREGISTER_CLASS(SkillSystem);
	GDREGISTER_CLASS(ArtifactSystem);
	GDREGISTER_CLASS(BuffSystem);
	GDREGISTER_CLASS(SectSystem);
	GDREGISTER_CLASS(AlchemySystem);
	GDREGISTER_CLASS(AbilityManager);
	GDREGISTER_CLASS(Localization);
	GDREGISTER_CLASS(SignalBus);
	GDREGISTER_CLASS(GameManager);
	GDREGISTER_CLASS(SoulLedgerSystem);
	GDREGISTER_CLASS(ShopSystem);
	GDREGISTER_CLASS(ContinentManager);
	GDREGISTER_CLASS(DataLoader);
	GDREGISTER_CLASS(GameHUD);
	GDREGISTER_CLASS(TelemetryPanel);
	GDREGISTER_CLASS(InventoryPanel);
	GDREGISTER_CLASS(StoragePanel);
	GDREGISTER_CLASS(ShopPanel);
	GDREGISTER_CLASS(GridList);
	GDREGISTER_CLASS(ItemDatabase);
	GDREGISTER_CLASS(ItemPickup);
	GDREGISTER_CLASS(HerbNode);
	GDREGISTER_CLASS(SaveSystem);
	GDREGISTER_CLASS(Inventory);
	GDREGISTER_CLASS(DropSystem);
	GDREGISTER_CLASS(BreakthroughManager);
	GDREGISTER_CLASS(TribulationController);
	GDREGISTER_CLASS(DamageNumbers);
	GDREGISTER_CLASS(GameMenu);
}

void uninitialize_mcpp_kaki_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
	GDExtensionBool GDE_EXPORT mcpp_kaki_library_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {

		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_mcpp_kaki_module);
		init_obj.register_terminator(uninitialize_mcpp_kaki_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
