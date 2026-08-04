# 洞天仓库（v2 储物箱）harness:
# ①炼虚入洞天 ②贴近储物箱 X 打开双栏面板（暂停）③背包→仓库整堆存入
# ④Q/E 切到仓库栏 X 取出 ⑤存入后存档/读档持久化 ⑥ESC 关闭面板恢复
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

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _press_key(code: int):
	# 切栏/翻页走 _input 原始键码（Q/E），不用 action；仅按下（同帧释放会吞掉 _input 派发）
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _dt():
	return root.find_child("DongtianManager", true, false)

func _gm():
	return root.find_child("GameManager", true, false)

func _inv():
	return _player().call("get_inventory")

func _storage_panel():
	return root.find_child("StoragePanel", true, false)

func _inv_count(id: String) -> int:
	return int(_inv().call("get_item_count", id))

func _clear_inv():
	var inv = _inv()
	for i in int(inv.call("get_capacity")):
		inv.call("set_slot", i, "", 0)

func _slot_text():
	# 仓库栏第一行文本（StoragePanel 里的 Label，含 ▶ 前缀）
	for l in _storage_panel().find_children("*", "Label", true, false):
		if "止血草" in l.text:
			return l.text
	return ""

func _storage_slot(i: int) -> Dictionary:
	return _dt().call("get_storage_slot", i)

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			_cult().call("set_free_breakthrough", true)
			_cult().call("accumulate_energy", 100000000000)
			while int(_cult().call("get_realm_index")) < 6:
				_cult().call("attempt_breakthrough")
			_cult().call("set_free_breakthrough", false)
			_clear_inv()
			_inv().call("add_item", "zhi_xue_cao", 3)
			_check(_inv_count("zhi_xue_cao") == 3, "清空背包并备好止血草 ×3")
			_step = 55 # 等一拍让能力解锁落地
		55:
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == true, "进入洞天")
			# 传送到储物箱（与 dongtian.gd 摆点一致：285, 220）
			_player().global_position = Vector2(285, 212)
			_next = _t + 0.5
			_step = 2
		2:
			_check(_hud_has("打开仓库"), "贴近提示：[X] 打开仓库")
			_press("interact")
			_step = 3
		3:
			_check(_storage_panel().call("is_open") == true, "X 打开储物面板")
			_check(self.paused, "面板打开时暂停")
			_press("interact") # 存入背包首槽（止血草 ×3）
			_step = 4
		4:
			_check(_inv_count("zhi_xue_cao") == 0, "存入后背包无止血草")
			var s = _storage_slot(0)
			_check(String(s.get("id", "")) == "zhi_xue_cao", "仓库槽 0 = 止血草")
			_check(int(s.get("quantity", 0)) == 3, "仓库槽 0 数量 3")
			_check(_slot_text().find("止血草") >= 0, "仓库栏可见止血草")
			_press_key(KEY_E) # 切到仓库栏
			_step = 5
		5:
			_press("interact") # 取出
			_step = 6
		6:
			_check(_inv_count("zhi_xue_cao") == 3, "取出后背包恢复 ×3")
			_check(_storage_slot(0).is_empty(), "取出后仓库槽 0 清空")
			_press_key(KEY_Q) # 切回背包栏
			_step = 7
		7:
			_press("interact") # 再存入，供持久化测试
			_step = 8
		8:
			_check(_inv_count("zhi_xue_cao") == 0, "再次存入（背包 3→0）")
			_check(int(_storage_slot(0).get("quantity", 0)) == 3, "仓库槽 0 = ×3")
			_press("menu") # ESC 关闭面板
			_step = 9
		9:
			_check(_storage_panel().call("is_open") == false, "ESC 关闭面板")
			_check(not self.paused, "关闭后面板恢复非暂停")
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 10
		10:
			var s = _storage_slot(0)
			_check(String(s.get("id", "")) == "zhi_xue_cao" and int(s.get("quantity", 0)) == 3,
					"读档后仓库存储保留（止血草 ×3）")
			_check(_dt().call("is_inside") == false, "读档后已退出洞天")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
