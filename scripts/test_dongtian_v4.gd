# 洞天 v4 扩张经营 harness:
# ①炼虚入洞天（初始 6 块地、聚灵阵 ×2.0）②扩张碑 X 购买第 7 块（扣下品×500）
# ③新地块立即可种（FarmPlot6 节点 + 播种成功）④阵眼 X 升级（扣上品×5=500 下品）
# ⑤打坐倍率 +0.5（×2.5，HUD 提示反映）⑥存档/读档 plot_count/jlz_level/种植状态保留
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _wallet0 := 0

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

func _cur():
	return root.find_child("CurrencySystem", true, false)

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
			_player().call("get_inventory").call("add_item", "zhi_xue_cao", 3)
			_wallet0 = int(_cur().call("get_total"))
			_cur().call("add", 0, 5000) # 下品 ×5000
			_check(int(_cur().call("get_total")) == _wallet0 + 5000, "钱包充值下品×5000")
			_step = 55 # 等能力解锁落地
		55:
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == true, "进入洞天")
			_check(int(_dt().call("get_plot_count")) == 6, "初始 6 块灵田")
			_check(int(_dt().call("get_expand_cost")) == 500, "第 7 块价格 = 下品×500")
			_check(_mult() == 2.0, "初始聚灵阵 ×2.0（炼虚）")
			_player().global_position = Vector2(36, 210) # 扩张碑
			_next = _t + 0.7
			_step = 2
		2:
			_check(_hud_has("扩张灵田") and _hud_has("500"), "扩张碑提示：价格下品×500")
			_press("interact")
			_step = 3
		3:
			_check(int(_dt().call("get_plot_count")) == 7, "扩张成功（7 块地）")
			_check(int(_cur().call("get_total")) == _wallet0 + 4500, "扣款下品×500")
			_check(root.find_child("FarmPlot6", true, false) != null, "新地块 FarmPlot6 已就位")
			_player().global_position = Vector2(66, 191) # 第 7 块（第二排高台）
			_next = _t + 0.7
			_step = 4
		4:
			_check(_hud_has("播种"), "新地块提示：播种")
			_press("interact")
			_step = 5
		5:
			var p = _dt().call("get_plot", 6)
			_check(p.get("empty", true) == false and String(p.get("herb", "")) == "zhi_xue_cao",
					"新地块立即可种（止血草）")
			_player().global_position = Vector2(365, 210) # 阵眼
			_next = _t + 0.7
			_step = 6
		6:
			_check(_hud_has("升级聚灵阵") and _hud_has("上品×5"), "阵眼提示：上品×5")
			_press("interact")
			_step = 7
		7:
			_check(int(_dt().call("get_jlz_level")) == 1, "聚灵阵升至 1 级")
			_check(int(_cur().call("get_total")) == _wallet0 + 4000, "扣款上品×5（=500 下品）")
			_check(abs(_mult() - 2.5) < 0.001, "打坐倍率 ×2.5（+0.5）")
			_next = _t + 0.6 # 等落地（Q 打坐需在地面）
			_step = 56
		56:
			_press("cultivate") # Q 打坐（1s 内收功，避免触发封顶自动突破请求）
			_step = 8
		8:
			_check(_hud_has("聚灵阵×2.50") or _hud_has("聚灵阵×2.5"), "打坐提示反映升级后倍率")
			_press("right") # 移动收功
			_press("left")
			_step = 9
		9:
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 10
		10:
			_check(int(_dt().call("get_plot_count")) == 7, "读档后地块数保留（7）")
			_check(int(_dt().call("get_jlz_level")) == 1, "读档后聚灵阵等级保留（1）")
			var p = _dt().call("get_plot", 6)
			_check(p.get("empty", true) == false and String(p.get("herb", "")) == "zhi_xue_cao",
					"读档后新地块种植状态保留")
			_check(_dt().call("is_inside") == false, "读档后已退出洞天（v1 行为不变）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
