# Open world Metroidvania — Portal-based room entry (composition pattern).
extends Node2D

var _interact_hint = null

func _ready():
	call_deferred("_setup_game")

func _make_sprite(parent, color, size, pos=Vector2.ZERO):
	var poly = Polygon2D.new()
	poly.name = "Polygon2D"
	poly.color = color
	var half = size * 0.5
	poly.polygon = PackedVector2Array([Vector2(-half.x, -half.y), Vector2(half.x, -half.y), Vector2(half.x, half.y), Vector2(-half.x, half.y)])
	poly.position = pos
	parent.add_child(poly)

func _setup_game():
	# ---- SignalBus (global signal hub) ----
	var signal_bus = ClassDB.instantiate("SignalBus")
	signal_bus.name = "SignalBus"
	add_child(signal_bus)

	# ---- ItemDatabase (must exist before any pickup is created) ----
	var item_db = ClassDB.instantiate("ItemDatabase")
	item_db.name = "ItemDatabase"
	add_child(item_db)

	# ---- GameManager (game state controller) ----
	var game_mgr = ClassDB.instantiate("GameManager")
	game_mgr.name = "GameManager"
	add_child(game_mgr)

	# ---- DropSystem (enemy loot drops) ----
	var drop_system = ClassDB.instantiate("DropSystem")
	drop_system.name = "DropSystem"
	add_child(drop_system)

	# ---- BreakthroughManager (机缘突破事件唯一入口) ----
	var breakthrough_mgr = ClassDB.instantiate("BreakthroughManager")
	breakthrough_mgr.name = "BreakthroughManager"
	add_child(breakthrough_mgr)

	# ---- DamageNumbers (伤害数字显示) ----
	var dmg_numbers = ClassDB.instantiate("DamageNumbers")
	dmg_numbers.name = "DamageNumbers"
	add_child(dmg_numbers)

	# ---- HUD (must be created after SignalBus so it can connect signals) ----
	var hud = ClassDB.instantiate("GameHUD")
	hud.name = "GameHUD"
	add_child(hud)

	# ---- Telemetry Panel (debug readout, F3) ----
	var telemetry = ClassDB.instantiate("TelemetryPanel")
	telemetry.name = "TelemetryPanel"
	add_child(telemetry)

	# ---- Inventory Panel (I-key overlay) ----
	var inv_panel = ClassDB.instantiate("InventoryPanel")
	inv_panel.name = "InventoryPanel"
	add_child(inv_panel)

	# ---- Game Menu (ESC 多页管理菜单: 背包/能力/功法/技能/法宝/设置) ----
	var game_menu = ClassDB.instantiate("GameMenu")
	game_menu.name = "GameMenu"
	add_child(game_menu)

	# ---- Camera ----
	var camera = ClassDB.instantiate("CameraRoom2D")
	camera.name = "CameraRoom2D"
	add_child(camera)
	game_mgr.call("set_camera", camera)

	# ---- Player ----
	var player = ClassDB.instantiate("Player")
	player.name = "Player"
	player.position = Vector2(200, 200)

	var shape = CollisionShape2D.new()
	var capsule = CapsuleShape2D.new()
	capsule.radius = 8.0; capsule.height = 18.0
	shape.shape = capsule
	player.add_child(shape)
	_make_sprite(player, Color(0.2, 0.4, 0.9, 1), Vector2(16, 28))
	add_child(player)
	camera.call("set_follow_target", player)
	game_mgr.call("set_player", player)
	inv_panel.call("set_player", player)
	telemetry.call("set_player", player) # telemetry data source

	# Set initial checkpoint (auto-saves here)
	game_mgr.call("set_checkpoint", Vector2(200, 200), "")

	# Connect player death to GameManager
	if player.has_signal("player_died"):
		player.connect("player_died", game_mgr.on_player_died)

	# ---- Interact Hint (must exist before portals connect portal_prompt) ----
	var hint_layer = CanvasLayer.new()
	add_child(hint_layer)
	_interact_hint = Label.new()
	_interact_hint.position = Vector2(200, 210)
	_interact_hint.add_theme_font_size_override("font_size", 16)
	_interact_hint.add_theme_color_override("font_color", Color(1, 1, 0.5, 1))
	_interact_hint.visible = false
	hint_layer.add_child(_interact_hint)

	# ---- Portals (composition: each portal owns its scene lifecycle) ----
	# Town portal at x=600
	_create_portal(600, "res://scenes/rooms/town.tscn", "[X] Enter Town", player, camera)
	# Cave portal at x=1000
	_create_portal(1000, "res://scenes/rooms/cave.tscn", "[X] Enter Cave", player, camera)

	# ---- Enemies (variety: melee, archer, flyer, boss) ----
	# Melee grunt
	_spawn_enemy(Vector2(350, 200), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0)
	# Melee grunt (tougher)
	var e2 = _spawn_enemy(Vector2(500, 200), Color(0.9, 0.3, 0.1, 1), 55.0, 200.0)
	e2.set("max_health", 2.0); e2.set("current_health", 2.0)
	# Archer — stays at range, shoots projectiles
	var archer = _spawn_enemy(Vector2(750, 200), Color(0.2, 0.8, 0.2, 1), 80.0, 380.0)
	archer.set("is_ranged", true)
	archer.set("attack_range", 300.0)
	archer.set("preferred_distance", 200.0)
	archer.set("attack_damage", 8.0)
	archer.set("attack_cooldown", 1.5)
	# Flying enemy — hovers in the air
	var flyer = _spawn_enemy(Vector2(650, 150), Color(0.7, 0.3, 1.0, 1), 100.0, 300.0)
	flyer.set("is_flying", true)
	flyer.set("attack_range", 50.0)
	flyer.set("attack_damage", 12.0)
	# Melee grunt near cave
	_spawn_enemy(Vector2(950, 200), Color(0.9, 0.2, 0.2, 1), 65.0, 200.0)
	# Archer near cave
	var archer2 = _spawn_enemy(Vector2(1050, 200), Color(0.2, 0.8, 0.2, 1), 70.0, 350.0)
	archer2.set("is_ranged", true)
	archer2.set("attack_range", 280.0)
	archer2.set("preferred_distance", 180.0)
	archer2.set("attack_damage", 10.0)
	archer2.set("attack_cooldown", 1.3)
	# BOSS — large, multi-phase, special attacks
	var boss = _spawn_enemy(Vector2(1200, 195), Color(1.0, 0.1, 0.1, 1), 40.0, 500.0)
	boss.set("is_boss", true)
	boss.set("display_name", "赤瞳魔狼")
	# 属性注册后才真正生效；_ready 的 ×5 已过（add_child 时 is_boss 还是 false），
	# 这里直接给最终值 3×5=15
	boss.set("max_health", 15.0); boss.set("current_health", 15.0)
	boss.set("attack_damage", 20.0)
	boss.set("attack_cooldown", 1.2)
	boss.set("detection_radius", 500.0)
	# Boss visual: bigger sprite
	boss.get_node("Polygon2D").scale = Vector2(1.5, 1.5)
	boss.connect("boss_died", _on_boss_died)

	# ---- Item Pickups (test items scattered on the ground) ----
	_spawn_item_pickup(Vector2(250, 220), "healing_pill", 1)
	_spawn_item_pickup(Vector2(300, 220), "qi_pill", 1)
	_spawn_item_pickup(Vector2(400, 220), "spirit_stone", 3)
	_spawn_item_pickup(Vector2(450, 220), "healing_pill", 1)
	_spawn_item_pickup(Vector2(550, 220), "qi_pill", 2)
	_spawn_item_pickup(Vector2(700, 220), "foundation_pill", 1)
	_spawn_item_pickup(Vector2(800, 220), "spirit_stone", 5)
	_spawn_item_pickup(Vector2(850, 220), "healing_pill", 2)
	_spawn_item_pickup(Vector2(260, 220), "flying_sword", 1) # 筑基御剑飞行测试（出生点旁，金色菱形）

	# ---- Key input for save/load test ----
	set_process_input(true)

	print("Open world ready. Walk to portal markers and press F.")
	print("Pick up items by walking over diamond markers.")
	print("Autosave on checkpoint. Press F6 to reload from last save.")

