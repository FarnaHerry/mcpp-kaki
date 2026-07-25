# 西牛贺洲（stub v1：佛门故地，火焰山主题；design/world-map.md）
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _setup():
	var ctx = WC.setup(self)

	WC.make_landmark(self, 120, 140, "西牛贺洲 · 火焰山地界", Color(1.0, 0.6, 0.3, 1))

	WC.make_ground(self, -50, 2200, 238)
	WC.make_wall(self, -44, 40, 270, Color(0.35, 0.15, 0.1, 1))
	WC.make_wall(self, 2194, 40, 270, Color(0.35, 0.15, 0.1, 1))

	# 火焰山：赤岩台地（跳跃）
	WC.make_platform(self, 500, 180, 90)
	WC.make_platform(self, 700, 130, 90)
	WC.make_platform(self, 900, 170, 90)
	WC.make_platform(self, 1200, 120, 100)
	WC.make_platform(self, 1500, 160, 90)

	WC.create_checkpoint(self, 100)
	WC.create_checkpoint(self, 1100)

	# 火鸦（飞行）+ 火牛（近战）
	for i in range(3):
		var hy = WC.spawn_enemy(self, Vector2(600 + i * 300, 120 + i * 15), Color(0.9, 0.4, 0.1, 1), 110.0, 320.0, "HuoYa%d" % i)
		hy.set("is_flying", true)
		hy.set("max_health", 4.0); hy.set("current_health", 4.0); hy.set("realm", 3)
	var niu = WC.spawn_enemy(self, Vector2(1000, 210), Color(0.7, 0.2, 0.1, 1), 80.0, 260.0, "HuoNiu")
	niu.set("max_health", 10.0); niu.set("current_health", 10.0); niu.set("realm", 3)

	WC.spawn_herb(self, Vector2(700, 124), "chi_yan_hua", 2)
	WC.spawn_herb(self, Vector2(1500, 154), "chi_yan_hua", 1)

	print("西牛贺洲 · 火焰山地界（stub）")
