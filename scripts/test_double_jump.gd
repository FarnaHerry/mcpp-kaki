# 二段跳 harness: ①凡人无二段跳 ②炼气解锁+空中按跳复升 ③每离地一次（第三跳无效）
#      ④落地刷新 ⑤筑基序列：二段跳用完后再按跳=飞行
# 注意：跳要按住（hold）几帧——跳按钮立即松开会触发可变跳高（jump_cut_multiplier=0.5
# 裁掉上冲速度，二段跳 -350→-105 正好卡检查阈值，导致时序性 FAIL）。
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _deadline := 0.0
var _held := "" # 当前按住待释放的跳按钮

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

func _tap(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _hold(action: String):
	_held = action
	Input.action_press(action)

func _release(action: String):
	if _held == action:
		_held = ""
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

func _vel_y() -> float:
	return _player().get("velocity").y

# 轮询等待下落段（vel.y>0）；到点转 _step+1，超时记 FAIL 并同样前进
func _wait_falling(what: String) -> bool:
	if _vel_y() > 0.0:
		return true
	if _t > _deadline:
		_check(false, what + "（等到下落段超时）")
		return true
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.15

	match _step:
		0:
			for c in current_scene.get_children():
				if c.get_class() == "Enemy":
					c.queue_free()
			_player().global_position = Vector2(600, 100)
			_player().set("velocity", Vector2.ZERO)
			_next = _t + 0.4
			_step = 1
		1: # 凡人：空中按跳无反应
			_tap("jump")
			_step = 2
		2:
			_check(_vel_y() > -50.0, "凡人空中按跳无反应（无二段跳）")
			_breakthrough_to(1)
			var am = _player().call("get_ability_manager")
			_check(am.call("has_ability", "double_jump"), "炼气解锁 double_jump 能力")
			_player().global_position = Vector2(600, 200)
			_player().set("velocity", Vector2.ZERO)
			_next = _t + 0.5
			_step = 3
		3:
			_check(_player().is_on_floor(), "已落地")
			_hold("jump") # 一段跳（按住保持满速，避免可变跳高裁减）
			_deadline = _t + 2.0
			_next = _t + 0.2
			_step = 4
		4:
			_release("jump")
			if _wait_falling("一段跳到下落段"):
				_hold("jump") # 二段跳（按住）
				_next = _t + 0.1
				_step = 5
		5:
			# 检查时仍按住 → 未被裁减（-350 起步）
			_check(_vel_y() < -100.0, "炼气二段跳（空中复升）")
			_release("jump")
			_deadline = _t + 2.0
			_step = 6
		6:
			if _wait_falling("二段跳后再下落"):
				_tap("jump") # 第三跳：应无效（tap，二段跳已用）
				_next = _t + 0.1
				_step = 7
		7:
			_check(_vel_y() > -50.0, "第三跳无效（每离地一次）")
			_check(String(_player().call("get_state_name")) != "fly", "炼气无飞行")
			_deadline = _t + 3.0
			_step = 8
		8:
			if _player().is_on_floor():
				_check(true, "落地（刷新二段跳）")
				_hold("jump")
				_deadline = _t + 2.0
				_next = _t + 0.2
				_step = 9
			elif _t > _deadline:
				_check(false, "落地超时")
				_step = 12
		9:
			_release("jump")
			if _wait_falling("刷新后一段跳到下落段"):
				_hold("jump")
				_next = _t + 0.1
				_step = 10
		10:
			_check(_vel_y() < -100.0, "落地刷新后二段跳可用")
			_release("jump")
			# 筑基序列：二段跳 → 飞行
			_breakthrough_to(2)
			_player().call("pickup_item", "flying_sword", 1)
			_player().global_position = Vector2(600, 200)
			_player().set("velocity", Vector2.ZERO)
			_next = _t + 0.5
			_step = 11
		11:
			_check(_player().is_on_floor(), "筑基落地")
			_hold("jump")
			_deadline = _t + 2.0
			_next = _t + 0.2
			_step = 12
		12:
			_release("jump")
			if _wait_falling("筑基一段跳到下落段"):
				_hold("jump") # 二段跳
				_next = _t + 0.1
				_step = 13
		13:
			_check(_vel_y() < -100.0, "筑基首次空按=二段跳")
			_release("jump")
			_deadline = _t + 2.0
			_step = 14
		14:
			if _wait_falling("筑基二段跳后再下落"):
				_hold("jump") # 二段跳已用 → 飞行（按住保持飞行）
				_next = _t + 0.3
				_step = 15
		15:
			var st = String(_player().call("get_state_name"))
			print("[TEST] state after 3rd press: ", st, " vel.y=", _vel_y())
			_release("jump")
			_check(st == "fly", "二段跳用完再按跳=飞行")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
