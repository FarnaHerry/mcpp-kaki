# 宗门 harness: ①散修无加成 ②凡人拒收 ③炼气拜师+专属技+攻加成
#      ④击杀贡献 ⑤晋阶内门档升 ⑥存档往返 ⑦叛门保留技能 ⑧宗门页 UI
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _atk0 := 0.0

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

func _sect():
	return _player().call("get_sect_system")

func _gm():
	return root.find_child("GameManager", true, false)

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _menu_labels() -> Array:
	var menu = root.find_child("GameMenu", true, false)
	if menu == null:
		return []
	return menu.find_children("*", "Label", true, false)

func _has_menu_label(sub: String) -> bool:
	for l in _menu_labels():
		if sub in l.text:
			return true
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			# ①散修
			_check(String(_sect().call("get_sect_id")) == "", "开局散修")
			_check(float(_sect().call("get_atk_mult")) == 1.0, "散修攻乘区=1")
			_check(int(_sect().call("get_rank")) == -1, "散修无职位")
			# ②凡人拒收
			_check(not _player().call("join_sect", "shushan"), "凡人拜师被拒（需炼气）")
			# ③炼气拜师蜀山
			_breakthrough_to(1)
			_atk0 = float(_player().call("get_effective_attack"))
			_check(_player().call("join_sect", "shushan"), "炼气拜入蜀山")
			_check(String(_sect().call("get_sect_id")) == "shushan", "宗门=蜀山")
			_step = 1
		1:
			_check(bool(_player().call("get_skills").call("is_known", "wan_jian_gui_zong")), "授专属技·万剑归宗")
			_check(abs(float(_sect().call("get_atk_mult")) - 1.06) < 0.001, "外门攻+6%")
			var atk1 = float(_player().call("get_effective_attack"))
			_check(atk1 > _atk0 * 1.05, "有效攻击上升（%.1f→%.1f）" % [_atk0, atk1])
			# ④击杀贡献：就地捏一只怪打死
			var WC = load("res://scripts/world_common.gd")
			var e = WC.spawn_enemy(current_scene, Vector2(200, 210), Color(0.5, 0.5, 0.5, 1), 50.0, 200.0, "SectTestMob")
			e.call("take_damage", 9999.0, _player())
			_next = _t + 0.5
			_step = 2
		2:
			_check(int(_sect().call("get_contribution")) == 1, "击杀贡献+1")
			# ⑤刷到内门（boss×10=100）
			for i in range(10):
				_sect().call("on_kill", true)
			_check(int(_sect().call("get_contribution")) == 101, "贡献101")
			_check(int(_sect().call("get_rank")) == 1, "晋阶内门")
			_check(abs(float(_sect().call("get_atk_mult")) - 1.10) < 0.001, "内门攻+10%")
			_check(String(_sect().call("get_rank_name")) == "内门弟子", "职位名")
			# ⑥存档往返
			_gm().call("save_game", "auto")
			_player().call("leave_sect")
			_check(String(_sect().call("get_sect_id")) == "", "叛门回散修")
			_check(int(_sect().call("get_contribution")) == 0, "叛门贡献清零")
			_gm().call("load_game", "auto")
			_next = _t + 0.8
			_step = 3
		3:
			_check(String(_sect().call("get_sect_id")) == "shushan", "读档宗门=蜀山")
			_check(int(_sect().call("get_contribution")) == 101, "读档贡献保留")
			_check(int(_sect().call("get_rank")) == 1, "读档职位保留")
			# ⑦叛门保留技能
			_player().call("leave_sect")
			_check(bool(_player().call("get_skills").call("is_known", "wan_jian_gui_zong")), "叛门保留专属技")
			# ⑧宗门页 UI
			_press("menu")
			_step = 4
		4:
			_press_key(KEY_E) # → 能力
			_step = 5
		5:
			_press_key(KEY_E) # → 功法
			_step = 6
		6:
			_press_key(KEY_E) # → 技能
			_step = 7
		7:
			_press_key(KEY_E) # → 法宝
			_step = 8
		8:
			_press_key(KEY_E) # → 宗门
			_step = 9
		9:
			_check(_has_menu_label("—— 宗门 ——"), "宗门页标题")
			_check(_has_menu_label("蜀山剑派"), "列表：蜀山")
			_check(_has_menu_label("昆仑道宗"), "列表：昆仑")
			_check(_has_menu_label("蓬莱仙岛"), "列表：蓬莱")
			_check(_has_menu_label("魔罗教"), "列表：魔罗")
			_press("down") # → 昆仑
			_step = 10
		10:
			_check(_has_menu_label("▶ 昆仑道宗"), "光标到昆仑")
			_press("interact") # 拜入昆仑
			_step = 11
		11:
			_check(String(_sect().call("get_sect_id")) == "kunlun", "UI 拜入昆仑")
			_check(_has_menu_label("宗门加成"), "已入门总览")
			_check(_has_menu_label("灵力+10%"), "昆仑灵力加成显示")
			_check(_has_menu_label("已拜入昆仑"), "拜师成功提示")
			_press("interact") # 叛门
			_step = 12
		12:
			_check(String(_sect().call("get_sect_id")) == "", "UI 叛门回散修")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
