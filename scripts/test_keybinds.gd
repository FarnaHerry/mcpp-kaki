# 键位配置 harness（GameMenu 设置页「键位」子页 + InputMap 运行时改绑 + keybinds.cfg）：
# ① 设置页 6 号「键位」行 X 进子页（32 可改绑项 + 恢复默认行）
# ② 跳跃改绑 N：X 进「等待按键」→ 原始键 N → InputMap 生效 + cfg 落盘
# ③ 冲突拒绑：冲刺改绑 N（已被跳跃占用）→ 提示占用 + 绑定不变
# ④ ESC 取消捕获：绑定不变且子页不关  ⑤ reload_keybinds 模拟重载 → cfg 覆盖重新应用
# ⑥ 恢复默认键位：全部回快照 + cfg 覆盖清空（interact 双事件 Space+X 还原）
# 输入模式：action 轮询用「按住一帧再释放」（_hold）；原始键码用 _press_key（只发按下，次步补释放）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _held := "" # 上一拍按住的 action，本拍释放
var _held_keys: Array = [] # 上一拍按下的原始键码，本拍补释放
var _cfg_backup: PackedByteArray = [] # 进入前 keybinds.cfg 快照，结束还原（不污染本机设置）
var _cfg_existed := false

const CFG_PATH := "user://keybinds.cfg"

func _initialize():
	_cfg_existed = FileAccess.file_exists(CFG_PATH)
	if _cfg_existed:
		_cfg_backup = FileAccess.get_file_as_bytes(CFG_PATH)
		DirAccess.remove_absolute(ProjectSettings.globalize_path(CFG_PATH))
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
	_held = action
	Input.action_press(action)

func _press_key(code: int):
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)
	_held_keys.append(code)

func _release_held():
	if _held != "":
		Input.action_release(_held)
		_held = ""
	for code in _held_keys:
		var ev := InputEventKey.new()
		ev.keycode = code
		ev.physical_keycode = code
		ev.pressed = false
		Input.parse_input_event(ev)
	_held_keys.clear()

func _menu():
	return root.find_child("GameMenu", true, false)

func _scan(n: Node, s: String) -> bool:
	if n is Label and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _scan(c, s):
			return true
	return false

func _has_label(s: String) -> bool:
	var m = _menu()
	return m != null and _scan(m, s)

func _bind_phys(action: String) -> int:
	for ev in InputMap.action_get_events(action):
		if ev is InputEventKey:
			var p := int(ev.physical_keycode)
			return p if p != 0 else int(ev.keycode)
	return 0

func _bind_count(action: String) -> int:
	return InputMap.action_get_events(action).size()

func _cfg_has(action: String) -> bool:
	var cfg := ConfigFile.new()
	if cfg.load(CFG_PATH) != OK:
		return false
	return cfg.has_section_key("keybinds", action)

func _cfg_val(action: String) -> int:
	var cfg := ConfigFile.new()
	if cfg.load(CFG_PATH) != OK:
		return -1
	return int(cfg.get_value("keybinds", action, -1))

