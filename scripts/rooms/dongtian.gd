# 洞天（v2 灵田 + 仓库 / v4 扩张经营 / 设施补全：灵泉打坐点+丹房+灵植采集点×2
# / v5 药童托付照料 + 灵兽栏降伏入驻）
# —— C++ 节点运行时创建（GDExtension 注册时序，勿写进 .tscn）
extends Node2D

const BASE_PLOTS := 6
const PLOT_X0 := 66.0
const PLOT_DX := 24.0
const PLOT_Y := 220.0
# v4：第 7~12 块在灵田上方第二排
const PLOT2_Y := 196.0

const ExpandMonument := preload("res://scripts/spots/expand_monument.gd")
const JlzEye := preload("res://scripts/spots/jlz_eye.gd")
const MeditateSpot := preload("res://scripts/spots/meditate_spot.gd")
const PillLab := preload("res://scripts/spots/pill_lab.gd")
const DongtianHerbSpot := preload("res://scripts/spots/dongtian_herb_spot.gd")
const YaoTong := preload("res://scripts/spots/yao_tong.gd")
const WC := preload("res://scripts/world_common.gd")

# 灵兽闯阵：入侵灵兽种类与出生点（Manager 触发后回调 spawn_invasion 装配）
const INVASION_KINDS := ["qie_ling_shu", "tan_ling_feng"]
const INVASION_SPOTS := [Vector2(100, 190), Vector2(380, 190)]

# 灵兽栏（v5）：浮空围栏台位置（中央上空，避开出生点下落走廊与灵植高台）
const PEN_POS := Vector2(240, 128)

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

	# 设施补全：灵泉打坐点（灵泉右侧，x=415 避开阵眼 345..385 交互区）
	var spot = MeditateSpot.new()
	spot.name = "MeditateSpot"
	spot.position = Vector2(415, 216)
	add_child(spot)

	# 设施补全：丹房（仓库与灵泉之间，x=316 避开箱子 272..298 交互区）
	var lab = PillLab.new()
	lab.name = "PillLab"
	lab.position = Vector2(316, 216)
	add_child(lab)

	# 设施补全：灵植采集点×2（浮空苗圃单向高台，跳跃可达；地面让位既有设施）
	# 聚灵草台：仓库/丹房上空（x=296）；千年灵芝台：灵泉上空（x=420）
	_make_herb_ledge("HerbLedge0", Vector2(296, 176), 0)
	_make_herb_ledge("HerbLedge1", Vector2(420, 168), 1)

	# 采集点扩充：冰心莲台（灵田左端上空 x=84）/赤焰花台（灵田右端上空 x=176）
	_make_herb_ledge("HerbLedge2", Vector2(84, 168), 2)
	_make_herb_ledge("HerbLedge3", Vector2(176, 156), 3)

	# v5 药童（灵泉右侧远角，x=452：
	# 出生点 (240,200) 48px 幽灵 enter 守卫半径内不得放交互物——
	# 进洞天重挂载帧的物理误报会被守卫拦下；30px 距离内则 enter/exit 连发误清提示。
	# 地面 195..285 全在此半径内，故落在打坐点右侧边角）
	var yaotong = YaoTong.new()
	yaotong.name = "YaoTong"
	yaotong.position = Vector2(452, 216)
	add_child(yaotong)

	# v5 灵兽栏（中央上空浮台 + 围栏视觉；降伏的灵兽由 refresh_beast_pen 装配）
	_make_beast_pen()

	# 自家小世界：不压制（-1），显式还原防残留
	call_deferred("_suppress_player", -1)

# 灵兽闯阵：Manager 判定触发后回调（realm 已按玩家-1 算好）。返回实际生成数。
func spawn_invasion(realm: int) -> int:
	var count: int = 1 + randi() % 2
	var spawned := 0
	for i in count:
		var id: String = INVASION_KINDS[randi() % INVASION_KINDS.size()]
		var e = WC.spawn_enemy_by_id(self, INVASION_SPOTS[i], id)
		if e == null:
			continue
		e.set("realm", max(1, realm))
		e.add_to_group("dongtian_invaders")
		spawned += 1
	return spawned

