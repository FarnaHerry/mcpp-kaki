# 洲场景公共装配（design/world-map.md 四大部洲）：
# 管理器/UI/玩家/相机创建 + 地形/敌人/草药/检查点/传送门 helper。
# 每洲根场景脚本：var ctx = WorldCommon.setup(self) → 再搭本洲地形。
extends RefCounted

# 自引用（static 信号回调用；preload 自引用会循环，运行时 load 有缓存）
static func _wc() -> GDScript:
	return load("res://scripts/world_common.gd")

# ---- 视觉占位 ----
static func make_sprite(parent: Node, color: Color, size: Vector2, pos := Vector2.ZERO) -> Polygon2D:
	var poly = Polygon2D.new()
	poly.name = "Polygon2D"
	poly.color = color
	var half = size * 0.5
	poly.polygon = PackedVector2Array([Vector2(-half.x, -half.y), Vector2(half.x, -half.y), Vector2(half.x, half.y), Vector2(-half.x, half.y)])
	poly.position = pos
	parent.add_child(poly)
	return poly

# ---- 核心装配：管理器 + UI + 玩家 + 相机（每洲一份）----
static func setup(root: Node) -> Dictionary:
	var signal_bus = ClassDB.instantiate("SignalBus")
	signal_bus.name = "SignalBus"
	root.add_child(signal_bus)

	var localization = ClassDB.instantiate("Localization")
	localization.name = "Localization"
	root.add_child(localization)

	var data_loader = ClassDB.instantiate("DataLoader")
	data_loader.name = "DataLoader"
	root.add_child(data_loader)

	var item_db = ClassDB.instantiate("ItemDatabase")
	item_db.name = "ItemDatabase"
	root.add_child(item_db)

	var game_mgr = ClassDB.instantiate("GameManager")
	game_mgr.name = "GameManager"
	root.add_child(game_mgr)

	var drop_system = ClassDB.instantiate("DropSystem")
	drop_system.name = "DropSystem"
	root.add_child(drop_system)

	var breakthrough_mgr = ClassDB.instantiate("BreakthroughManager")
	breakthrough_mgr.name = "BreakthroughManager"
	root.add_child(breakthrough_mgr)

	var dongtian_mgr = ClassDB.instantiate("DongtianManager")
	dongtian_mgr.name = "DongtianManager"
	root.add_child(dongtian_mgr)

	var dmg_numbers = ClassDB.instantiate("DamageNumbers")
	dmg_numbers.name = "DamageNumbers"
	root.add_child(dmg_numbers)

	var hud = ClassDB.instantiate("GameHUD")
	hud.name = "GameHUD"
	root.add_child(hud)

	var telemetry = ClassDB.instantiate("TelemetryPanel")
	telemetry.name = "TelemetryPanel"
	root.add_child(telemetry)

	var inv_panel = ClassDB.instantiate("InventoryPanel")
	inv_panel.name = "InventoryPanel"
	root.add_child(inv_panel)

	var game_menu = ClassDB.instantiate("GameMenu")
	game_menu.name = "GameMenu"
	root.add_child(game_menu)

	# 洲框架（云游图/旅行）
	var continents = ClassDB.instantiate("ContinentManager")
	continents.name = "ContinentManager"
	root.add_child(continents)

	var camera = ClassDB.instantiate("CameraRoom2D")
	camera.name = "CameraRoom2D"
	root.add_child(camera)
	game_mgr.call("set_camera", camera)

	var player = ClassDB.instantiate("Player")
	player.name = "Player"
	player.position = Vector2(200, 200)
	var shape = CollisionShape2D.new()
	var capsule = CapsuleShape2D.new()
	capsule.radius = 8.0; capsule.height = 18.0
	shape.shape = capsule
	player.add_child(shape)
	make_sprite(player, Color(0.2, 0.4, 0.9, 1), Vector2(16, 28))
	root.add_child(player)
	camera.call("set_follow_target", player)
	game_mgr.call("set_player", player)
	dongtian_mgr.call("set_player", player)
	dongtian_mgr.call("set_camera", camera)
	inv_panel.call("set_player", player)
	telemetry.call("set_player", player)

	# 旅行到岸：桥未应用前不碰初始检查点（否则用新档白板覆盖自动存档）
	if not game_mgr.call("has_pending_bridge"):
		var scene_path = str(root.get_tree().current_scene.scene_file_path)
		game_mgr.call("set_checkpoint", Vector2(200, 200), scene_path)

	if player.has_signal("player_died"):
		player.connect("player_died", game_mgr.on_player_died)

	# 交互提示（portal_prompt 显示层）
	var hint_layer = CanvasLayer.new()
	root.add_child(hint_layer)
	var hint = Label.new()
	hint.position = Vector2(200, 210)
	hint.add_theme_font_size_override("font_size", 16)
	hint.add_theme_color_override("font_color", Color(1, 1, 0.5, 1))
	hint.visible = false
	hint_layer.add_child(hint)

	return {
		"player": player, "camera": camera, "game_mgr": game_mgr,
		"hint": hint, "continents": continents, "telemetry": telemetry,
	}

