extends Area2D
## 灵植采集点（洞天设施补全）：固定草药（聚灵草/千年灵芝），X 采集入包+喂练气，
## 采后枯萎，现实时间刷新（状态自持于 DongtianManager，场景卸载不丢）。
## StorageChest/JlzEye 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
## 用法：dongtian.gd 运行时创建并 set("spot_index", 0/1)。
var spot_index := 0

var _player_inside := false
var _player: Node = null
var _refresh_t := 0.0
var _result_t := 0.0 # 结果话术展示期（周期重声明让位）
var _visual: Polygon2D = null
var _grown := true # 视觉状态缓存（避免每帧改色）

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(28, 36)
	shape.shape = rect
	shape.position = Vector2(0, -10)
	add_child(shape)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	set_process(true)
	# 灵植视觉（三叶草形，品级配色：灵=冰蓝 / 地=紫金；与 HerbNode 一致）
	_visual = Polygon2D.new()
	_visual.color = Color(0.4, 0.7, 1.0, 0.85) if spot_index == 0 else Color(0.8, 0.5, 1.0, 0.9)
	_visual.polygon = PackedVector2Array([
		0, -16, 3, -9, 7, -13, 5, -7, 3, -5, -3, -5, -5, -7, -7, -13, -3, -9
	])
	add_child(_visual)

func _mgr():
	return get_tree().current_scene.find_child("DongtianManager", true, false)

func _spot() -> Dictionary:
	var mgr = _mgr()
	return mgr.call("get_herb_spot", spot_index) if mgr else {}

func _on_body_entered(body):
	if body.name != "Player":
		return
	# 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if body.global_position.distance_to(global_position) > 48.0:
		return
	_player_inside = true
	_player = body
	_refresh_prompt()

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	_player = null
	if body.get_parent() != get_parent():
		return
	_prompt("", false)

func _process(d):
	# 枯萎视觉 + 到点复生（周期轮询，0.5s 一拍足够）
	var spot := _spot()
	if not spot.is_empty():
		var available := bool(spot.get("available", true))
		if available != _grown:
			_grown = available
			var c: Color = _visual.color
			c.a = 0.85 if available else 0.18
			_visual.color = c
	# 结果话术让位；否则周期重声明（防同帧 enter/exit 乱序误清提示）
	if _result_t > 0.0:
		_result_t -= d
	elif _player_inside:
		_refresh_t -= d
		if _refresh_t <= 0.0:
			_refresh_t = 0.5
			_refresh_prompt()
	if _player_inside and Input.is_action_just_pressed("interact"):
		_interact()

func _interact():
	var mgr = _mgr()
	if mgr == null:
		return
	var spot := _spot()
	if spot.is_empty():
		return
	if bool(mgr.call("gather_herb_spot", spot_index)):
		_result("采得 %s×%d（练气+2）" % [String(spot["herb_name"]), int(spot["qty"])])
	else:
		_result("灵植生长中 · 约%d秒后成熟" % int(spot.get("remaining", 0)))

func _refresh_prompt():
	var spot := _spot()
	if spot.is_empty():
		return
	if bool(spot.get("available", true)):
		_prompt("[X] 采集 ·%s" % String(spot["herb_name"]), true)
	else:
		_prompt("灵植生长中 · 约%d秒" % int(spot.get("remaining", 0)), true)

func _prompt(text: String, show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, show)

func _result(msg: String):
	_prompt(msg, true)
	_result_t = 2.5
	var timer = get_tree().create_timer(2.5)
	timer.timeout.connect(func():
		if _player_inside:
			_refresh_prompt()
		else:
			_prompt("", false)
	)
