# 打坐 harness: ①Q入坐 ②修为增长 ③移动收功 ④灵力回复×3 ⑤受击收功
#      ⑥修为封顶→入定1s自动请求机缘突破（事件暂停树）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _e0 := 0.0
var _m1 := 0.0
var _sit_t := 0.0

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			for c in current_scene.get_children():
				if c.get_class() == "Enemy":
					c.queue_free()
			_cult().call("set_free_breakthrough", false) # 关掉默认开启的调试无门槛
			_press("cultivate") # Q 入坐
			_step = 1
		1:
			_check(_player().call("is_meditating"), "Q 入坐（is_meditating）")
			_check(float(_player().call("get_meditate_rate")) >= 5.0, "打坐速率 >= 5/s")
			_e0 = float(_cult().call("get_current_energy"))
			_next = _t + 1.0
			_step = 2
		2:
			var e1 = float(_cult().call("get_current_energy"))
			print("[TEST] energy ", _e0, " -> ", e1)
			_check(e1 > _e0 + 3.0, "打坐修为增长（~5/s）")
			Input.action_press("right") # 移动收功
			_step = 3
		3:
			Input.action_release("right")
			_check(not _player().call("is_meditating"), "移动收功")
			_breakthrough_to(1) # 炼气（灵力池 50，基础回复 2%×50=1/s）
			_cult().call("set_spiritual_energy", 0.0) # 清空修为，避免打坐触发突破
			_check(float(_cult().call("get_current_energy")) == 0.0, "修为已清零（防误触突破）")
			_cult().call("set_mana", 0.0)
			_next = _t + 1.0
			_step = 4
		4:
			_m1 = float(_cult().call("get_mana")) # 站立 1s 基础回复
			_cult().call("set_mana", 0.0)
			_press("cultivate")
			_next = _t + 0.4
			_step = 5
		5:
			_check(_player().call("is_meditating"), "再次入坐")
			_next = _t + 1.0
			_step = 6
		6:
			var m2 = float(_cult().call("get_mana"))
			print("[TEST] mana regen standing ", _m1, " vs sitting ", m2)
			_check(m2 > _m1 * 2.0, "打坐灵力回复×3")
			_player().call("take_damage", 1.0, null) # 受击收功
			_step = 7
		7:
			_check(not _player().call("is_meditating"), "受击收功")
			# 修为封顶（非调试开关，走真实封顶条件）→ 入定 1s 自动请求突破
			_cult().call("accumulate_energy", 1000000000)
			_press("cultivate")
			_sit_t = _t
			_next = _t
			_step = 8
		8:
			if paused:
				_check(true, "修为封顶入坐 1s 自动请求机缘突破（事件暂停树）")
				_check(_t - _sit_t < 4.0, "突破请求在合理时窗内")
				paused = false
				_step = 9
			elif _t - _sit_t > 6.0:
				_check(false, "封顶入坐后未触发突破请求")
				_step = 9
			# 否则下一帧继续轮询
		9:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
