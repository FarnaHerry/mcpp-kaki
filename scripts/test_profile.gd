# 个人信息页：ESC 菜单（Q 回退一次到首页）显示人物数据（境界/生命/灵力/攻击/防御/速度/饱食/寿元）
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

	match _step:
		0:
			_press("menu") # ESC 开菜单（默认背包页，个人信息为首页需 Q 回退一次）
			_step = 1
		1:
			_press_key(KEY_Q) # → 个人信息页
			_step = 2
		2:
			_check(_has_label("—— 个人信息 ——"), "个人信息页标题")
			_check(_has_label("境界"), "境界行存在")
			_check(_has_label("生命"), "生命行存在")
			_check(_has_label("灵力"), "灵力行存在")
			_check(_has_label("攻击"), "攻击行存在")
			_check(_has_label("防御"), "防御行存在")
			_check(_has_label("速度"), "速度行存在")
			_check(_has_label("饱食"), "饱食行存在")
			_check(_has_label("寿元"), "寿元行存在")
			_check(_has_label("凡人"), "境界值显示（凡人）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
