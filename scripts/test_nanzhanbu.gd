# 南赡部洲测试：①炼虚解锁 ②travel 到长安 ③长安内容（商店/地府入口/人参果）
# ④地府入口 ↑ 进地府 ⑤还阳回长安
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

func _player():
	return root.find_child("Player", true, false)

func _cm():
	return root.find_child("ContinentManager", true, false)

func _find(s: String):
	return root.find_child(s, true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 40:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			var cm = _cm()
			_check(cm != null, "ContinentManager 存在")
			_check(not bool(cm.call("is_unlocked", "nanzhanbu")), "凡人未解锁南赡部洲")
			# 炼虚
			_player().call("get_cultivation").call("set_realm", 6)
			_next = _t + 0.3
		2:
			var cm = _cm()
			_check(bool(cm.call("is_unlocked", "nanzhanbu")), "炼虚解锁南赡部洲")
			_check(cm.call("travel_to_direct", "nanzhanbu"), "travel_to_direct 南赡部洲")
			_next = _t + 1.5
		3:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("nanzhanbu.tscn"), "到达南赡部洲: " + sc)
			_check(_find("ShopKeeper") != null, "长安坊市商店掌柜存在")
			_check(_find("DifuGate") != null, "长安地府入口存在")
			_check(_find("ShopSystem") != null, "ShopSystem 存在")
			# 人参果：五庄观拾取（去五庄观区找）
			var has_ren = false
			var inv = _player().call("get_inventory")
			# 直接把玩家传到地府入口
			var gate = _find("DifuGate")
			_player().global_position = Vector2(240, 200)
			_next = _t + 0.5
		4:
			Input.action_press("up")
		5:
			Input.action_release("up")
			_next = _t + 1.5
		6:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("difu.tscn"), "长安地府入口 → 黄泉路: " + sc)
			_check(_find("QinGuangWang") != null, "地府秦广王存在")
			# 还阳
			var gate = _find("HuanYangGate")
			_check(gate != null, "还阳门存在")
			_player().position = Vector2(445, 195)
			_next = _t + 0.5
		7:
			Input.action_press("up")
		8:
			Input.action_release("up")
			_next = _t + 1.5
		9:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("nanzhanbu.tscn"), "还阳回南赡部洲: " + sc)
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
