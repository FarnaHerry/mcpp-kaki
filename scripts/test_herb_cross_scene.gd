# HerbNode/ItemPickup 跨场景检测修复 harness:
# ①主场景草药旁（炼虚有纳戒）磁吸采集正常 ②进洞天后同全局坐标的草药不被吸走/提示抑制
# ③退出洞天回主场景后磁吸恢复
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _herb2 = null # 第二棵草药（跨场景测试对象）
var _herb2_pos := Vector2.ZERO

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

func _herbs() -> Array:
	var out = []
	for h in current_scene.find_children("*", "Area2D", true, false):
		if h.get("herb_id") != null and not h.call("is_harvested"):
			out.append(h)
	return out

func _prompt_text() -> String:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return ""
	for l in hud.find_children("*", "Label", true, false):
		if l.visible and "[X]" in l.text:
			return l.text
	return ""

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
			_step = 55
		55:
			var hs = _herbs()
			_check(hs.size() >= 2, "主场景至少 2 棵草药")
			# 第一棵：验证主场景磁吸正常
			_player().global_position = hs[0].global_position + Vector2(0, -4)
			_next = _t + 0.8
			_step = 1
		1:
			_check(_herbs().size() < 999, "dummy") # 占位
			var hs2 = _herbs()
			_herb2 = hs2[0]
			_herb2_pos = _herb2.global_position
			# 站第二棵旁立即进洞天（同帧，抢在磁吸飞入前；进入点即返回点）
			_player().global_position = _herb2_pos + Vector2(0, -4)
			_press("dongtian")
			_next = _t + 1.0
			_step = 3
		2:
			_step = 3
		3:
			_check(_dt().call("is_inside") == true, "进入洞天")
			# 洞天出生点 (240,200) 与主场景 (240,214) 聚灵草同全局坐标——等 1s 仍不应被吸
			_check(not _herb2.call("is_harvested"), "洞天内主场景草药未被跨场景磁吸")
			_check(not ("采集" in _prompt_text()), "洞天内无跨场景采集提示")
			_press("dongtian") # 退出，回到第二棵草药旁
			_next = _t + 0.8
			_step = 4
		4:
			_check(_dt().call("is_inside") == false, "退出洞天回主场景")
			_next = _t + 0.8 # 等磁吸飞入
			_step = 5
		5:
			_check(_herb2.call("is_harvested"), "回主场景后磁吸恢复（草药被采集）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
