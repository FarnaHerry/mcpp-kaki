# 渡劫 v2：三灾齐至（雷/火/风全程并发）+ 天罚使 Boss 考验（斩之渡劫成）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _hp_before := 0.0
var _hp_after := 0.0
var _hp_last := 0.0
var _hp_dips := 0

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _p():
	return root.find_child("Player", true, false)

func _cs():
	var p = _p()
	return p.call("get_cultivation") if p != null else null

func _tc():
	return root.find_child("TribulationController", true, false)

func _boss():
	return root.find_child("TribulationBoss", true, false)

func _boss_bar_name() -> String:
	var hud = root.find_child("GameHUD", true, false)
	return String(hud.call("get_boss_bar_name")) if hud != null else ""

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	var cs = _cs()
	if cs == null:
		return false

	match _step:
		0:
			# 大乘圆满 + 免门槛 → 请求机缘突破（=渡劫事件）
			cs.call("set_realm", 8)
			cs.call("set_spiritual_energy", 999999999)
			cs.call("set_free_breakthrough", true)
			_step = 1
		1:
			var bus = root.find_child("SignalBus", true, false)
			bus.emit_signal("breakthrough_requested")
			_step = 2
		2, 3, 4, 5, 6:
			# 推进 intro overlay（4 行）
			_press("interact")
			_step += 1
		7:
			_press("interact")
			var tc = _tc()
			_check(tc != null, "渡劫秘境：TribulationController 已启动")
			_step = 8
		8:
			_check(int(cs.call("get_realm_index")) == 9, "进入渡劫过渡态 realm=9")
			var tc = _tc()
			_check(tc != null and tc.call("is_boss_alive") == true, "天罚使已降临（is_boss_alive）")
			var b = _boss()
			_check(b != null, "天罚使节点存在（TribulationBoss）")
			if b != null:
				_check(String(b.get("display_name")) == "天罚使", "Boss 名=天罚使")
				_check(int(b.get("realm")) == 9, "天罚使 realm 与玩家同境=9（威压不可慑服）")
				_check(b.get("no_drops") == true, "天罚使 no_drops")
				_check(float(b.get("max_health")) == 2500.0, "天罚使 HP=2500（显式值压过×5补偿）")
			_step = 9
		9:
			_check(_boss_bar_name() == "天罚使", "HUD Boss 血条=天罚使")
			_check(_p().call("is_input_inverted") == true, "风灾并发：进场即控制反转")
			_hp_before = float(_p().call("get_current_health"))
			_hp_last = _hp_before
			_hp_dips = 0
			_step = 10
		10, 11, 12, 13, 14, 15:
			# 挂机：阴火 DoT + 风蚀持续掉血（不动躲雷也可能中雷，只断言有环境伤害）
			# 逐帧采样掉血次数——受击养肉身途中升 1 级（+3% 上限并回填 ~60HP）会掩盖首尾净额对比
			var hp_now = float(_p().call("get_current_health"))
			if hp_now < _hp_last - 0.001:
				_hp_dips += 1
			_hp_last = hp_now
			_step += 1
		16:
			_hp_after = float(_p().call("get_current_health"))
			_check(_hp_dips >= 1 or _hp_after < _hp_before, "三灾并发：挂机持续掉血（dips=" + str(_hp_dips) + ", " + str(_hp_before) + "→" + str(_hp_after) + "）")
			# 落雷视觉在场（雷灾并发证据）
			var tc = _tc()
			var has_bolt := false
			if tc != null:
				for c in tc.get_children():
					if c is Polygon2D:
						has_bolt = true
			_check(has_bolt, "雷灾并发：场上有落雷预警/雷柱")
			_step = 17
		17:
			# 斩天罚使 → 渡劫成
			var b = _boss()
			if b != null:
				b.call("take_damage", 999999.0, null)
			_step = 18
		18, 19:
			_step += 1
		20:
			_check(int(cs.call("get_realm_index")) == 10, "斩天罚使→渡劫成功→真仙 realm=10")
			_check(not _p().call("is_input_inverted") == true, "渡劫毕：控制反转已还原")
			_step = 21
		21, 22, 23, 24:
			# 推进 outro overlay
			_press("interact")
			_step += 1
		25:
			_press("interact")
			_step = 26
		26:
			# ---- 失败路径：再入渡劫，战死 → 退回大乘 + 天罚使清场 + 效果还原 ----
			cs.call("set_realm", 8)
			cs.call("set_spiritual_energy", 999999999)
			var bus2 = root.find_child("SignalBus", true, false)
			bus2.emit_signal("breakthrough_requested")
			_step = 27
		27, 28, 29, 30, 31:
			_press("interact") # 推进 intro
			_step += 1
		32:
			_press("interact")
			var tc2 = _tc()
			_check(tc2 != null and tc2.call("is_boss_alive") == true, "再入渡劫：天罚使再临")
			_step = 33
		33:
			var p2 = _p()
			p2.call("take_damage", 999999.0, null) # 战死
			_step = 34
		34, 35, 36, 37:
			_step += 1
		38:
			_check(int(cs.call("get_realm_index")) == 8, "渡劫战死→退回大乘 realm=8")
			_check(_boss() == null, "渡劫战死→天罚使已清场")
			_check(_tc() == null, "渡劫战死→TribulationController 已撤")
			_check(_p().call("is_input_inverted") == false, "渡劫战死→控制反转已还原")
			_step = 39
		39:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
