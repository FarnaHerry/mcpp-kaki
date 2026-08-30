# 能力解锁表 harness（data/abilities.json + AbilityManager 数据驱动 + GameMenu 能力页）：
# ① abilities.json 装配（22 条=15 主动+7 被动，字段抽查）
# ② 初始解锁（冲刺/攀墙）+ 境界门控（炼气→二段跳/纳戒，金丹→滑翔/自主飞行，炼虚→开辟洞天）
# ③ 混元解锁（道域展开/化身千万）
# ④ 能力页文本来自数据表（锁定项显「名·条件」，解锁项显「✓名」）
# 注：一帧只按一键（同帧连按会被引擎 Input 去抖吞掉）；翻页走 _input 原始键码（Q/E）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _pending_release: Array = []

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

func _press(action: String):
	# 按住一帧再 release（同帧 press+release 对 action 轮询不可靠）
	Input.action_press(action)
	_pending_release.append(action)

func _press_key(code: int):
	# 翻页走 _input 原始键码（Q/E），不用 action；仅按下（同帧释放会吞掉 _input 派发）
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _am():
	return _player().call("get_ability_manager")

func _scan(n: Node, s: String) -> bool:
	if n is Label and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _scan(c, s):
			return true
	return false

func _has_menu_label(s: String) -> bool:
	var menu = root.find_child("GameMenu", true, false)
	return menu != null and _scan(menu, s)

func _find_row(list: Array, id: String) -> Dictionary:
	for r in list:
		if String(r.get("id", "")) == id:
			return r
	return {}

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _process(delta) -> bool:
	for a in _pending_release:
		Input.action_release(a)
	_pending_release.clear()
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			# 等 bootstrap call_deferred 装配完成
			if _player() == null:
				_next = _t + 0.3
				return false
			_next = _t + 0.5
			_step = 1
		1:
			# ① JSON 装配 + 数据表加载
			var dl = root.find_child("DataLoader", true, false)
			_check(dl != null, "DataLoader 存在")
			_check(dl != null and int(dl.call("get_all_abilities").size()) == 22, "abilities.json 22 条（DataLoader）")
			var list: Array = _am().call("get_ability_list")
			_check(list.size() == 22, "AbilityManager 能力表 22 条（15 主动+7 被动）")
			var dj = _find_row(list, "double_jump")
			_check(not dj.is_empty() and int(dj["unlock_realm"]) == 1 and String(dj["type"]) == "active",
				"二段跳：active / 炼气(1)")
			var sr = _find_row(list, "storage_ring")
			_check(not sr.is_empty() and int(sr["unlock_realm"]) == 1 and String(sr["type"]) == "passive",
				"纳戒：passive / 炼气(1)")
			var gf = _find_row(list, "giant_form")
			_check(not gf.is_empty() and int(gf["unlock_realm"]) == 11, "法天象地：金仙(11)")
			var dd = _find_row(list, "dao_domain")
			_check(not dd.is_empty() and bool(dd["hunyuan"]) and String(dd["cond"]) == "混元",
				"道域展开：混元解锁")
			var dash = _find_row(list, "dash")
			_check(not dash.is_empty() and bool(dash["innate"]) and String(dash["cond"]) == "初始",
				"冲刺：初始即会")
			var wc = _find_row(list, "wall_cling")
			_check(not wc.is_empty() and bool(wc["innate"]) and String(wc["type"]) == "passive",
				"攀墙：初始被动")
			_step = 2
		2:
			# ② 初始解锁状态（凡人期）
			_check(bool(_am().call("has_ability", "dash")), "初始：冲刺已解锁")
			_check(bool(_am().call("has_ability", "wall_cling")), "初始：攀墙已解锁")
			_check(not bool(_am().call("has_ability", "double_jump")), "凡人期：二段跳未解锁")
			_check(not bool(_am().call("has_ability", "dongtian")), "凡人期：开辟洞天未解锁")
			_breakthrough_to(1)
			_next = _t + 0.3
			_step = 3
		3:
			_check(int(_cult().call("get_realm_index")) == 1, "突破到炼气")
			_check(bool(_am().call("has_ability", "double_jump")), "炼气：二段跳解锁")
			_check(bool(_am().call("has_ability", "storage_ring")), "炼气：纳戒解锁")
			_check(not bool(_am().call("has_ability", "air_dash")), "炼气：空中冲刺仍未解锁")
			_breakthrough_to(3)
			_next = _t + 0.3
			_step = 4
		4:
			_check(int(_cult().call("get_realm_index")) == 3, "突破到金丹")
			_check(bool(_am().call("has_ability", "air_dash")), "金丹：空中冲刺解锁（筑基表项）")
			_check(bool(_am().call("has_ability", "short_flight")), "金丹：短暂飞行解锁（筑基表项）")
			_check(bool(_am().call("has_ability", "spirit_vision")), "金丹：灵视解锁（筑基表项）")
			_check(bool(_am().call("has_ability", "glide")), "金丹：滑翔解锁")
			_check(bool(_am().call("has_ability", "free_flight")), "金丹：自主飞行解锁")
			_check(not bool(_am().call("has_ability", "domain")), "金丹：领域展开仍未解锁")
			_breakthrough_to(6)
			_next = _t + 0.3
			_step = 5
		5:
			_check(int(_cult().call("get_realm_index")) == 6, "突破到炼虚")
			_check(bool(_am().call("has_ability", "void_shift")), "炼虚：虚实转换解锁")
			_check(bool(_am().call("has_ability", "dongtian")), "炼虚：开辟洞天解锁（系统能力）")
			# ③ 混元解锁
			_cult().call("set_hunyuan", true)
			_am().call("check_realm_unlocks")
			_check(bool(_am().call("has_ability", "dao_domain")), "混元：道域展开解锁")
			_check(bool(_am().call("has_ability", "myriad_avatars")), "混元：化身千万解锁")
			_next = _t + 0.3
			_step = 6
		6:
			# ④ 能力页渲染数据驱动（ESC 开菜单=背包页，E 翻一页到能力页）
			_press("menu")
			_next = _t + 0.3
			_step = 7
		7:
			_press_key(KEY_E)
			_next = _t + 0.3
			_step = 8
		8:
			_check(_has_menu_label("— 主动 —"), "能力页：主动分区标题")
			_check(_has_menu_label("— 被动 —"), "能力页：被动分区标题")
			_check(_has_menu_label("✓冲刺"), "能力页：初始项显 ✓（数据表 innate）")
			_check(_has_menu_label("✓滑翔"), "能力页：金丹已解锁项显 ✓")
			_check(_has_menu_label("✓化身千万"), "能力页：混元已解锁项显 ✓")
			_check(_has_menu_label("功德金光·大乘"), "能力页：锁定项文本来自数据表（名·条件）")
			_check(_has_menu_label("法天象地·金仙"), "能力页：金仙锁定项条件文本")
			_check(_has_menu_label("✓ 威压 U"), "能力页：战技区保持原样")
			_press("menu") # ESC 关菜单
			_next = _t + 0.3
			_step = 9
		9:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
