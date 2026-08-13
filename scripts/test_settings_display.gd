# 设置页显示设置：窗口模式 3 档 + 分辨率（原生分辨率档：独占全屏=显示模式/渲染精度）+ cfg 持久化
# 终案 v2：分辨率行=原生分辨率（1920×1080 等，常规游戏语义，全屏下影响渲染精度）；
# 无边框全屏=桌面/窗口=自由拉伸 灰显；内部渲染比例（480 基准 5 档）按显示比例静默自动匹配；
# 缩放固定整数倍（fractional=高级能力不做）
# headless 下 DisplayServer 窗口操作为 no-op，断言落点 = 行标签 + cfg + content_scale 属性 + 自动匹配
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
	cfg.set_value("display", "res_idx", 2)
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
			_check(_has_label("窗口自由拉伸"), "窗口档：分辨率行灰显=窗口自由拉伸")
			_check(_has_label("保存游戏"), "保存游戏行存在（顺延行4）")
			_check(not _has_label("窗口大小"), "窗口大小行已移除")
			_check(not _has_label("缩放模式"), "缩放模式行已移除（固定整数倍）")
			_check(not _has_label("渲染比例"), "渲染比例行已移除（内部自动匹配）")
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
			_check(_has_label("无边框全屏=桌面"), "无边框全屏：分辨率行=桌面分辨率")
			_press("right") # → 独占全屏
			_step = 13
		13:
			_check(int(_cfg_display("window_mode", -1)) == 2, "无边框全屏→独占全屏（cfg window_mode=2）")
			_check(_has_label("1920×1080"), "独占全屏：分辨率行=原生档（默认1920×1080）")
			_press("down") # → sel3 分辨率
			_step = 14
		14:
			_check(_has_label("▶ 分辨率"), "选中分辨率行（独占全屏可选）")
			_press("right") # → 2560×1440
			_step = 15
		15:
			_check(int(_cfg_display("res_idx", -1)) == 3, "cfg res_idx=3")
			_check(_has_label("2560×1440"), "分辨率档位→2560×1440")
			_press("right") # → 3120×2080（3:2 屏）
			_step = 16
		16:
			_check(_has_label("3120×2080"), "分辨率档位→3120×2080")
			# 内部渲染比例自动匹配显示比例：3120×2080=3:2 → 480×320
			_check(_cs() == Vector2i(480, 320), "内部比例自动匹配 3:2 → 480×320: " + str(_cs()))
			_press("left") # → 回 2560×1440
			_step = 17
		17:
			_check(int(_cfg_display("res_idx", -1)) == 3, "←回退 cfg res_idx=3")
			_check(_cs() == Vector2i(480, 270), "内部比例回 16:9 → 480×270: " + str(_cs()))
			_press("up") # → sel2 窗口模式
			_step = 18
		18:
			_press("right") # 独占全屏 → 回窗口
			_step = 19
		19:
			_check(int(_cfg_display("window_mode", -1)) == 0, "独占全屏→窗口（3 档循环闭合）")
			# 窗口档：内部比例跟随窗口尺寸（用户自由拉伸，视野自适应）
			root.size = Vector2i(1500, 1000) # 3:2 窗口
			_step = 20
		20:
			_check(_cs() == Vector2i(480, 320), "窗口拖成3:2 → 内部 480×320: " + str(_cs()))
			root.size = Vector2i(2100, 900) # 21:9 窗口
			_step = 21
		21:
			_check(_cs() == Vector2i(630, 270), "窗口拖成21:9 → 内部 630×270: " + str(_cs()))
			root.size = Vector2i(1920, 1080)
			_step = 22
		22:
			_check(_cs() == Vector2i(480, 270), "窗口回16:9 → 内部 480×270")
			_check(root.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_INTEGER, "缩放固定=整数倍")
			_check(root.content_scale_aspect == Window.CONTENT_SCALE_ASPECT_KEEP, "stretch aspect=keep")
			# cfg 不再写旧键
			var cfg2 := ConfigFile.new()
			cfg2.load("user://settings.cfg")
			_check(not cfg2.has_section_key("display", "resolution_idx"), "cfg 不再写 resolution_idx")
			_check(not cfg2.has_section_key("display", "aspect_idx"), "cfg 不再写 aspect_idx（内部比例自动匹配）")
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
