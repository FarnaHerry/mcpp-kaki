# 验证: 树暂停期间（渡劫三灾场景）伤害数字照常上浮淡出并销毁
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _scene = null

func _initialize():
	_scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(_scene)
	current_scene = _scene
	print("[TEST] main scene loaded")

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _num_count() -> int:
	# 伤害数字 = current_scene 下带 Label 子节点的 Node2D
	var n := 0
	for c in _scene.get_children():
		if c is Node2D and c.get_child_count() > 0 and c.get_child(0) is Label:
			n += 1
	return n

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			# 模拟渡劫：树暂停 + DoT 连续跳伤害（TribulationController 是 ALWAYS 模式）
			paused = true
			var bus = root.find_child("SignalBus", true, false)
			for i in 5:
				bus.emit_signal("damage_dealt", Vector2(200, 150), 3.0, true)
			print("[TEST] paused, spawned 5 numbers")
		2:
			_next = _t + 0.3
			print("[TEST] numbers while paused: ", _num_count())
			_check(_num_count() > 0, "numbers spawned while paused")
		3:
			# 暂停中等待超过寿命 0.8s → 应全部自然消失
			_next = _t + 1.2
		4:
			_next = _t + 0.2
			print("[TEST] numbers after 1.5s still paused: ", _num_count())
			_check(_num_count() == 0, "numbers faded and freed while paused")
			paused = false
		5:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
