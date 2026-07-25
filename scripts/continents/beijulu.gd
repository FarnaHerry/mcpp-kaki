# 北俱芦洲（stub v1：极北莽荒，玄冰主题；design/world-map.md）
extends Node2D

const WC = preload("res://scripts/world_common.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _setup():
	var ctx = WC.setup(self)

	WC.make_landmark(self, 120, 140, "北俱芦洲 · 玄冰原", Color(0.6, 0.85, 1.0, 1))

	WC.make_ground(self, -50, 2200, 238)
	WC.make_wall(self, -44, 40, 270, Color(0.5, 0.65, 0.8, 1))
	WC.make_wall(self, 2194, 40, 270, Color(0.5, 0.65, 0.8, 1))

	# 玄冰原：冰柱墙跳
	WC.make_wall(self, 500, 130, 238, Color(0.5, 0.65, 0.8, 1))
	WC.make_platform(self, 500, 124, 44, false)
	WC.make_wall(self, 600, 90, 238, Color(0.5, 0.65, 0.8, 1))
	WC.make_platform(self, 600, 84, 44, false)
	WC.make_platform(self, 900, 150, 90)
	WC.make_platform(self, 1200, 110, 90)

	WC.create_checkpoint(self, 100)
	WC.create_checkpoint(self, 1000)

	# 雪魈（近战厚血）+ 冰鸾（飞行）
	for i in range(2):
		var xx = WC.spawn_enemy(self, Vector2(800 + i * 400, 210), Color(0.7, 0.8, 0.9, 1), 75.0, 260.0, "XueXiao%d" % i)
		xx.set("max_health", 12.0); xx.set("current_health", 12.0)
	var luan = WC.spawn_enemy(self, Vector2(1200, 90), Color(0.5, 0.7, 1.0, 1), 120.0, 340.0, "BingLuan")
	luan.set("is_flying", true)
	luan.set("max_health", 6.0); luan.set("current_health", 6.0)

	WC.spawn_herb(self, Vector2(900, 144), "bing_xin_lian", 2)
	WC.spawn_herb(self, Vector2(300, 214), "jin_gang_teng", 1)

	print("北俱芦洲 · 玄冰原（stub）")
