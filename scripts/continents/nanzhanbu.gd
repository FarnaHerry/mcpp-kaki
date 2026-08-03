# 南赡部洲（stub v1：人间王朝，长安坊市主题；design/world-map.md）
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _setup():
	var ctx = WC.setup(self)

	WC.make_landmark(self, 120, 140, "南赡部洲 · 长安郊外", Color(0.9, 0.8, 0.4, 1))

	WC.make_ground(self, -50, 2200, 238)
	WC.make_wall(self, -44, 40, 270)
	WC.make_wall(self, 2194, 40, 270)

	# 城郊：屋檐平台（跳跃）
	WC.make_platform(self, 450, 170, 100)
	WC.make_platform(self, 650, 130, 100)
	WC.make_platform(self, 900, 160, 100)
	WC.make_platform(self, 1300, 140, 110)

	WC.create_checkpoint(self, 100)
	WC.create_checkpoint(self, 1000)

	# 山贼（近战）+ 蛊雕（飞行）
	for i in range(3):
		WC.spawn_enemy(self, Vector2(500 + i * 350, 210), Color(0.5, 0.4, 0.2, 1), 85.0, 240.0, "ShanZei%d" % i).set("realm", 6)
	var gu = WC.spawn_enemy(self, Vector2(1300, 110), Color(0.4, 0.5, 0.6, 1), 115.0, 330.0, "GuDiao")
	gu.set("is_flying", true)
	gu.set("max_health", 5.0); gu.set("current_health", 5.0); gu.set("realm", 6)

	WC.spawn_herb(self, Vector2(300, 214), "zhi_xue_cao", 2)
	WC.spawn_herb(self, Vector2(1300, 134), "wu_dao_cha", 1)

	print("南赡部洲 · 长安郊外（stub）")