func _restore_cfg():
	if _cfg_existed:
		var f := FileAccess.open(CFG_PATH, FileAccess.WRITE)
		if f:
			f.store_buffer(_cfg_backup)
			f.close()
	elif FileAccess.file_exists(CFG_PATH):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(CFG_PATH))

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false # 菜单暂停世界，harness 继续跑
	_release_held()

	match _step:
		0:
			_hold("menu") # ESC 开菜单
			_step = 1
		1:
			_check(_has_label("Q/E 切换页"), "ESC 开菜单（背包页）")
			_check(_bind_phys("jump") == KEY_C, "前置：跳跃默认 [C]")
			_press_key(KEY_Q) # Q×3 翻页：背包(1)→个人信息(0)
			_step = 2
		2:
			_press_key(KEY_Q) # →图鉴(10)
			_step = 3
		3:
			_press_key(KEY_Q) # →设置(9)
			_step = 4
		4:
			_check(_has_label("—— 设置 ——"), "Q×3 翻到设置页")
			_check(_has_label("键位"), "设置页含「键位」行")
			_hold("down") # sel0→1
			_step = 5
		5, 6, 7, 8, 9:
			_hold("down") # →sel6 键位
			_step += 1
		10:
			_check(_has_label("▶ 键位"), "选中键位行（sel=6）")
			_hold("interact") # X 进键位子页
			_step = 11
		11:
			# ① 子页装配
			_check(_has_label("—— 键位 ——"), "X 进键位子页")
			_check(_has_label("跳跃/飞行"), "子页含跳跃行")
			_check(_has_label("技能槽 Q"), "子页含技能槽行")
			_check(_has_label("消耗品 6"), "子页含消耗品行")
			_check(_has_label("恢复默认键位"), "子页含恢复默认行")
			_check(_has_label("跳跃/飞行  [C]"), "跳跃显示当前绑定 [C]")
			_hold("down") # →sel1
			_step = 12
		12, 13, 14:
			_hold("down") # →sel4 跳跃/飞行
			_step += 1
		15:
			_check(_has_label("▶ 跳跃/飞行  [C]"), "选中跳跃行")
			_hold("interact") # X 进等待按键态
			_step = 16
		16:
			_check(_has_label("按下新键"), "进入等待按键态")
			_press_key(KEY_N) # ② 改绑 N
			_step = 17
		17:
			_check(_bind_phys("jump") == KEY_N, "改绑后 InputMap 跳跃=[N]")
			_check(_bind_count("jump") == 1, "改绑收敛为单键")
			_check(_has_label("已改绑为"), "改绑成功提示")
			_check(_cfg_val("jump") == KEY_N, "keybinds.cfg 落盘 jump=N")
			_hold("down") # →sel5 冲刺
			_step = 18
		18:
			_hold("interact") # X 进等待按键态（冲刺）
			_step = 19
		19:
			_check(_has_label("按下新键"), "冲刺行进入等待按键态")
			_press_key(KEY_N) # ③ 冲突：N 已被跳跃占用
			_step = 20
		20:
			_check(_has_label("占用"), "冲突提示「已被 跳跃/飞行 占用」")
			_check(_bind_phys("dash") == KEY_Z, "冲突拒绑：冲刺仍 [Z]")
			_check(_bind_phys("jump") == KEY_N, "冲突拒绑：跳跃仍 [N]")
			_hold("interact") # 再次 X 进等待按键态（冲刺）
			_step = 21
		21:
			_check(_has_label("按下新键"), "再次进入等待按键态")
			_press_key(KEY_ESCAPE) # ④ ESC 取消
			_step = 22
		22:
			_check(not _has_label("按下新键"), "ESC 取消捕获（退出等待态）")
			_check(_bind_phys("dash") == KEY_Z, "取消不改绑：冲刺仍 [Z]")
			_check(_has_label("—— 键位 ——"), "ESC 取消后子页不关")
			# ⑤ 模拟重载：跳跃还原默认 C → reload_keybinds 应从 cfg 重新应用 N
			InputMap.action_erase_events("jump")
			var ev := InputEventKey.new()
			ev.keycode = KEY_C
			ev.physical_keycode = KEY_C
			InputMap.action_add_event("jump", ev)
			_check(_bind_phys("jump") == KEY_C, "重载前置：跳跃手动还原 [C]")
			_menu().call("reload_keybinds")
			_check(_bind_phys("jump") == KEY_N, "reload_keybinds 后 cfg 覆盖重新应用（跳跃=[N]）")
			_hold("right") # sel5(col0) → col1 row5
			_step = 23
		23:
			_hold("right") # → col2 row5（消耗品 3，index 29）
			_step = 24
		24, 25, 26:
			_hold("down") # → index 32 恢复默认键位
			_step += 1
		27:
			_check(_has_label("▶ 恢复默认键位"), "选中恢复默认行")
			_hold("interact") # X 恢复默认
			_step = 28
		28:
			_check(_has_label("已恢复默认键位"), "恢复默认提示")
			_check(_bind_phys("jump") == KEY_C, "恢复默认：跳跃回 [C]")
			_check(_bind_phys("dash") == KEY_Z, "恢复默认：冲刺回 [Z]")
			_check(_bind_count("interact") == 2, "恢复默认：交互还原 Space+X 双事件")
			_check(not _cfg_has("jump"), "恢复默认：cfg 覆盖项清空")
			_hold("menu") # ESC 退回设置页
			_step = 29
		29:
			_check(_has_label("—— 设置 ——"), "ESC 退回设置页")
			_check(not _has_label("—— 键位 ——"), "键位子页已退出")
			_hold("menu") # ESC 关菜单
			_step = 30
		30:
			_check(not _has_label("—— 设置 ——"), "ESC 关菜单")
			_restore_cfg()
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
