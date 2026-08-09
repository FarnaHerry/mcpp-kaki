# 天界测试：①真仙解锁+travel_to_direct("tianjie") ②场景加载 ③蟠桃拾取+使用（回血100%/修为+5000）
# ④巨灵神 Boss 存在（realm 11）⑤回主洲（东胜神洲）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _hp_max := 0.0
var _energy0 := 0.0

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
	if _step > 60:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			# 真仙（realm 10）飞升后可入天界
			_player().call("get_cultivation").call("set_realm", 10)
			_next = _t + 0.3
		2:
			_check(bool(_cm().call("is_unlocked", "tianjie")), "真仙解锁天界")
			_check(bool(_cm().call("travel_to_direct", "tianjie")), "travel 天界（真仙腾云直达）")
			_next = _t + 1.5
		3:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("tianjie.tscn"), "到达天界: " + sc)
			# 蟠桃园拾取蟠桃：先压血到 50%（拾取自动用：回血+修为一并结算）
			var p = _player()
			_hp_max = float(p.call("get_max_health"))
			_energy0 = float(p.call("get_cultivation").call("get_current_energy"))
			p.call("set_current_health", _hp_max * 0.5)
			p.global_position = Vector2(2980, 230)
			_next = _t + 0.8
		4:
			var p = _player()
			var hp = float(p.call("get_current_health"))
			_check(hp >= _hp_max - 0.5, "蟠桃回血 100%% (%.0f/%.0f)" % [hp, _hp_max])
			var e1 = float(p.call("get_cultivation").call("get_current_energy"))
			_check(e1 >= _energy0 + 5000.0 - 0.5, "蟠桃修为 +5000 (%.0f → %.0f)" % [_energy0, e1])
			_next = _t + 0.3
		5:
			# 巨灵神 Boss 守关（realm 11 金仙级）
			var boss = _find("Boss_JuLingShen")
			_check(boss != null, "巨灵神 Boss 守关")
			if boss:
				_check(int(boss.get("realm")) == 11, "巨灵神 realm 11（金仙级）")
			# 回主洲（东胜神洲）
			_check(bool(_cm().call("travel_to_direct", "dongsheng")), "travel 回东胜神洲")
			_next = _t + 1.5
		6:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("main.tscn"), "回到东胜神洲: " + sc)
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
