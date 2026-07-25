# 威压/灵压 harness: ①威压慑服低阶 ②灵压伤害 ③镇杀(gap≥4)
#      ④护佑反弹(高阶在场) ⑤耗灵冷却 ⑥无目标提示
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _hp_ref := 0.0 # cross-step hp reference

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

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _enemies_alive() -> Array:
	var arr = []
	for e in root.get_tree().get_nodes_in_group("enemies"):
		if not e.is_queued_for_deletion() and float(e.get("current_health")) > 0.0:
			arr.append(e)
	return arr

func _count_suppressed() -> int:
	var n = 0
	for e in root.get_tree().get_nodes_in_group("enemies"):
		if not e.is_queued_for_deletion() and e.modulate.r < 0.99:
			n += 1
	return n

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			# 清除所有敌人
			for e in root.get_tree().get_nodes_in_group("enemies"):
				e.queue_free()
			_next = _t + 0.5
			_step = 1
		1:
			var WC = load("res://scripts/world_common.gd")
			WC.spawn_enemy(current_scene, Vector2(200, 210), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0, "TestMob").set("realm", 0)
			_next = _t + 0.5
			_step = 2
		2:
			# ①凡人无灵力
			_check(not _player().call("cast_wei_pressure"), "凡人无灵力威压失败")
			_breakthrough_to(1)
			_step = 3
		3:
			_check(int(_cult().call("get_realm_index")) == 1, "达到炼气")
			_cult().call("restore_mana", 999.0)
			_check(_player().call("cast_wei_pressure"), "炼气威压 realm=0 成功")
			_next = _t + 0.5
			_step = 4
		4:
			_check(_count_suppressed() > 0, "敌人被慑服变灰")
			_check(_player().call("get_wei_cooldown_left") > 0.0, "威压冷却中")
			_check(not _player().call("cast_wei_pressure"), "冷却期拒发")
			_next = _t + 8.5
			_step = 5
		5:
			_check(_player().call("get_wei_cooldown_left") <= 0.0, "威压冷却结束")
			# ②灵压 gap=1 无效（炼气 vs realm0）
			_check(not _player().call("cast_lin_pressure"), "灵压 gap<2 无效")
			_breakthrough_to(2)
			_step = 6
		6:
			if _player().call("get_lin_cooldown_left") > 0.0:
				_next = _t + 15.5
			else:
				_step = 7
		7:
			_check(int(_cult().call("get_realm_index")) == 2, "达到筑基")
			_cult().call("restore_mana", 999.0)
			_check(_player().call("cast_lin_pressure"), "筑基灵压 gap=2 有效")
			_next = _t + 0.5
			_step = 8
		8:
			# ③护佑：spawn realm=3 guard + realm=0 low
			for e in root.get_tree().get_nodes_in_group("enemies"):
				e.queue_free()
			_next = _t + 0.5
			_step = 9
		9:
			# 等灵压冷却
			if _player().call("get_lin_cooldown_left") > 0.0:
				_next = _t + 15.5
			else:
				_step = 10
		10:
			var WC = load("res://scripts/world_common.gd")
			var guard = WC.spawn_enemy(current_scene, Vector2(200, 210), Color(0.8, 0.2, 0.8, 1), 60.0, 300.0, "GuardE")
			guard.set("realm", 3)
			guard.set("max_health", 20.0); guard.set("current_health", 20.0)
			var low = WC.spawn_enemy(current_scene, Vector2(215, 210), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0, "LowMob2")
			low.set("realm", 0)
			low.set("max_health", 10.0); low.set("current_health", 10.0)
			_next = _t + 0.5
			_step = 11
		11:
			_player().call("set_current_health", 100.0)
			_hp_ref = float(_player().call("get_current_health"))
			_cult().call("restore_mana", 999.0)
			_player().call("cast_lin_pressure")
			_next = _t + 0.5
			_step = 12
		12:
			var hp_now = float(_player().call("get_current_health"))
			_check(hp_now < _hp_ref, "护佑反弹: %.0f→%.0f" % [_hp_ref, hp_now])
			# ④镇杀：元婴(realm=4) vs realm=0 (gap=4)
			_breakthrough_to(4)
			_step = 13
		13:
			if _player().call("get_lin_cooldown_left") > 0.0:
				_next = _t + 15.5
			else:
				_step = 14
		14:
			_check(int(_cult().call("get_realm_index")) == 4, "达到元婴")
			# 清除守卫（护佑消除），保低阶存活用于镇杀测试
			for e in root.get_tree().get_nodes_in_group("enemies"):
				e.queue_free()
			_next = _t + 0.5
			_step = 15
		15:
			var WC = load("res://scripts/world_common.gd")
			var target = WC.spawn_enemy(current_scene, Vector2(200, 210), Color(0.9, 0.2, 0.2, 1), 60.0, 200.0, "ZhenShaTarget")
			target.set("realm", 0)
			target.set("max_health", 10.0); target.set("current_health", 10.0)
			_next = _t + 0.5
			_step = 16
		16:
			_cult().call("restore_mana", 999.0)
			_player().call("cast_lin_pressure")
			_next = _t + 0.5
			_step = 17
		17:
			var tgt = root.find_child("ZhenShaTarget", true, false)
			_check(tgt == null or float(tgt.get("current_health")) <= 0.0, "gap≥4 镇杀")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