func _input(event):
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_F6:
			# Reload from last autosave
			var gm = get_node_or_null("GameManager")
			if gm and gm.has_save("auto"):
				gm.load_game("auto")
				print("Loaded from auto-save!")
			else:
				print("No save file found.")

func _create_portal(x, scene_path, prompt, player, camera):
	var portal = ClassDB.instantiate("Portal")
	portal.position = Vector2(x, 210)  # near ground level
	portal.set("target_scene", scene_path)
	portal.set("prompt_text", prompt)
	portal.set_collision_mask_value(3, true)  # detect Player
	portal.call("set_player", player)
	portal.call("set_camera", camera)

	# Tall collision shape covering player height range
	var ds = CollisionShape2D.new()
	var dr = RectangleShape2D.new()
	dr.size = Vector2(32, 80)
	ds.shape = dr
	portal.add_child(ds)

	# Visual marker on ground at player feet level
	var marker = Polygon2D.new()
	marker.color = Color(0.8, 0.7, 0.3, 0.5)
	marker.polygon = PackedVector2Array([Vector2(-16, 25), Vector2(16, 25), Vector2(16, 35), Vector2(-16, 35)])
	add_child(marker)
	marker.position = Vector2(x, 210)

	portal.connect("portal_prompt", _on_portal_prompt)
	add_child(portal)

func _on_portal_prompt(text, show):
	if _interact_hint == null:
		return
	_interact_hint.text = text
	_interact_hint.visible = show

func _spawn_item_pickup(pos, item_id, qty):
	var pickup = ClassDB.instantiate("ItemPickup")
	if not pickup: return
	pickup.position = pos
	pickup.set("item_id", item_id)
	pickup.set("quantity", qty)
	add_child(pickup)

func _spawn_enemy(pos, color, speed, detect_range):
	var enemy = ClassDB.instantiate("Enemy")
	if not enemy: return
	enemy.position = pos
	enemy.set("move_speed", speed)
	enemy.set("detection_radius", detect_range)
	enemy.set("attack_range", 35.0)
	var eshape = CollisionShape2D.new()
	var ecap = CapsuleShape2D.new()
	ecap.radius = 10.0; ecap.height = 20.0
	eshape.shape = ecap
	enemy.add_child(eshape)
	_make_sprite(enemy, color, Vector2(20, 28))
	enemy.connect("enemy_died", _on_enemy_died)
	add_child(enemy)
	return enemy

func _on_enemy_died():
	print("Enemy killed!")

func _on_boss_died():
	print("BOSS DEFEATED!")
