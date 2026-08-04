# 验证 GameMenu: ESC 开关、Q/E 翻页到设置页、设置页被选中行时 Q/E 仍可翻页（回归）、I 关闭
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
	# 翻页走 _input 原始键码（Q/E），不用 action；仅按下（同帧释放会吞掉 _input 派发）
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
	_next = _t + 0.3

	match _step:
		0:
			_press("menu")
			print("[TEST] ESC open: paused=", paused, " (expect true)")
			_step = 1
		1, 2, 3, 4, 5, 6, 7, 8: # E 切 8 页: 背包→能力→功法→技能→法宝→宗门→云游→炼丹→设置
			_press_key(KEY_E)
			print("[TEST] page E ", _step)
			_step += 1
		9:
			_check(_has_label("—— 设置 ——"), "设置页打开")
			_press("down") # 选语言行（sel=1）——正是此前拦截 Q/E 的行
			_step = 10
		10:
			_check(_has_label("—— 设置 ——"), "设置页仍在（sel=1 语言行）")
			_press_key(KEY_E) # 回归：被选中行时 Q/E 翻页必须生效（设置 8 → 回卷背包 0）
			_step = 11
		11:
			_check(not _has_label("—— 设置 ——"), "Q/E 在被选中的音量/语言行仍可翻页（离开设置）")
			_check(_has_label("Q/E 切换页"), "回到背包页（提示行 Q/E 切换页）")
			_step = 12
		12:
			_press("inventory") # I 关闭
			print("[TEST] I close: paused=", paused, " (expect false)")
			_step = 13
		13:
			print("[TEST] final state: paused=", paused, " (expect false)")
			if _fail == 0:
				print("[TEST] DONE")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
