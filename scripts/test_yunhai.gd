# H4 harness: 云海关卡机制
# ①travel_to 先入云海+travel_dest 落档 ②罡风推移 ③落雷伤害 ④坠海遣返
# ⑤渡海中途读档目的地不丢 ⑥登岸 complete_travel
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _hp0 := 0.0
var _wind_x0 := 0.0

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

func _cult():
	return _player().call("get_cultivation")

func _cm():
	return root.find_child("ContinentManager", true, false)

func _gm():
	return root.find_child("GameManager", true, false)

func _scene_path() -> String:
	return str(current_scene.scene_file_path) if current_scene else ""

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
	paused = false

	match _step:
		0:
			_breakthrough_to(3) # 金丹
			_check(_cm().call("travel_to", "xiniuhe"), "travel_to 受理（云海强渡）")
			_next = _t + 1.5
			_step = 1
		1:
			_check(_scene_path() == "res://scenes/continents/yunhai.tscn", "先入云海")
			_check(String(_gm().call("get_travel_dest")) == "xiniuhe", "travel_dest=西牛贺洲")
			_check(_player() != null, "云海玩家存在")
			# 起云台承托：自由落体后应停在 y≈200
			_next = _t + 0.8
			_step = 2
		2:
			var pos = _player().global_position
			print("[TEST] spawn rest pos: ", pos)
			_check(pos.y > 170.0 and pos.y < 215.0, "起云台承托")
			# ②罡风：站进风带(700)，不按键，应被西推
			_player().global_position = Vector2(700, 110)
			_wind_x0 = 700.0
			_next = _t + 0.5
			_step = 3
		3:
			var wx = _player().global_position.x
			print("[TEST] wind pushed x: ", wx)
			_check(wx < _wind_x0 - 15.0, "罡风西推")
			# ③落雷：站雷柱(500)下等劈落（周期4s内必中）
			_hp0 = float(_player().call("get_current_health"))
			_player().global_position = Vector2(500, 110)
			_next = _t + 4.6
			_step = 4
		4:
			var hp1 = float(_player().call("get_current_health"))
			print("[TEST] hp before/after bolt: ", _hp0, " -> ", hp1)
			_check(hp1 < _hp0, "落雷劈中扣血")
			# ④坠海遣返
			_hp0 = hp1
			_player().global_position = Vector2(1200, 500)
			_next = _t + 0.6
			_step = 5
		5:
			var pos2 = _player().global_position
			print("[TEST] after fall: ", pos2)
			_check(abs(pos2.x - 60.0) < 30.0 and pos2.y < 220.0, "坠海遣返起云台")
			_check(float(_player().call("get_current_health")) < _hp0, "坠海扣血代价")
			# ⑤渡海中途存档+读档，目的地不丢
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_next = _t + 1.5
			_step = 6
		6:
			_check(_scene_path() == "res://scenes/continents/yunhai.tscn", "读档仍在云海")
			_check(String(_gm().call("get_travel_dest")) == "xiniuhe", "读档 travel_dest 不丢")
			# ⑥登岸
			_player().global_position = Vector2(2300, 170)
			_next = _t + 1.5
			_step = 7
		7:
			_check(_scene_path() == "res://scenes/continents/xiniuhe.tscn", "登岸到西牛贺洲")
			_check(String(_gm().call("get_travel_dest")) == "", "travel_dest 到岸即清")
			_next = _t + 0.8
			_step = 8
		8:
			_check(int(_cult().call("get_realm_index")) == 3, "境界保留（金丹）")
			_check(String(_cm().call("get_current_id")) == "xiniuhe", "当前洲=西牛贺洲")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
