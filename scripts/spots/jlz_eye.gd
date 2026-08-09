extends Area2D
## 阵眼（洞天 v4 聚灵阵升级）：贴近显示下一级价格，
## X 升级聚灵阵（两级，每级打坐倍率 +0.5；上品×5 / 上品×15）。
## StorageChest/RefineSpot 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
var _player_inside := false
var _player: Node = null
var _refresh_t := 0.0
var _result_t := 0.0 # 结果话术展示期（周期重声明让位）

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(40, 50)
	shape.shape = rect
	add_child(shape)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	set_process(true)

func _mgr():
	return get_tree().current_scene.find_child("DongtianManager", true, false)

func _cur():
	return get_tree().current_scene.find_child("CurrencySystem", true, false)

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
	# 结果话术让位；否则周期重声明（防同帧 enter/exit 乱序误清提示）
	if _result_t > 0.0:
		_result_t -= d
	elif _player_inside:
		_refresh_t -= d
		if _refresh_t <= 0.0:
			_refresh_t = 0.3
			_refresh_prompt()
	if _player_inside and Input.is_action_just_pressed("interact"):
		_interact()

func _cost_text(_level: int) -> String:
	# 价格一律下品基准（500/1500），话术按上品展示（×5 / ×15）
	return "上品×%d" % (int(_mgr().call("get_jlz_upgrade_cost")) / 100)

func _interact():
	var mgr = _mgr()
	if mgr == null:
		return
	var level = int(mgr.call("get_jlz_level"))
	if level >= 2:
		_prompt("聚灵阵已至极限", true)
		return
	if bool(mgr.call("upgrade_jlz")):
		_result("聚灵阵升级 · 打坐倍率+0.5（当前+%.1f）" % float(mgr.call("get_jlz_bonus")))
	else:
		_result("灵石不足（需%s）" % _cost_text(level))

func _refresh_prompt():
	var mgr = _mgr()
	if mgr == null:
		return
	var level = int(mgr.call("get_jlz_level"))
	if level >= 2:
		_prompt("聚灵阵已至极限", true)
		return
	var cost = int(mgr.call("get_jlz_upgrade_cost"))
	var cur = _cur()
	if cur and int(cur.call("get_total")) >= cost:
		_prompt("[X] 升级聚灵阵（+%.1f→+%.1f）· %s" % [level * 0.5, (level + 1) * 0.5, _cost_text(level)], true)
	else:
		_prompt("升级聚灵阵 需%s（灵石不足）" % _cost_text(level), true)

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
