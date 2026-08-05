# 地府/生死簿/勾魂 测试：
# ①生死簿数据 ②寿元随境界+进地府 ③判官查簿 ④还阳
# ⑤濒死刷黑白无常 ⑥反杀得修为 ⑦勾魂死亡入地府 ⑧秦广王审判 ⑨改簿划名
# ⑩还阳 ⑪划名豁免（不再被勾魂）⑫免死原地复活
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _inspect_data := {}
var _inspect_show := false
var _last_lifespan := []
var _energy_before := 0
var _connected_bus = null

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	_reconnect()
	print("[TEST] main scene loaded")

func _reconnect():
	# 场景切换会新建 SignalBus（WC.setup 每场景一个），需重连
	var bus = current_scene.find_child("SignalBus", true, false) if current_scene else null
	if bus and bus != _connected_bus:
		if _connected_bus:
			_connected_bus.disconnect("ledger_inspect_requested", Callable(self, "_on_ledger_inspect"))
			_connected_bus.disconnect("lifespan_changed", Callable(self, "_on_lifespan"))
		bus.connect("ledger_inspect_requested", Callable(self, "_on_ledger_inspect"))
		bus.connect("lifespan_changed", Callable(self, "_on_lifespan"))
		_connected_bus = bus

func _on_ledger_inspect(data, show):
	_inspect_data = data
	_inspect_show = show

func _on_lifespan(ledger, actual):
	_last_lifespan = [int(ledger), int(actual)]

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _player():
	return root.find_child("Player", true, false)

func _soul():
	return root.find_child("SoulLedgerSystem", true, false)

func _find(s: String):
	return root.find_child(s, true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	_reconnect()
	if _step > 70:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			var sl = _soul()
			_check(sl != null, "SoulLedgerSystem 存在")
			_check(int(sl.call("get_ledger_lifespan")) == 100, "簿上寿元=100（凡人）")
			_check(int(sl.call("get_actual_lifespan")) == 100, "凡人实际寿元=100")
			_check(not sl.call("is_struck"), "初始未划名")
		2:
			# 寿元随境界 + 直接进地府（入口已移到长安，见 test_nanzhanbu）
			var p = _player()
			p.call("get_cultivation").call("set_realm", 1) # 炼气
			_check(int(_soul().call("get_actual_lifespan")) == 150, "炼气实际寿元=150")
			_check(_last_lifespan == [100, 150], "lifespan_changed 信号 (簿上/实际)")
			var gm = root.find_child("GameManager", true, false)
			gm.call("enter_difu")
			_next = _t + 1.5
		3:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("difu.tscn"), "进入地府场景: " + sc)
			var p = _player()
			_check(p.call("get_current_health") >= p.call("get_max_health") - 0.01, "地府满血")
			_check(_find("Panguan") != null, "判官存在")
			p.position = Vector2(150, 190)
			_next = _t + 0.5
		4:
			Input.action_press("interact")
		5:
			Input.action_release("interact")
			_check(_inspect_show, "判官查簿 overlay 显示")
			_check(String(_inspect_data.get("original_body", "")) == "凡人", "原身=凡人")
			_check(int(_inspect_data.get("actual_lifespan", 0)) == 150, "实际寿元=150（炼气）")
			# 还阳
			var gate = _find("HuanYangGate")
			_check(gate != null, "还阳门存在")
			_player().position = Vector2(445, 195)
			_next = _t + 0.5
		6:
			Input.action_press("up")
		7:
			Input.action_release("up")
			_next = _t + 1.5
		8:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("main.tscn"), "还阳回主场景: " + sc)
			# 勾魂 spawn（尚未划名）
			var p = _player()
			p.call("set_current_health", p.call("get_max_health") * 0.1)
			_next = _t + 0.4
		9:
			var a = _find("黑无常")
			var b = _find("白无常")
			_check(a != null and b != null, "濒死黑白无常出现（未划名）")
			if a:
				_check(a.call("get_is_soul_reaper"), "黑无常 is_soul_reaper")
			# 反杀
			_energy_before = int(_player().call("get_cultivation").call("get_spiritual_energy"))
			if a: a.call("take_damage", 99999.0, _player())
			if b: b.call("take_damage", 99999.0, _player())
			_next = _t + 0.6
		10:
			var after = int(_player().call("get_cultivation").call("get_spiritual_energy"))
			_check(after - _energy_before >= 30, "反杀修为 +%d (≥30)" % (after - _energy_before))
			# 重新刷无常 → 被勾魂
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			_next = _t + 0.8
		11:
			var p = _player()
			p.call("set_current_health", p.call("get_max_health") * 0.1)
			_next = _t + 0.4
		12:
			var a = _find("黑无常")
			_check(a != null, "第二次黑白无常出现")
			var p = _player()
			p.call("set_last_damage_source", a)
			p.call("set_current_health", 0.0)
			p.emit_signal("player_died")
			_next = _t + 2.2
		13:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("difu.tscn"), "勾魂死亡魂魄入地府: " + sc)
			# 秦广王审判
			var qin = _find("QinGuangWang")
			_check(qin != null, "秦广王存在")
			_player().position = Vector2(230, 190)
			_next = _t + 0.5
		14:
			Input.action_press("interact")
		15:
			Input.action_release("interact")
			_check(bool(_inspect_data.get("trial", false)), "秦广王审判 overlay")
			_check(String(_inspect_data.get("origin", "")) != "", "审判含出身")
			# 改簿划名
			var book = _find("ShengSiBo")
			_check(book != null, "生死簿存在")
			_player().position = Vector2(330, 190)
			_next = _t + 0.5
		16:
			Input.action_press("interact")
		17:
			Input.action_release("interact")
			_check(_soul().call("has_soul_protection"), "改簿划名 → 免死 on")
			_check(_soul().call("is_struck"), "划名 → 阴寿豁免标记")
			# 还阳
			var gate = _find("HuanYangGate")
			_player().position = Vector2(445, 195)
			_next = _t + 0.5
		18:
			Input.action_press("up")
		19:
			Input.action_release("up")
			_next = _t + 1.5
		20:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("main.tscn"), "还阳回主场景: " + sc)
			# 划名豁免：濒死不再刷勾魂使
			var p = _player()
			p.call("set_current_health", p.call("get_max_health") * 0.1)
			_next = _t + 1.0
		21:
			_check(_find("黑无常") == null and _find("白无常") == null, "划名后濒死不再被勾魂")
			# 免死：触发死亡 → 原地复活（消耗免死，划名保留）
			var p = _player()
			p.call("set_current_health", 1.0)
			p.emit_signal("player_died")
			_next = _t + 0.6
		22:
			var p = _player()
			_check(p.call("get_current_health") > 1.0, "免死原地复活")
			_check(not _soul().call("has_soul_protection"), "免死标记已消耗")
			_check(_soul().call("is_struck"), "划名豁免保留")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
