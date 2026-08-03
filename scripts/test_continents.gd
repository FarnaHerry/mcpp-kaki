# H1 harness: ①当前洲识别/洲列表 ②境界门控 ③旅行（场景切换+状态保留）
#      ④旅行往返 ⑤跨洲读档（检查点在别洲→旅行桥切场景）
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
			_check(_cm() != null, "ContinentManager 存在")
			_check(String(_cm().call("get_current_id")) == "dongsheng", "当前洲=东胜神洲")
			_check(String(_cm().call("get_current_name")) == "东胜神洲", "洲名")
			var list = _cm().call("get_continent_list")
			_check(list.size() == 4, "四大部洲注册")
			var locked_count = 0
			for c in list:
				if not bool(c.get("unlocked")):
					locked_count += 1
			_check(locked_count == 3, "凡人期三洲未解锁")
			_check(not _cm().call("can_travel", "xiniuhe"), "凡人不可去西牛贺洲")
			_check(not _cm().call("travel_to", "xiniuhe"), "travel_to 拒绝未解锁")
			_step = 1
		1:
			_breakthrough_to(3) # 金丹
			_player().call("pickup_item", "healing_pill", 3)
			_gm().call("increment_kill_count")
			_check(_cm().call("can_travel", "xiniuhe"), "金丹可去西牛贺洲")
			_check(not _cm().call("can_travel", "dongsheng"), "不可去当前洲")
			_check(_cm().call("travel_to_direct", "xiniuhe"), "travel_to_direct 受理")
			_next = _t + 1.5
			_step = 2
		2:
			_check(_scene_path() == "res://scenes/continents/xiniuhe.tscn", "场景已切到西牛贺洲")
			_check(_player() != null, "新场景玩家存在")
			_next = _t + 0.8 # 等桥应用
			_step = 3
		3:
			_check(int(_cult().call("get_realm_index")) == 3, "境界保留（金丹）")
			_check(int(_player().call("get_inventory").call("get_item_count", "healing_pill")) == 3, "背包保留（止血丹×3）")
			_check(int(_gm().call("get_kill_count")) == 1, "击杀数保留")
			_check(String(_cm().call("get_current_id")) == "xiniuhe", "当前洲=西牛贺洲")
			var pos = _player().global_position
			print("[TEST] arrive pos: ", pos)
			_check(abs(pos.x - 150.0) < 5.0, "落点=洲 spawn")
			_gm().call("save_game", "auto") # 存一份检查点在西牛贺洲的档
			_check(_cm().call("travel_to_direct", "dongsheng"), "返程受理")
			_next = _t + 1.5
			_step = 4
		4:
			_check(_scene_path() == "res://scenes/main.tscn", "返回东胜神洲")
			_next = _t + 0.8
			_step = 5
		5:
			_check(int(_cult().call("get_realm_index")) == 3, "返程境界保留")
			_check(String(_cm().call("get_current_id")) == "dongsheng", "当前洲=东胜神洲")
			# 跨洲读档：auto 档的检查点在西牛贺洲 → load 应切场景回去
			_gm().call("load_game", "auto")
			_next = _t + 1.5
			_step = 6
		6:
			_check(_scene_path() == "res://scenes/continents/xiniuhe.tscn", "跨洲读档切回西牛贺洲")
			_next = _t + 0.8
			_step = 7
		7:
			_check(int(_cult().call("get_realm_index")) == 3, "读档境界正确")
			_check(int(_player().call("get_inventory").call("get_item_count", "healing_pill")) == 3, "读档背包正确")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
