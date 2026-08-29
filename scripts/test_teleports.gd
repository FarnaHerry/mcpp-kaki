# 云游阵 harness（TeleportArray + teleport_panel + data/teleports.json）：
# ① JSON 装配（10 阵点/主场景 3 碑）② 走近自动铭刻（flag）
# ③ X 开驾云面板（暂停/未铭刻 ？？？）④ 同洲驾云落阵点
# ⑤ 跨洲境界门控拒行（凡人）→ 金丹后驾云跨洲落阵点 ⑥ 铭刻随存档持久
# 注：一帧只按一键（同帧连按会被引擎 Input 去抖吞掉）；暂停断言在压掉前取样
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _was_paused := false

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
	Input.action_press(action)
	Input.action_release(action)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _gm():
	return root.find_child("GameManager", true, false)

func _panel():
	return root.find_child("TeleportPanel", true, false)

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
	_was_paused = paused # 断言面板暂停要在压掉之前取样
	paused = false

	match _step:
		0:
			# ① 装配
			var dl = root.find_child("DataLoader", true, false)
			_check(int(dl.call("get_all_teleports").size()) == 10, "teleports.json 10 阵点")
			_check(get_nodes_in_group("tp_arrays").size() == 3, "主场景 3 座阵碑（落霞村/花果山/东海之滨）")
			var lx = root.find_child("TpArray_luoxia", true, false)
			_check(lx != null and String(lx.call("get_tp_name")) == "落霞村", "落霞村阵碑就位")
			_check(not bool(lx.call("is_activated")), "初始未铭刻")
			_player().global_position = Vector2(800, 200)
			_next = _t + 0.5
			_step = 1
		1:
			# ② 走近自动铭刻
			var lx = root.find_child("TpArray_luoxia", true, false)
			_check(bool(lx.call("is_activated")), "走近自动铭刻（tp:luoxia）")
			_check(bool(_gm().call("has_flag", "tp:luoxia")), "铭刻入 GameManager flag")
			_press("interact")
			_next = _t + 0.4
			_step = 2
		2:
			# ③ 面板打开 + 暂停
			_check(bool(_panel().call("is_open")), "X 开驾云面板")
			_check(_was_paused, "面板打开暂停世界")
			var found_unknown := false
			var found_luoxia := false
			for l in _panel().find_children("*", "Label", true, false):
				if "未铭刻" in str(l.text):
					found_unknown = true
				if "落霞村" in str(l.text):
					found_luoxia = true
			_check(found_luoxia, "面板列出已铭刻阵点（落霞村）")
			_check(found_unknown, "未铭刻阵点显示 ？？？")
			_press("menu") # ESC 关面板
			_next = _t + 0.4
			_step = 3
		3:
			_check(not bool(_panel().call("is_open")), "ESC 关面板还原暂停")
			_check(not paused, "关闭后世界恢复运行")
			_player().global_position = Vector2(6150, 200)
			_next = _t + 0.5
			_step = 4
		4:
			_check(bool(_gm().call("has_flag", "tp:huaguoshan")), "花果山阵碑铭刻")
			_player().global_position = Vector2(8240, 200)
			_next = _t + 0.5
			_step = 5
		5:
			_check(bool(_gm().call("has_flag", "tp:donghai")), "东海之滨阵碑铭刻")
			_next = _t + 0.3
			_step = 6
		6:
			# ④ 同洲驾云：东海之滨阵碑开面板（sel=0 落霞村）→ down×2 到东海之滨
			_press("interact")
			_next = _t + 0.4
			_step = 7
		7:
			_check(bool(_panel().call("is_open")), "面板再次打开")
			_press("down")
			_next = _t + 0.3
			_step = 8
		8:
			_press("down")
			_next = _t + 0.3
			_step = 9
		9:
			_press("interact") # 选中东海之滨（index 2）→ 驾云
			_next = _t + 0.6
			_step = 10
		10:
			_check(abs(_player().global_position.x - 8240.0) < 60.0, "同洲驾云落东海之滨阵点")
			_check(not bool(_panel().call("is_open")), "驾云后面板关闭")
			# ⑤ 跨洲门控：直设长安铭刻（模拟他洲铭刻）
			_gm().call("set_flag", "tp:changan")
			_next = _t + 0.3
			_step = 11
		11:
			# 重开面板 sel 归 0；长安坊市 index 5 → down×5
			_press("interact")
			_next = _t + 0.4
			_step = 12
		12:
			_check(bool(_panel().call("is_open")), "面板打开（重开 sel 归 0）")
			_press("down")
			_next = _t + 0.3
			_step = 13
		13:
			_press("down")
			_next = _t + 0.3
			_step = 14
		14:
			_press("down")
			_next = _t + 0.3
			_step = 15
		15:
			_press("down")
			_next = _t + 0.3
			_step = 16
		16:
			_press("down")
			_next = _t + 0.3
			_step = 17
		17:
			_press("interact") # 长安坊市：凡人跨洲 → 拒行
			_next = _t + 0.5
			_step = 18
		18:
			_check(_scene_path() == "res://scenes/main.tscn", "凡人期跨洲拒行（仍在东胜）")
			_check(bool(_panel().call("is_open")), "拒行后面板不关（提示境界不足）")
			_press("menu")
			_next = _t + 0.4
			_step = 19
		19:
			# 突破金丹 → 避火庄驾云（flag 直设）
			_gm().call("set_flag", "tp:bihuo")
			_breakthrough_to(3)
			_check(int(_cult().call("get_realm_index")) == 3, "突破到金丹")
			_next = _t + 0.3
			_step = 20
		20:
			# 重开面板 sel 归 0；避火庄 index 3 → down×3
			_press("interact")
			_next = _t + 0.4
			_step = 21
		21:
			_check(bool(_panel().call("is_open")), "面板打开（金丹后）")
			_press("down")
			_next = _t + 0.3
			_step = 22
		22:
			_press("down")
			_next = _t + 0.3
			_step = 23
		23:
			_press("down")
			_next = _t + 0.3
			_step = 24
		24:
			_press("interact") # 避火庄：金丹可至 → 跨洲驾云
			_next = _t + 1.5
			_step = 25
		25:
			_check(_scene_path() == "res://scenes/continents/xiniuhe.tscn", "金丹后驾云跨洲入西牛贺洲")
			_check(abs(_player().global_position.x - 1590.0) < 80.0, "落点=避火庄阵点（%d）" % int(_player().global_position.x))
			_check(get_nodes_in_group("tp_arrays").size() == 2, "西牛贺洲 2 座阵碑")
			# ⑥ 铭刻随存档持久
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_next = _t + 1.2
			_step = 26
		26:
			_check(bool(_gm().call("has_flag", "tp:luoxia")), "读档后铭刻保留（tp:luoxia）")
			_check(bool(_gm().call("has_flag", "tp:bihuo")), "读档后铭刻保留（tp:bihuo）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