# 灵兽栏：单向浮台（可跳上/从下方穿过）+ 围栏视觉 + 栏牌；灵兽按 Manager 状态装配
func _make_beast_pen():
	var pen = Node2D.new()
	pen.name = "BeastPen"
	pen.position = PEN_POS
	var platform = StaticBody2D.new()
	platform.name = "PenPlatform"
	platform.collision_layer = 1
	platform.collision_mask = 0
	var pshape = CollisionShape2D.new()
	var prect = RectangleShape2D.new()
	prect.size = Vector2(96, 6)
	pshape.shape = prect
	pshape.one_way_collision = true
	platform.add_child(pshape)
	pen.add_child(platform)
	# 围栏：两侧栏柱 + 横杆（纯视觉）
	var fence = Polygon2D.new()
	fence.name = "PenFence"
	fence.color = Color(0.55, 0.4, 0.22, 1)
	fence.polygon = PackedVector2Array([
		-48, -16, -44, -16, -44, -2, -48, -2,
		44, -16, 48, -16, 48, -2, 44, -2,
		-48, -13, 48, -13, 48, -10, -48, -10,
	])
	pen.add_child(fence)
	var label = Label.new()
	label.name = "PenLabel"
	label.text = "灵兽栏"
	label.add_theme_font_size_override("font_size", 7)
	label.add_theme_color_override("font_color", Color(0.95, 0.92, 0.8, 1))
	label.position = Vector2(-40, -30)
	label.size = Vector2(80, 10)
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	pen.add_child(label)
	add_child(pen)
	refresh_beast_pen()

# 按 DongtianManager 灵兽栏状态重建栏内灵兽视觉（降伏即时刷新 / 进洞天重建）
func refresh_beast_pen():
	var pen = get_node_or_null("BeastPen")
	if pen == null:
		return
	for c in pen.get_children():
		if String(c.name).begins_with("Beast"):
			pen.remove_child(c)
			c.free()
	var mgr = _mgr()
	if mgr == null:
		return
	var count := int(mgr.call("get_beast_count"))
	for i in count:
		var info: Dictionary = mgr.call("get_beast", i)
		var sx := (i - 1) * 30.0
		var beast = Polygon2D.new()
		beast.name = "BeastVisual%d" % i
		beast.position = Vector2(sx, -6)
		# 小兽趴伏剪影：窃灵鼠灰褐 / 贪灵蜂蜜黄，其余青灰兜底
		match String(info.get("id", "")):
			"qie_ling_shu":
				beast.color = Color(0.62, 0.55, 0.45, 1)
			"tan_ling_feng":
				beast.color = Color(0.9, 0.78, 0.3, 1)
			_:
				beast.color = Color(0.55, 0.7, 0.68, 1)
		beast.polygon = PackedVector2Array([-7, 0, 7, 0, 6, -6, 2, -8, -2, -8, -6, -6])
		pen.add_child(beast)
		var nl = Label.new()
		nl.name = "BeastName%d" % i
		nl.text = String(info.get("name", ""))
		nl.add_theme_font_size_override("font_size", 6)
		nl.add_theme_color_override("font_color", Color(0.9, 0.88, 0.75, 1))
		nl.position = Vector2(sx - 20, -22)
		nl.size = Vector2(40, 8)
		nl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		pen.add_child(nl)

# 浮空苗圃：单向高台（可跳上/从下方穿过）+ 土壤视觉 + 灵植采集点
func _make_herb_ledge(ledge_name: String, pos: Vector2, spot_index: int):
	var platform = StaticBody2D.new()
	platform.name = ledge_name
	platform.position = pos
	platform.collision_layer = 1
	platform.collision_mask = 0
	var pshape = CollisionShape2D.new()
	var prect = RectangleShape2D.new()
	prect.size = Vector2(48, 6)
	pshape.shape = prect
	pshape.one_way_collision = true
	platform.add_child(pshape)
	var soil = Polygon2D.new()
	soil.color = Color(0.38, 0.27, 0.16, 1)
	soil.polygon = PackedVector2Array([-24, -6, 24, -6, 20, 0, -20, 0])
	platform.add_child(soil)
	add_child(platform)
	var herb = DongtianHerbSpot.new()
	herb.name = "HerbSpot%d" % spot_index
	herb.position = pos + Vector2(0, -3)
	herb.set("spot_index", spot_index)
	add_child(herb)

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
