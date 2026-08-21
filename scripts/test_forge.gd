# 熔炼炉页端到端测试：①侧边栏 4 项 ②炼丹子页配方 ③装备铸造 ④法宝铸造 ⑤装备强化 ⑥存档往返强化
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _p = null
var _inv = null
var _arts = null
var _cs = null
var _gm = null

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

func _cyan(n: Node, s: String) -> bool:
	if n is Label and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _cyan(c, s):
			return true
	return false

func _hl(s: String) -> bool:
	var menu = root.find_child("GameMenu", true, false)
	return menu != null and _cyan(menu, s)

func _cnt(id: String) -> int:
	if _inv == null: return 0
	for i in range(_inv.call("get_capacity")):
		var sd = _inv.call("get_slot", i)
		if not sd.is_empty() and String(sd["id"]) == id:
			return int(sd["quantity"])
	return 0

func _pk(code: int):
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

func _find_refs():
	if _p == null: _p = root.find_child("Player", true, false)
	if _p != null and _inv == null: _inv = _p.call("get_inventory")
	if _p != null and _arts == null: _arts = _p.call("get_artifacts")
	if _cs == null: _cs = root.find_child("CurrencySystem", true, false)
	if _gm == null: _gm = root.find_child("GameManager", true, false)

func _press_action(action: String):
	Input.action_press(action)

func _release_action(action: String):
	Input.action_release(action)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	_find_refs()

	match _step:
		1:
			_next = _t + 0.6
			_find_refs()
			_check(_p != null, "player exists")
			_check(_inv != null, "inventory exists")
			_inv.call("add_item", "zhi_xue_cao", 20)
			_inv.call("add_item", "ju_ling_cao", 20)
			_inv.call("add_item", "xuan_bing_sui", 5)
			if _cs: _cs.call("add", 1, 20)
			_press_action("menu")
		2:
			_next = _t + 0.2
			_release_action("menu")
			_pk(KEY_E)
		3:
			_next = _t + 0.25
			_pk(KEY_E)
		4:
			_next = _t + 0.25
			_pk(KEY_E)
		5:
			_next = _t + 0.25
			_pk(KEY_E)
		6:
			_next = _t + 0.25
			_pk(KEY_E)
		7:
			_next = _t + 0.25
			_pk(KEY_E)
		8:
			_next = _t + 0.25
			_pk(KEY_E)
		9:
			_next = _t + 0.35
			_check(_hl("熔炼炉"), "熔炼炉页标题")
			_check(_hl("炼丹"), "侧边栏子页：炼丹")
			_check(_hl("装备铸造"), "侧边栏子页：装备铸造")
			_check(_hl("法宝铸造"), "侧边栏子页：法宝铸造")
			_check(_hl("装备强化"), "侧边栏子页：装备强化")
			_check(_hl("回春丹"), "炼丹子页：回春丹配方")
			# 侧边栏切到装备铸造（→）
			_press_action("right")
		10:
			_next = _t + 0.25
			_release_action("right")
			_check(_hl("铁剑"), "装备铸造子页：铁剑")
			_press_action("interact")
		11:
			_next = _t + 0.25
			_release_action("interact")
			_check(_hl("铸造成功"), "装备铸造成功")
			_check(_cnt("iron_sword") >= 1, "铁剑已入背包")
			_check(_cnt("zhi_xue_cao") == 17, "止血草扣3（余17）")
			_inv.call("add_item", "iron_sword", 1)
			# 侧边栏切到法宝铸造（→）
			_press_action("right")
		12:
			_next = _t + 0.25
			_release_action("right")
			_check(_hl("飞剑"), "法宝铸造子页：飞剑")
			_press_action("interact")
		13:
			_next = _t + 0.25
			_release_action("interact")
			var owned = _arts != null and _arts.call("is_owned", "fei_jian")
			_check(owned, "飞剑已习得")
			_check(_cnt("iron_sword") >= 1, "铁剑消耗1把后仍≥1")
			# 侧边栏切到装备强化（→）
			_press_action("right")
		14:
			_next = _t + 0.25
			_release_action("right")
			_check(_hl("装备强化"), "装备强化子页标题")
			_check(_hl("铁剑"), "铁剑在强化列表")
			_press_action("interact")
		15:
			_next = _t + 0.25
			_release_action("interact")
			_check(_hl("强化成功"), "装备强化成功")
			_check(_inv.call("get_item_extra_atk", "iron_sword") == 1, "铁剑攻+1")
			_check(_inv.call("get_item_extra_def", "iron_sword") == 1, "铁剑防+1")
			_press_action("interact")
		16:
			_next = _t + 0.25
			_release_action("interact")
			_check(_inv.call("get_item_extra_atk", "iron_sword") == 2, "二次强化攻+2")
			_check(_inv.call("get_item_extra_def", "iron_sword") == 2, "二次强化防+2")
			_press_action("menu")
		17:
			_next = _t + 0.25
			_release_action("menu")
			_check(_gm != null, "GameManager")
			if _gm:
				_gm.call("save_game", "test_forge")
				_gm.call("load_game", "test_forge")
				_find_refs()
				_check(_inv.call("get_item_extra_atk", "iron_sword") == 2, "读档后攻+2 保持")
				_check(_inv.call("get_item_extra_def", "iron_sword") == 2, "读档后防+2 保持")
		18:
			_next = _t + 0.35
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false