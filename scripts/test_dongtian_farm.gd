# 洞天 v2 灵田 harness:
# ①炼虚入洞天 ②空地 X 播种（扣草药）③生长中状态+倒计时提示 ④debug 拨快→成熟
# ⑤X 收获（种一收二）⑥种植状态存档/读档持久化
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

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

func _gm():
	return root.find_child("GameManager", true, false)

func _herb_count() -> int:
	return int(_player().call("get_inventory").call("get_item_count", "zhi_xue_cao"))

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

func _plot(i: int) -> Dictionary:
	return _dt().call("get_plot", i)

func _goto_plot(i: int):
	# 与 scripts/rooms/dongtian.gd 的摆点一致：66 + i*24, y=220
	_player().global_position = Vector2(66.0 + i * 24.0, 214.0)

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
			_player().call("get_inventory").call("add_item", "zhi_xue_cao", 3)
			_check(_herb_count() == 3, "备好止血草 ×3")
			_step = 55 # 等一拍让能力解锁落地
		55:
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == true, "进入洞天")
			_goto_plot(0)
			_next = _t + 0.5
			_step = 2
		2:
			_check(_hud_has("播种"), "空地提示：播种")
			_press("interact")
			_step = 3
		3:
			var p = _plot(0)
			_check(p.get("empty", true) == false, "播种成功（地块非空）")
			_check(String(p.get("herb", "")) == "zhi_xue_cao", "种的是止血草")
			_check(_herb_count() == 2, "播种扣 1（3→2）")
			_check(p.get("mature", true) == false, "生长中（未成熟）")
			_check(int(p.get("remaining", 0)) > 0, "剩余时间 > 0")
			_step = 4
		4:
			_check(_hud_has("生长中"), "生长中倒计时提示")
			_dt().call("debug_age_plot", 0, 120.0)
			_step = 5
		5:
			_check(_plot(0).get("mature", false) == true, "拨快后成熟")
			_next = _t + 0.5 # 等提示刷新
			_step = 6
		6:
			_check(_hud_has("收获"), "成熟提示：收获")
			_press("interact")
			_step = 7
		7:
			_check(_plot(0).get("empty", false) == true, "收获后地块空置")
			_check(_herb_count() == 4, "种一收二（2→4）")
			_step = 8
		8:
			# 持久化：地块 1 播种 → 存档 → 读档 → 状态保留
			_goto_plot(1)
			_next = _t + 0.5
			_step = 9
		9:
			_press("interact")
			_step = 10
		10:
			_check(String(_plot(1).get("herb", "")) == "zhi_xue_cao", "地块 1 播种成功")
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 11
		11:
			var p = _plot(1)
			_check(p.get("empty", true) == false and String(p.get("herb", "")) == "zhi_xue_cao",
					"读档后地块 1 种植状态保留")
			_check(_dt().call("is_inside") == false, "读档后已退出洞天（v1 行为不变）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
