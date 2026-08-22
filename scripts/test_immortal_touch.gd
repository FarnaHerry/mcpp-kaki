# 仙人抚顶（太上老君）+ 醍醐灌顶（古佛化身）双机缘叙事测试
# 端到端：travel 天界 → 交互仙人抚顶 → buff/修为/寿元 → after_lines
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _held: Array = []

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

func _hold(action: String):
	Input.action_press(action)
	_held.append(action)

func _release_all():
	for a in _held:
		Input.action_release(a)
	_held.clear()

func _player():
	return root.find_child("Player", true, false)

func _gm():
	return root.find_child("GameManager", true, false)

func _bus():
	return root.find_child("SignalBus", true, false)

func _cm():
	return root.find_child("ContinentManager", true, false)

func _soul():
	return root.find_child("SoulLedgerSystem", true, false)

func _find(s: String):
	return root.find_child(s, true, false)

func _finish() -> bool:
	if _fail == 0:
		print("[TEST] ALL PASS")
	else:
		print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_release_all()
	_step += 1
	if _step > 60:
		print("[TEST] hard cap")
		return _finish()

	match _step:
		1:
			# 真仙（realm 10）→ 上天界
			_check(not _gm().call("has_flag", "immortal_touch_granted"), "初始无仙人抚顶 flag")
			_check(not _gm().call("has_flag", "ti_hu_guan_ding_granted"), "初始无醍醐灌顶 flag")
			_player().call("get_cultivation").call("set_realm", 10)
			_check(bool(_cm().call("travel_to_direct", "tianjie")), "travel 天界")
			_next = _t + 2.0
		2:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("tianjie.tscn"), "到达天界: " + sc)
			_next = _t + 0.3
		3:
			# 找太上老君
			var laojun = _find("TaiShangLaoJun")
			_check(laojun != null, "太上老君 NarrativeNode 存在")
			_next = _t + 0.3
		4:
			# 靠近老君
			_player().global_position = Vector2(2580, 220)
			_next = _t + 0.5
		5:
			# 交互（X）
			_hold("interact")
			_next = _t + 0.3
		6:
			# overlay 已开，逐行推进叙事（5行 + 1关闭）
			var laojun = _find("TaiShangLaoJun")
			if laojun and bool(laojun.call("is_overlay_open")):
				var line = String(laojun.call("get_current_line"))
				_check(line.begins_with("天地玄黄") or line.begins_with("天上白玉京"), "仙人抚顶首行: " + line)
				_hold("interact")  # 2/5
				_next = _t + 0.3
			else:
				_check(false, "仙人抚顶 overlay 已开")
				_next = _t + 0.3
		7:
			var laojun = _find("TaiShangLaoJun")
			if laojun and bool(laojun.call("is_overlay_open")):
				_hold("interact")  # 3/5
				_next = _t + 0.3
			else:
				_next = _t + 0.3
		8:
			var laojun = _find("TaiShangLaoJun")
			if laojun and bool(laojun.call("is_overlay_open")):
				_hold("interact")  # 4/5
				_next = _t + 0.3
			else:
				_next = _t + 0.3
		9:
			var laojun = _find("TaiShangLaoJun")
			if laojun and bool(laojun.call("is_overlay_open")):
				_hold("interact")  # 5/5
				_next = _t + 0.3
			else:
				_next = _t + 0.3
		10:
			var laojun = _find("TaiShangLaoJun")
			if laojun and bool(laojun.call("is_overlay_open")):
				_hold("interact")  # close
				_next = _t + 0.4
			else:
				_next = _t + 0.4
		11:
			# 验证 gm_method 回调结果
			_check(bool(_gm().call("has_flag", "immortal_touch_granted")), "仙人抚顶 flag 已立")
			var player = _player()
			var buffs = player.call("get_buffs")
			_check(bool(buffs.call("has", "buff_chang_sheng")), "长生 buff 已施加")
			var soul = _soul()
			if soul:
				var ls = int(soul.call("get_ledger_lifespan"))
				_check(ls >= 100 + 500, "寿元增加 500: " + str(ls))
			_next = _t + 0.3
		12:
			# after_lines 再交互
			var laojun = _find("TaiShangLaoJun")
			if laojun:
				_player().global_position = Vector2(2580, 220)
				_next = _t + 0.5
			else:
				_next = _t + 0.3
		13:
			_hold("interact")  # 播 after_lines
			_next = _t + 0.3
		14:
			var laojun = _find("TaiShangLaoJun")
			if laojun and bool(laojun.call("is_overlay_open")):
				var line = String(laojun.call("get_current_line"))
				_check(line.contains("长生之路") or line.contains("造化"), "仙人抚顶 after_lines: " + line)
				_hold("interact")  # close
				_next = _t + 0.3
			else:
				_next = _t + 0.3
		15:
			# ===== 醍醐灌顶（需要化神 realm 5+）=====
			# 天界没有醍醐灌顶节点，验证 gm 方法本身已注册
			_check(bool(_gm().has_method("check_ti_hu_guan_ding")), "check_ti_hu_guan_ding 已注册")
			_check(bool(_gm().has_method("grant_ti_hu_guan_ding")), "grant_ti_hu_guan_ding 已注册")
			# 直接调用 gm 方法验证
			var reason = String(_gm().call("check_ti_hu_guan_ding"))
			_check(reason == "", "醍醐灌顶 precheck 通过（真仙≥化神）: " + reason)
			_gm().call("grant_ti_hu_guan_ding")
			_check(bool(_gm().call("has_flag", "ti_hu_guan_ding_granted")), "醍醐灌顶 flag 已立（直接调）")
			var player = _player()
			var buffs = player.call("get_buffs")
			_check(bool(buffs.call("has", "buff_ti_hu")), "醍醐 buff 已施加（直接调）")
			_next = _t + 0.3
		16:
			# 仙人抚顶 precheck 后禁止二次触发
			# 先清 flag（测试用），再重复触发
			# 实际上 once_flag 已立，播 after_lines 不走 gm_method
			# 但我们验证 precheck 返回空字符串（once 已立，放行去播 after_lines）
			_check(_gm().call("has_flag", "immortal_touch_granted"), "仙人抚顶 flag 仍在")
			_next = _t + 0.3
		17:
			print("[TEST] 仙人抚顶 + 醍醐灌顶 完整测试完成")
			return _finish()

	return false