# ---- 地形 helper ----
static func make_platform(root: Node, x: float, y: float, w: float, one_way := true) -> StaticBody2D:
	var body = StaticBody2D.new()
	body.name = "Platform_%d_%d" % [x, y]
	body.position = Vector2(x, y)
	if one_way:
		body.set_collision_layer_value(1, false)
		body.set_collision_layer_value(2, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, 6)
	shape.shape = rect
	shape.one_way_collision = one_way
	body.add_child(shape)
	make_sprite(body, Color(0.35, 0.4, 0.2, 1), Vector2(w, 6))
	root.add_child(body)
	return body

static func make_wall(root: Node, x: float, y_top: float, y_bottom: float, color := Color(0.22, 0.18, 0.15, 1)) -> StaticBody2D:
	var h = y_bottom - y_top
	var body = StaticBody2D.new()
	body.name = "Wall_%d_%d" % [x, y_top]
	body.position = Vector2(x, (y_top + y_bottom) / 2.0)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(12, h)
	shape.shape = rect
	body.add_child(shape)
	make_sprite(body, color, Vector2(12, h))
	root.add_child(body)
	return body

static func make_ground(root: Node, x0: float, x1: float, y := 238.0) -> StaticBody2D:
	var w = x1 - x0
	var body = StaticBody2D.new()
	body.name = "Ground_%d_%d" % [x0, x1]
	body.position = Vector2((x0 + x1) / 2.0, y)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(w, 24)
	shape.shape = rect
	body.add_child(shape)
	make_sprite(body, Color(0.25, 0.2, 0.15, 1), Vector2(w, 24), Vector2(0, 12))
	root.add_child(body)
	return body

# ---- 检查点（scene_path 记本洲场景，跨洲重生/读档可回对场景）----
static func create_checkpoint(root: Node, x: float, y := 210.0) -> Area2D:
	var area = Area2D.new()
	area.name = "Checkpoint_%d" % x
	area.position = Vector2(x, y)
	area.set_collision_layer_value(1, false)
	area.set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(24, 60)
	shape.shape = rect
	area.add_child(shape)
	var marker = Polygon2D.new()
	marker.color = Color(0.4, 0.8, 0.9, 0.5)
	marker.polygon = PackedVector2Array([Vector2(-5, -22), Vector2(5, -22), Vector2(5, 8), Vector2(-5, 8)])
	area.add_child(marker)
	area.connect("body_entered", Callable(_wc(), "_on_checkpoint_entered").bind(area, root))
	root.add_child(area)
	return area

