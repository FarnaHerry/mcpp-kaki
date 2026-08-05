# 寿元测试：境界寿元表 + 成仙后正常 + 仅天尊（三清级）无限
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

func _ledger():
	return root.find_child("SoulLedgerSystem", true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 20:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			var cs = _player().call("get_cultivation")
			cs.call("set_realm", 9) # 渡劫
			_next = _t + 0.3
		2:
			var l = int(_ledger().call("get_actual_lifespan"))
			_check(l == 50000, "渡劫寿元 50000 (实际=%d)" % l)
			_player().call("get_cultivation").call("set_realm", 10) # 真仙（成仙）
			_next = _t + 0.3
		3:
			var l = int(_ledger().call("get_actual_lifespan"))
			_check(l == 100000, "真仙寿元 100000（成仙后正常）")
			_player().call("get_cultivation").call("set_realm", 11) # 金仙
			_next = _t + 0.3
		4:
			var l = int(_ledger().call("get_actual_lifespan"))
			_check(l == 200000, "金仙寿元 200000")
			_player().call("get_cultivation").call("set_realm", 12) # 天尊（三清级）
			_next = _t + 0.3
		5:
			var l = int(_ledger().call("get_actual_lifespan"))
			_check(l < 0, "天尊（三清级）寿元无限 (实际=%d)" % l)
			# 簿上寿元仍为 100（信息差终点：簿上定数 vs 跳出五行）
			_check(int(_ledger().call("get_ledger_lifespan")) == 100, "簿上寿元仍 100")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
