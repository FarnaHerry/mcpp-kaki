# Open world Metroidvania — Portal-based room entry (composition pattern).
extends Node2D

var _interact_hint = null

func _ready():
	call_deferred("_setup_game")

func _make_sprite(parent, color, size, pos=Vector2.ZERO):
	var poly = Polygon2D.new()
	poly.color = color
	var half = size * 0.5
	poly.polygon = PackedVector2Array([Vector2(-half.x, -half.y), Vector2(half.x, -half.y), Vector2(half.x, half.y), Vector2(-half.x, half.y)])
	poly.position = pos
	parent.add_child(poly)

func _setup_game():
	# ---- Camera ----
	var camera = ClassDB.instantiate("CameraRoom2D")
	camera.name = "CameraRoom2D"
	add_child(camera)

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

	# ---- Portals (composition: each portal owns its scene lifecycle) ----
	# Town portal at x=600
	_create_portal(600, "res://scenes/rooms/town.tscn", "[F] Enter Town", player, camera)
	# Cave portal at x=1000
	_create_portal(1000, "res://scenes/rooms/cave.tscn", "[F] Enter Cave", player, camera)

	# ---- Enemies ----
	_spawn_enemy(Vector2(350, 200), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0)
	_spawn_enemy(Vector2(500, 200), Color(0.9, 0.2, 0.2, 1), 50.0, 180.0)
	_spawn_enemy(Vector2(750, 200), Color(0.9, 0.5, 0.2, 1), 70.0, 220.0)
	_spawn_enemy(Vector2(900, 200), Color(0.9, 0.2, 0.2, 1), 65.0, 190.0)
	_spawn_enemy(Vector2(1150, 200), Color(1.0, 0.4, 0.1, 1), 55.0, 200.0)

	# ---- Interact Hint ----
	var hint_layer = CanvasLayer.new()
	add_child(hint_layer)
	_interact_hint = Label.new()
	_interact_hint.position = Vector2(200, 210)
	_interact_hint.add_theme_font_size_override("font_size", 16)
	_interact_hint.add_theme_color_override("font_color", Color(1, 1, 0.5, 1))
	_interact_hint.visible = false
	hint_layer.add_child(_interact_hint)

	# ---- Debug HUD ----
	var hud_layer = CanvasLayer.new()
	add_child(hud_layer)
	var hud_label = Label.new()
	hud_label.position = Vector2(10, 10)
	hud_label.add_theme_font_size_override("font_size", 12)
	hud_label.add_theme_color_override("font_color", Color.WHITE)
	hud_layer.add_child(hud_label)

	var timer = Timer.new()
	timer.wait_time = 0.2; timer.autostart = true
	timer.timeout.connect(_debug_update.bind(hud_label, player))
	add_child(timer)

	print("Open world ready. Walk to portal markers and press F.")

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
	_interact_hint.text = text
	_interact_hint.visible = show

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

func _on_enemy_died():
	print("Enemy killed!")

func _debug_update(label, player):
	var vel = player.get("velocity")
	var txt = "Pos:(%.0f, %.0f) Vel:(%.0f, %.0f)\n" % [player.position.x, player.position.y, vel.x, vel.y]
	txt += "Floor:%s  Wall:%s" % [player.is_on_floor(), player.is_on_wall()]
	label.text = txt