static func _on_checkpoint_entered(body: Node, area: Area2D, root: Node) -> void:
	if body.name != "Player":
		return
	var gm = root.get_node_or_null("GameManager")
	if gm:
		var scene_path = str(root.get_tree().current_scene.scene_file_path)
		gm.call("set_checkpoint", area.global_position + Vector2(0, 12), scene_path)

# ---- 传送门（房间用；洲间走云游图）----
static func create_portal(root: Node, x: float, scene_path: String, prompt: String, player: Node, camera: Node, hint: Label) -> void:
	var portal = ClassDB.instantiate("Portal")
	portal.position = Vector2(x, 210)
	portal.set("target_scene", scene_path)
	portal.set("prompt_text", prompt)
	portal.set_collision_mask_value(3, true)
	portal.call("set_player", player)
	portal.call("set_camera", camera)
	var ds = CollisionShape2D.new()
	var dr = RectangleShape2D.new()
	dr.size = Vector2(32, 80)
	ds.shape = dr
	portal.add_child(ds)
	var marker = Polygon2D.new()
	marker.color = Color(0.8, 0.7, 0.3, 0.5)
	marker.polygon = PackedVector2Array([Vector2(-16, 25), Vector2(16, 25), Vector2(16, 35), Vector2(-16, 35)])
	root.add_child(marker)
	marker.position = Vector2(x, 210)
	portal.connect("portal_prompt", Callable(_wc(), "_on_portal_prompt").bind(hint))
	root.add_child(portal)

static func _on_portal_prompt(text: String, show: bool, hint: Label) -> void:
	if hint == null:
		return
	hint.text = text
	hint.visible = show

# ---- 内容 helper ----
static func spawn_herb(root: Node, pos: Vector2, herb_id: String, qty: int) -> void:
	var herb = ClassDB.instantiate("HerbNode")
	if not herb: return
	herb.name = "Herb_" + herb_id
	herb.position = pos
	herb.set("herb_id", herb_id)
	herb.set("quantity", qty)
	root.add_child(herb)

static func spawn_item_pickup(root: Node, pos: Vector2, item_id: String, qty: int) -> void:
	var pickup = ClassDB.instantiate("ItemPickup")
	if not pickup: return
	pickup.position = pos
	pickup.set("item_id", item_id)
	pickup.set("quantity", qty)
	root.add_child(pickup)

static func spawn_enemy(root: Node, pos: Vector2, color: Color, speed: float, detect_range: float, ename := "") -> Node:
	var enemy = ClassDB.instantiate("Enemy")
	if not enemy: return null
	if ename != "":
		enemy.name = ename
	enemy.position = pos
	enemy.set("move_speed", speed)
	enemy.set("detection_radius", detect_range)
	enemy.set("attack_range", 35.0)
	var eshape = CollisionShape2D.new()
	var ecap = CapsuleShape2D.new()
	ecap.radius = 10.0; ecap.height = 20.0
	eshape.shape = ecap
	enemy.add_child(eshape)
	make_sprite(enemy, color, Vector2(20, 28))
	enemy.connect("enemy_died", Callable(_wc(), "_on_enemy_died"))
	root.add_child(enemy)
	return enemy

static func _on_enemy_died() -> void:
	print("Enemy killed!")

static func on_boss_died() -> void:
	print("BOSS DEFEATED!")

# ---- 地标（世界坐标名牌）----
static func make_landmark(root: Node, x: float, y: float, text: String, color := Color(1.0, 0.9, 0.5, 1)) -> Label:
	var l = Label.new()
	l.text = text
	l.position = Vector2(x, y)
	l.add_theme_font_size_override("font_size", 14)
	l.add_theme_color_override("font_color", color)
	root.add_child(l)
	return l

# ---- F6 读档（各洲 _input 转发到这里）----
static func handle_input(root: Node, event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.keycode == KEY_F6:
			var gm = root.get_node_or_null("GameManager")
			if gm and gm.has_save("auto"):
				gm.load_game("auto")
				print("Loaded from auto-save!")
			else:
				print("No save file found.")
