# 南赡部洲（人间王朝，长安坊市；design/world-map.md v4）
# 长安：商店掌柜（灵石买卖）+ 五庄观人参果 + 地府入口（正式版）
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

	# ===== 长安坊市（商店系统，灵石买卖）=====
	WC.make_landmark(self, 1400, 120, "长安 · 坊市", Color(0.95, 0.85, 0.4, 1))
	var keeper = ClassDB.instantiate("ShopKeeper")
	keeper.name = "ShopKeeper"
	keeper.position = Vector2(1500, 205)
	add_child(keeper)

	# ===== 五庄观：人参果（镇观灵果）=====
	WC.make_landmark(self, 1800, 120, "五庄观（人参果）", Color(0.6, 0.9, 0.5, 1))
	WC.spawn_item_pickup(self, Vector2(1850, 232), "ren_shen_guo", 1)
	WC.spawn_item_pickup(self, Vector2(1900, 232), "spirit_stone", 20)

	# ===== 地府入口（正式版：长安城内，design/world-map.md 南赡部洲地府入口）=====
	WC.make_landmark(self, 100, 60, "黄泉路入口（地府）", Color(0.6, 0.5, 0.9, 1))
	var difu_gate = load("res://scripts/gates/scene_gate.gd").new()
	difu_gate.name = "DifuGate"
	difu_gate.position = Vector2(240, 210)
	difu_gate.set("gm_method", "enter_difu")
	difu_gate.set("prompt", "[↑] 入黄泉路")
	var gs = CollisionShape2D.new()
	var gr = RectangleShape2D.new()
	gr.size = Vector2(32, 80)
	gs.shape = gr
	difu_gate.add_child(gs)
	var gv = Polygon2D.new()
	gv.color = Color(0.55, 0.4, 0.8, 1)
	gv.polygon = PackedVector2Array([Vector2(-7, -18), Vector2(7, -18), Vector2(7, 18), Vector2(-7, 18)])
	difu_gate.add_child(gv)
	add_child(difu_gate)

	print("南赡部洲 · 长安坊市")
