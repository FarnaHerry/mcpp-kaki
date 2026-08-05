# 云海（跨洲强渡关卡，design/world-map.md）：金丹飞行门控。
# 机制：无地面——云墩落脚 + 罡风带推移 + 落雷柱（预警→劈落）+ 坠入云海遣返起云台。
# 起点=起云台(60,180)，终点=登岸区(2300)→ContinentManager.complete_travel。
extends Node2D

const WC = preload("res://scripts/world_common.gd")

const KILL_Y := 420.0
const BOLT_X := [500.0, 1000.0, 1500.0, 2000.0]
const BOLT_PERIOD := 4.0
const BOLT_WARN := 1.0
const BOLT_STRIKE := 0.25
const WIND_X := [700.0, 1300.0, 1900.0]

var _player = null
var _cm = null
var _t := 0.0
var _wind_areas := []
var _bolt_warn := []
var _bolt_strike := []
var _bolt_hit := [] # 每轮劈落只结算一次
var _arrived := false

func _ready():
	call_deferred("_setup")

func _input(event):
	WC.handle_input(self, event)

func _setup():
	var ctx = WC.setup(self)
	_player = ctx.player
	_cm = get_node_or_null("ContinentManager")

	# 天空底 + 云海远景
	var sky = Polygon2D.new()
	sky.color = Color(0.35, 0.5, 0.72, 1)
	sky.polygon = PackedVector2Array([Vector2(-100, -100), Vector2(2500, -100), Vector2(2500, 460), Vector2(-100, 460)])
	sky.z_index = -10
	add_child(sky)
	var sea = Polygon2D.new()
	sea.color = Color(0.85, 0.9, 0.98, 1)
	sea.polygon = PackedVector2Array([Vector2(-100, 380), Vector2(2500, 380), Vector2(2500, 460), Vector2(-100, 460)])
	sea.z_index = -9
	add_child(sea)

	WC.make_landmark(self, 40, 40, "云海 · 强渡四大部洲", Color(0.95, 0.97, 1.0, 1))

	# 起云台（坚实）+ 云墩（单向）+ 登岸台
	_cloud_platform(self, 60, 210, 150, false)
	var perches = [
		[280, 185, 70], [430, 140, 60], [620, 175, 60], [820, 130, 60],
		[1010, 170, 70], [1210, 125, 60], [1420, 175, 60], [1630, 130, 60],
		[1830, 170, 70], [2050, 140, 60], [2180, 185, 60],
	]
	for p in perches:
		_cloud_platform(self, p[0], p[1], p[2], true)
	_cloud_platform(self, 2300, 205, 160, false)

	# 散碎灵石（渡海彩头）
	WC.spawn_item_pickup(self, Vector2(1010, 164), "spirit_stone", 3)
	WC.spawn_item_pickup(self, Vector2(1830, 164), "spirit_stone", 3)

	# 罡风带（半透明风幕 + 区域推移）
	for wx in WIND_X:
		_make_wind(wx)

	# 落雷柱（预警黄幕 / 劈落白幕，初始隐藏）
	for bx in BOLT_X:
		_make_bolt(bx)

	# 雷鸟（飞行巡曳；平衡：HP 5→15，金丹玩家 2 击）
	for i in range(3):
		var bird = WC.spawn_enemy(self, Vector2(800 + i * 600, 110 + i * 10), Color(0.6, 0.6, 0.95, 1), 115.0, 320.0, "LeiNiao%d" % i)
		bird.set("is_flying", true)
		bird.set("max_health", 15.0); bird.set("current_health", 15.0); bird.set("realm", 3)
		bird.set("attack_damage", 12.0)

	# 登岸区
	var arrive = Area2D.new()
	arrive.name = "ArriveZone"
	arrive.position = Vector2(2300, 170)
	arrive.set_collision_mask_value(3, true)
	var ashape = CollisionShape2D.new()
	var arect = RectangleShape2D.new()
	arect.size = Vector2(150, 130)
	ashape.shape = arect
	arrive.add_child(ashape)
	arrive.connect("body_entered", Callable(self, "_on_arrive_entered"))
	add_child(arrive)

	print("云海 · 强渡")

func _cloud_platform(root: Node, x: float, y: float, w: float, one_way: bool) -> void:
	var p = WC.make_platform(root, x, y, w, one_way)
	p.get_node("Polygon2D").color = Color(0.92, 0.94, 1.0, 1)

func _make_wind(x: float) -> void:
	var area = Area2D.new()
	area.name = "Wind_%d" % x
	area.position = Vector2(x, 110)
	area.set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(90, 420)
	shape.shape = rect
	area.add_child(shape)
	add_child(area)
	var vis = Polygon2D.new()
	vis.color = Color(0.7, 0.85, 1.0, 0.18)
	vis.polygon = PackedVector2Array([Vector2(-45, -210), Vector2(45, -210), Vector2(45, 210), Vector2(-45, 210)])
	area.add_child(vis)
	_wind_areas.append(area)

func _make_bolt(x: float) -> void:
	var warn = Polygon2D.new()
	warn.color = Color(1.0, 0.9, 0.3, 0.25)
	warn.polygon = PackedVector2Array([Vector2(-13, -60), Vector2(13, -60), Vector2(13, 380), Vector2(-13, 380)])
	warn.position = Vector2(x, 0)
	warn.visible = false
	add_child(warn)
	_bolt_warn.append(warn)
	var strike = Polygon2D.new()
	strike.color = Color(1.0, 1.0, 0.85, 0.9)
	strike.polygon = PackedVector2Array([Vector2(-16, -60), Vector2(16, -60), Vector2(16, 380), Vector2(-16, 380)])
	strike.position = Vector2(x, 0)
	strike.visible = false
	add_child(strike)
	_bolt_strike.append(strike)
	_bolt_hit.append(false)

func _physics_process(delta):
	if _player == null or not is_instance_valid(_player):
		return
	_t += delta

	# 罡风：带内持续推移（逆风西行 + 下压）。
	# 注意：位移直推——速度增量会被状态机每帧的速度覆写（空中x由输入重设、飞行全量重设）
	for area in _wind_areas:
		if area.get_overlapping_bodies().has(_player):
			_player.global_position += Vector2(-75.0, 18.0) * delta

	# 落雷：周期 预警1.0s → 劈落0.25s（偏移错开）
	for i in range(BOLT_X.size()):
		var phase = fposmod(_t + i * 1.3, BOLT_PERIOD)
		var in_warn = phase < BOLT_WARN
		var in_strike = phase >= BOLT_WARN and phase < BOLT_WARN + BOLT_STRIKE
		_bolt_warn[i].visible = in_warn
		_bolt_strike[i].visible = in_strike
		if in_strike and not _bolt_hit[i]:
			_bolt_hit[i] = true
			if abs(_player.global_position.x - BOLT_X[i]) < 26.0:
				_player.call("take_damage", 20.0, self)
		elif not in_strike:
			_bolt_hit[i] = false

	# 坠入云海：遣返起云台 + 代价
	if _player.global_position.y > KILL_Y:
		_player.global_position = Vector2(60, 180)
		_player.velocity = Vector2.ZERO
		_player.call("take_damage", float(_player.call("get_max_health")) * 0.15, self)

func _on_arrive_entered(body: Node) -> void:
	if _arrived or body == null or body.name != "Player":
		return
	_arrived = true
	if _cm == null or not _cm.call("complete_travel"):
		# 目的地丢失（不应发生：travel_dest 随档持久化）——保底留在云海
		print("[云海] 登岸失败：目的洲不明")
		_arrived = false
