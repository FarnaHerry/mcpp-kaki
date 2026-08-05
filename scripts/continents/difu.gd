# 地府·黄泉路（design/cultivation-realms.md §五 地府体系）
# 判官（查生死簿）+ 生死簿（改簿划名）+ 还阳出口（全场景切换回主场景检查点）。
extends Node2D

const WC = preload("res://scripts/world_common.gd")
const GATE_SCRIPT = preload("res://scripts/gates/scene_gate.gd")

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _setup():
	var ctx = WC.setup(self)

	# 黄泉路背景（昏暗幽冥）
	var sky = Polygon2D.new()
	sky.color = Color(0.08, 0.08, 0.12, 1)
	sky.polygon = PackedVector2Array([Vector2(-20, -20), Vector2(500, -20), Vector2(500, 300), Vector2(-20, 300)])
	sky.z_index = -10
	add_child(sky)

	# 地（黄泉土）+ 左右墙
	WC.make_ground(self, -20, 500, 238)
	WC.make_wall(self, -14, 40, 270, Color(0.15, 0.12, 0.18, 1))
	WC.make_wall(self, 494, 40, 270, Color(0.15, 0.12, 0.18, 1))

	WC.make_landmark(self, 120, 60, "地府 · 黄泉路", Color(0.6, 0.5, 0.9, 1))

	# 彼岸花（红点装饰，黄泉路特征）
	for i in range(5):
		var flower = Polygon2D.new()
		flower.color = Color(0.75, 0.15, 0.2, 1)
		var px = 80 + i * 80
		flower.polygon = PackedVector2Array([Vector2(px, 230), Vector2(px + 6, 222), Vector2(px + 12, 230)])
		add_child(flower)

	# 判官（查生死簿）
	var judge = ClassDB.instantiate("UnderworldInteractNode")
	judge.name = "Panguan"
	judge.position = Vector2(150, 205)
	judge.set("mode", 0) # MODE_INSPECT
	add_child(judge)

	# 生死簿（改簿划名 → 免死一次）
	var book = ClassDB.instantiate("UnderworldInteractNode")
	book.name = "ShengSiBo"
	book.position = Vector2(330, 205)
	book.set("mode", 1) # MODE_AMEND
	add_child(book)

	# 还阳出口（场景门，↑ 触发）
	var gate = GATE_SCRIPT.new()
	gate.name = "HuanYangGate"
	gate.position = Vector2(445, 210)
	gate.set("gm_method", "huan_yang")
	gate.set("prompt", "[↑] 还阳")
	var gs = CollisionShape2D.new()
	var gr = RectangleShape2D.new()
	gr.size = Vector2(32, 70)
	gs.shape = gr
	gate.add_child(gs)
	var vis = Polygon2D.new()
	vis.color = Color(0.85, 0.9, 0.5, 1)
	vis.polygon = PackedVector2Array([Vector2(-6, -16), Vector2(6, -16), Vector2(6, 16), Vector2(-6, 16)])
	gate.add_child(vis)
	add_child(gate)
