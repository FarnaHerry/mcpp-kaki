# 洞天 v3 聚灵阵 harness:
# ①洞天外打坐倍率 ×1 ②洞天内 ×2.0（炼虚）③打坐提示含聚灵阵标识 ④退出洞天恢复 ×1
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _rate_out := 0.0

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

func _dt():
	return root.find_child("DongtianManager", true, false)

func _rate() -> float:
	return float(_player().call("get_meditate_rate"))

func _mult() -> float:
	return float(_player().call("get_dongtian_meditate_mult"))

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			_cult().call("set_free_breakthrough", true)
			_cult().call("accumulate_energy", 100000000000)
			while int(_cult().call("get_realm_index")) < 6:
				_cult().call("attempt_breakthrough")
			_cult().call("set_free_breakthrough", false)
			_step = 55 # 等能力解锁落地
		55:
			_rate_out = _rate()
			_check(_mult() == 1.0, "洞天外聚灵阵倍率 ×1")
			_check(_rate_out > 0.0, "洞天外打坐速率正常")
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == true, "进入洞天")
			_check(_mult() == 2.0, "洞天内聚灵阵倍率 ×2.0（炼虚）")
			_check(abs(_rate() - _rate_out * 2.0) < 0.01, "洞天内打坐速率翻倍")
			_next = _t + 0.6 # 等落地（Q 打坐需在地面）
			_step = 56
		56:
			_press("cultivate") # Q 打坐（1s 内收功，避免触发封顶自动突破请求）
			_step = 2
		2:
			_check(_hud_has("聚灵阵×2.0"), "打坐提示含聚灵阵标识")
			_press("right") # 移动收功
			_press("left")
			_step = 3
		3:
			_press("dongtian") # 退出洞天
			_step = 4
		4:
			_check(_dt().call("is_inside") == false, "退出洞天")
			_check(_mult() == 1.0, "退出后倍率恢复 ×1")
			_check(abs(_rate() - _rate_out) < 0.01, "退出后速率恢复")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
