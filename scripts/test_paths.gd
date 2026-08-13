# 元婴分叉（肉身成圣/元神修炼）：喂养/等级/加成/focus 跟随/合体汇合/存档字段/功法页分区
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

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _press_key(code: int):
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

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

func _scan(n: Node, s: String) -> bool:
	if n is Label and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _scan(c, s):
			return true
	return false

func _has_label(s: String) -> bool:
	var menu = root.find_child("GameMenu", true, false)
	return menu != null and _scan(menu, s)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.25

	var cs = _cs()
	if cs == null:
		return false

	match _step:
		0:
			# 元婴前喂养无效
			cs.call("feed_path", 0, 500.0)
			cs.call("feed_path", 1, 500.0)
			_check(int(cs.call("get_path_body_level")) == 0, "元婴前喂养无效（body=0）")
			_check(int(cs.call("get_path_spirit_level")) == 0, "元婴前喂养无效（spirit=0）")
			cs.call("set_realm", 4) # 元婴
			_step = 1
		1:
			_check(int(cs.call("get_realm_index")) == 4, "境界=元婴")
			cs.call("feed_path", 0, 150.0) # 肉身 1 级（100/级）
			cs.call("feed_path", 1, 250.0) # 元神 2 级
			_check(int(cs.call("get_path_body_level")) == 1, "肉身 150 经验 = 1 级")
			_check(int(cs.call("get_path_spirit_level")) == 2, "元神 250 经验 = 2 级")
			_step = 2
		2:
			_check(abs(float(cs.call("get_path_atk_mult")) - 1.03) < 0.001, "肉身1级 物攻+3%")
			_check(abs(float(cs.call("get_path_spell_mult")) - 1.06) < 0.001, "元神2级 法强+6%")
			_check(abs(float(cs.call("get_path_law_mult")) - 1.10) < 0.001, "元神2级 法则回复+10%")
			_check(abs(float(cs.call("get_path_tribulation_resist")) - 0.08) < 0.001, "肉身1级 三灾减伤8%")
			_check(int(cs.call("get_focus")) == 2, "focus 自动跟随高侧=元神(2)")
			_step = 3
		3:
			# 肉身防御/生命乘区（get_max_health = 100 × def_mult，def 含肉身加成）
			var base_hp = 100.0 * float(cs.call("get_defense_multiplier"))
			_check(base_hp > 0.0, "生命上限读取: " + str(base_hp))
			# 5 级封顶
			cs.call("feed_path", 0, 9999.0)
			_check(int(cs.call("get_path_body_level")) == 5, "肉身喂满封顶 5 级")
			_check(abs(float(cs.call("get_path_tribulation_resist")) - 0.40) < 0.001, "肉身5级 三灾减伤40%")
			_step = 4
		4:
			# 合体「形神合一」：弱侧补 80% 差值（body 500，spirit 250 → spirit += 200）
			cs.call("set_realm", 7)
			_check(bool(cs.call("is_path_merged")), "合体汇合标记 is_path_merged")
			var se = float(cs.call("get_path_spirit_exp"))
			_check(abs(se - 450.0) < 0.5, "弱侧补 80% 差值（250→450）: " + str(se))
			_step = 5
		5:
			# 汇合后双轨同步喂养（肉身已封顶 500 不再涨，元神同步+10）
			cs.call("feed_path", 0, 10.0)
			_check(abs(float(cs.call("get_path_body_exp")) - 500.0) < 0.5, "汇合后肉身同步（封顶 500）")
			var se2 = float(cs.call("get_path_spirit_exp"))
			_check(abs(se2 - 460.0) < 0.5, "汇合后元神同步+10: " + str(se2))
			_step = 6
		6:
			# 存档字段
			var gm = root.find_child("GameManager", true, false)
			var data = gm.call("collect_save_data")
			var cd: Dictionary = data.get("cultivation", {})
			_check(cd.has("path_body") and cd.has("path_spirit") and cd.has("path_merged"), "存档含分叉三字段")
			_check(bool(cd.get("path_merged", false)), "存档 path_merged=true")
			_step = 7
		7:
			# 功法页分叉分区（ESC → E×2 到功法页）
			_press("menu")
			_step = 8
		8:
			_press_key(KEY_E)
			_step = 9
		9:
			_press_key(KEY_E)
			_step = 10
		10:
			_check(_has_label("—— 功法 ——"), "功法页打开")
			_check(_has_label("形神合一"), "分叉分区显示（已汇合→形神合一）")
			_check(_has_label("肉身成圣"), "肉身行显示")
			_check(_has_label("元神修炼"), "元神行显示")
			_step = 11
		11:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
