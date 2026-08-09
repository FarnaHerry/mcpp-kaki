extends Area2D
## 扩张碑（洞天 v4 扩张经营）：贴近显示下一块灵田价格，
## X 购买扩张（6 → 最多 12 块，价格递增，走 CurrencySystem 四阶钱包）。
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
	# 石碑视觉（青灰方碑）
	var stone = Polygon2D.new()
	stone.color = Color(0.55, 0.6, 0.62, 1)
	stone.polygon = PackedVector2Array([-7, 0, -7, -22, 0, -27, 7, -22, 7, 0])
	add_child(stone)

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

func _interact():
	var mgr = _mgr()
	if mgr == null:
		return
	if int(mgr.call("get_plot_count")) >= 12:
		_prompt("灵田已至极限（十二块）", true)
		return
	var cost = int(mgr.call("get_expand_cost"))
	if bool(mgr.call("expand_plot")):
		# 新地块立即就位（dongtian.gd 补建 FarmPlot 节点）
		var host = get_parent()
		if host and host.has_method("refresh_plots"):
			host.call("refresh_plots")
		_result("灵田扩张 · 第%d块开垦完成" % int(mgr.call("get_plot_count")))
	else:
		_result("灵石不足（需下品×%d）" % cost)

func _refresh_prompt():
	var mgr = _mgr()
	if mgr == null:
		return
	var count = int(mgr.call("get_plot_count"))
	if count >= 12:
		_prompt("灵田已至极限（十二块）", true)
		return
	var cost = int(mgr.call("get_expand_cost"))
	var cur = _cur()
	if cur and int(cur.call("get_total")) >= cost:
		_prompt("[X] 扩张灵田（第%d块）· 下品×%d" % [count + 1, cost], true)
	else:
		_prompt("扩张灵田 需下品×%d（灵石不足）" % cost, true)

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
