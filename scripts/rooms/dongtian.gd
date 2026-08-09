# 洞天（v2 灵田 + 仓库 / v4 扩张经营）—— C++ 节点运行时创建（GDExtension 注册时序，勿写进 .tscn）
extends Node2D

const BASE_PLOTS := 6
const PLOT_X0 := 66.0
const PLOT_DX := 24.0
const PLOT_Y := 220.0
# v4：第 7~12 块在灵田上方第二排
const PLOT2_Y := 196.0

const ExpandMonument := preload("res://scripts/spots/expand_monument.gd")
const JlzEye := preload("res://scripts/spots/jlz_eye.gd")

func _mgr():
	return get_tree().current_scene.find_child("DongtianManager", true, false)

func _plot_pos(i: int) -> Vector2:
	if i < BASE_PLOTS:
		return Vector2(PLOT_X0 + i * PLOT_DX, PLOT_Y)
	return Vector2(PLOT_X0 + (i - BASE_PLOTS) * PLOT_DX, PLOT2_Y)

func _ready():
	refresh_plots()

	# 仓库（储物箱）：灵田与灵泉之间（x=285；避开出生点 x=240 的下落走廊——
	# 玩家右缘 248 会蹭到箱子左缘 247，传送离开时触发 body_exited 误清提示）
	var chest = ClassDB.instantiate("StorageChest")
	chest.name = "StorageChest"
	chest.position = Vector2(285, 220)
	add_child(chest)

	# v4 扩张碑（灵田左侧，x=36 避开第 1 块地 55..77 的交互区）
	var monument = ExpandMonument.new()
	monument.name = "ExpandMonument"
	monument.position = Vector2(36, 216)
	add_child(monument)

	# v4 阵眼（聚灵阵阵纹中心，灵泉旁）
	var eye = JlzEye.new()
	eye.name = "JlzEye"
	eye.position = Vector2(365, 216)
	add_child(eye)

# 按 DongtianManager 当前地块数补建缺失的 FarmPlot（v4 扩张后新地立即可种）
func refresh_plots():
	var mgr = _mgr()
	var count: int = int(mgr.call("get_plot_count")) if mgr else BASE_PLOTS
	for i in count:
		if has_node("FarmPlot%d" % i):
			continue
		var plot = ClassDB.instantiate("FarmPlot")
		plot.name = "FarmPlot%d" % i
		plot.position = _plot_pos(i)
		plot.set("plot_index", i)
		add_child(plot)
	if count > BASE_PLOTS and not has_node("SpiritField2"):
		# 第二排灵田：单向高台（y 200..208，玩家可跳上/从下方穿过）+ 土壤视觉
		var platform = StaticBody2D.new()
		platform.name = "FieldPlatform"
		platform.position = Vector2(128, 204)
		platform.collision_layer = 1
		platform.collision_mask = 0
		var pshape = CollisionShape2D.new()
		var prect = RectangleShape2D.new()
		prect.size = Vector2(144, 8)
		pshape.shape = prect
		pshape.one_way_collision = true
		platform.add_child(pshape)
		add_child(platform)
		var soil = Polygon2D.new()
		soil.name = "SpiritField2"
		soil.color = Color(0.38, 0.27, 0.16, 1)
		soil.polygon = PackedVector2Array([56, 186, 200, 186, 200, 200, 56, 200])
		add_child(soil)
