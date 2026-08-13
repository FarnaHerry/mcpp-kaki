# 设置页显示设置：窗口模式 3 档 + 分辨率（游戏内部分辨率 axb[比例] 5 档）+ settings.cfg 持久化
# 终案（用户定案）：和普通游戏一样——游戏只管自己的分辨率（content_scale_size），
# 窗口大小用户自管一概不碰（自由拉伸/全屏=屏幕）；缩放固定整数倍（fractional=高级能力不做）
# headless 下 DisplayServer 窗口操作为 no-op，断言落点 = 行标签文案 + cfg 写入 + content_scale 属性
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _saved_display := {} # 进入时快照 [display] 段，结束还原（不污染本机设置）

func _initialize():
	var cfg := ConfigFile.new()
	if cfg.load("user://settings.cfg") == OK:
		for k in ["window_mode"]:
			if cfg.has_section_key("display", k):
				_saved_display[k] = cfg.get_value("display", k, null)
	# 测试前置：窗口模式/分辨率档位置默认，保证档位循环断言确定
	cfg.set_value("display", "window_mode", 0)
	cfg.set_value("display", "res_idx", 0)
	cfg.save("user://settings.cfg")
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

func _cfg_display(key: String, dflt):
	var cfg := ConfigFile.new()
	if cfg.load("user://settings.cfg") != OK:
		return dflt
	return cfg.get_value("display", key, dflt)

func _cs() -> Vector2i:
	return root.content_scale_size

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.25

	match _step:
		0:
			_press("menu") # ESC 开菜单
			_step = 1
		1, 2, 3, 4, 5, 6, 7, 8: # E ×8 → 设置页
			_press_key(KEY_E)
			_step += 1
		9:
			_check(_has_label("—— 设置 ——"), "设置页打开")
			_check(_has_label("窗口模式"), "窗口模式行存在")
			_check(_has_label("分辨率"), "分辨率行存在")
			_check(_has_label("480×270[16:9]"), "分辨率档位格式 axb[16:9]")
			_check(_has_label("保存游戏"), "保存游戏行存在（顺延行4）")
			_check(not _has_label("窗口大小"), "窗口大小行已移除（窗口用户自管）")
			_check(not _has_label("缩放模式"), "缩放模式行已移除（固定整数倍）")
			_check(not _has_label("渲染比例"), "渲染比例行已改名分辨率")
			_press("down") # → sel1 语言
			_step = 10
		10:
			_press("down") # → sel2 窗口模式
			_step = 11
		11:
			_check(_has_label("▶ 窗口模式"), "选中窗口模式行")
			_press("right") # 窗口 → 无边框全屏
			_step = 12
		12:
			_check(int(_cfg_display("window_mode", -1)) == 1, "窗口→无边框全屏（cfg window_mode=1）")
			_press("right") # → 独占全屏
			_step = 13
		13:
			_check(int(_cfg_display("window_mode", -1)) == 2, "无边框全屏→独占全屏（cfg window_mode=2）")
			_press("right") # → 回窗口
			_step = 14
		14:
			_check(int(_cfg_display("window_mode", -1)) == 0, "独占全屏→窗口（3 档循环闭合）")
			_press("down") # → sel3 分辨率
			_step = 15
		15:
			_check(_has_label("▶ 分辨率"), "选中分辨率行")
			_check(_cs() == Vector2i(480, 270), "初始=480×270[16:9]: " + str(_cs()))
			_press("right") # → 16:10
			_step = 16
		16:
			_check(_cs() == Vector2i(480, 300), "→ 480×300[16:10]: " + str(_cs()))
			_check(int(_cfg_display("res_idx", -1)) == 1, "cfg res_idx=1")
			_check(_has_label("480×300[16:10]"), "档位 label=480×300[16:10]")
			_press("right") # → 3:2
			_step = 17
		17:
			_check(_cs() == Vector2i(480, 320), "→ 480×320[3:2]: " + str(_cs()))
			_press("right") # → 4:3
			_step = 18
		18:
			_check(_cs() == Vector2i(480, 360), "→ 480×360[4:3]: " + str(_cs()))
			_press("right") # → 21:9
			_step = 19
		19:
			_check(_cs() == Vector2i(630, 270), "→ 630×270[21:9]: " + str(_cs()))
			_press("right") # → 回 16:9
			_step = 20
		20:
			_check(_cs() == Vector2i(480, 270), "→ 回 480×270[16:9]（5 档循环闭合）")
			# 窗口大小用户自管：改窗口尺寸不会动游戏内部分辨率（无自动匹配）
			root.size = Vector2i(1500, 1000) # 3:2 窗口
			_step = 21
		21:
			_check(_cs() == Vector2i(480, 270), "窗口改3:2 → 内部分辨率不变（游戏不管窗口）: " + str(_cs()))
			_check(root.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_INTEGER, "缩放固定=整数倍")
			_check(root.content_scale_aspect == Window.CONTENT_SCALE_ASPECT_KEEP, "stretch aspect=keep")
			# cfg 不再写旧键
			var cfg2 := ConfigFile.new()
			cfg2.load("user://settings.cfg")
			_check(not cfg2.has_section_key("display", "resolution_idx"), "cfg 不再写 resolution_idx")
			_check(not cfg2.has_section_key("display", "aspect_idx"), "cfg 不再写 aspect_idx")
			_check(not cfg2.has_section_key("display", "scale_mode"), "cfg 不再写 scale_mode")
			# 还原进入前的 [display] 段（避免污染本机设置）
			var cfg := ConfigFile.new()
			cfg.load("user://settings.cfg")
			for k in _saved_display:
				if _saved_display[k] != null:
					cfg.set_value("display", k, _saved_display[k])
			cfg.save("user://settings.cfg")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
