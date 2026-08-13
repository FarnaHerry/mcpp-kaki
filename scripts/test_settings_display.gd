# 设置页显示设置：窗口模式 3 档 + 分辨率（原生分辨率档）+ 帧率（动态，按系统刷新率）+ 垂直同步 + cfg 持久化
# 终案 v4：16:9 固定基准（480×270 内部画布），非 16:9（3:2/4:3/16:10/21:9）由 aspect=keep 居中黑边兼容；
# 分辨率行=原生分辨率（1920×1080 等，常规游戏语义），全窗口模式可调；
# 缩放固定分数倍（fractional，窗口/全屏填满无黑边）。帧率行=上限档（按系统最高刷新率动态生成+无限）；
# 垂直同步行=关/开（Godot 原生 window_set_vsync_mode）。
# headless 下刷新率≈60（screen_get_refresh_rate≤1→默认 60），档位=[30,60,0]；断言落点 = 行标签 + cfg + content_scale + Engine.max_fps
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _saved_display := {} # 进入时快照 [display] 段，结束还原（不污染本机设置）

func _initialize():
	var cfg := ConfigFile.new()
	if cfg.load("user://settings.cfg") == OK:
		for k in ["window_mode", "res_idx", "max_fps", "vsync"]:
			if cfg.has_section_key("display", k):
				_saved_display[k] = cfg.get_value("display", k, null)
	# 测试前置：窗口模式/分辨率档/帧率/垂直同步置默认，保证档位循环断言确定
	cfg.set_value("display", "window_mode", 0)
	cfg.set_value("display", "res_idx", 2)
	cfg.set_value("display", "max_fps", 60)
	cfg.set_value("display", "vsync", 1)
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
			_check(_has_label("1920×1080"), "窗口档：分辨率行=原生档（默认1920×1080）")
			_check(not _has_label("窗口自由拉伸"), "窗口档不再灰显「自由拉伸」")
			_check(not _has_label("无边框全屏=桌面"), "无边框档不再灰显「=桌面」")
			_check(_has_label("帧率"), "帧率行存在")
			_check(_has_label("垂直同步"), "垂直同步行存在")
			_check(_has_label("保存游戏"), "保存游戏行存在（顺延行6）")
			_check(not _has_label("窗口大小"), "窗口大小行已移除")
			_check(not _has_label("缩放模式"), "缩放模式行已移除（固定分数倍）")
			_check(not _has_label("渲染比例"), "渲染比例行已移除（内部画布固定 16:9）")
			_press("down") # → sel1 语言
			_step = 10
		10:
			_press("down") # → sel2 窗口模式
			_step = 11
		11:
			_press("down") # → sel3 分辨率（先测窗口档可调）
			_step = 12
		12:
			_check(_has_label("▶ 分辨率"), "选中分辨率行（窗口档）")
			_press("right") # → 2560×1440
			_step = 13
		13:
			_check(int(_cfg_display("res_idx", -1)) == 3, "窗口档 cfg res_idx=3")
			_check(_has_label("2560×1440"), "窗口档分辨率→2560×1440")
			_check(root.size == Vector2i(2560, 1440), "窗口档：窗口尺寸=所选分辨率 " + str(root.size))
			_check(_cs() == Vector2i(480, 270), "内部比例 16:9 → 480×270: " + str(_cs()))
			_press("right") # → 3120×2080（3:2 屏）
			_step = 14
		14:
			_check(_has_label("3120×2080"), "窗口档分辨率→3120×2080")
			_check(root.size == Vector2i(3120, 2080), "窗口档：窗口尺寸=3120×2080 " + str(root.size))
			_check(_cs() == Vector2i(480, 270), "内部画布固定 16:9 → 480×270（3:2 不再延伸）: " + str(_cs()))
			_press("left") # → 回 2560×1440
			_step = 15
		15:
			_check(int(_cfg_display("res_idx", -1)) == 3, "←回退 cfg res_idx=3")
			_press("up") # → sel2 窗口模式
			_step = 16
		16:
			_check(_has_label("▶ 窗口模式"), "选中窗口模式行")
			_press("right") # → 无边框全屏
			_step = 17
		17:
			_check(int(_cfg_display("window_mode", -1)) == 1, "窗口→无边框全屏（cfg window_mode=1）")
			_check(_has_label("2560×1440"), "无边框全屏：分辨率行仍=原生档（非「=桌面」）")
			_press("down") # → sel3 分辨率
			_step = 18
		18:
			_check(_has_label("▶ 分辨率"), "选中分辨率行（无边框全屏）")
			_press("right") # → 3120×2080
			_step = 19
		19:
			_check(int(_cfg_display("res_idx", -1)) == 4, "无边框全屏 cfg res_idx=4（可调）")
			_check(_has_label("3120×2080"), "无边框全屏分辨率→3120×2080")
			_press("up") # → sel2 窗口模式
			_step = 20
		20:
			_press("right") # → 独占全屏
			_step = 21
		21:
			_check(int(_cfg_display("window_mode", -1)) == 2, "无边框全屏→独占全屏（cfg window_mode=2）")
			_check(_has_label("3120×2080"), "独占全屏：分辨率行=原生档（携带3120×2080）")
			_press("down") # → sel3 分辨率
			_step = 22
		22:
			_check(_has_label("▶ 分辨率"), "选中分辨率行（独占全屏）")
			_press("right") # → 3840×2160
			_step = 23
		23:
			_check(int(_cfg_display("res_idx", -1)) == 5, "独占全屏 cfg res_idx=5（可调）")
			_check(_has_label("3840×2160"), "独占全屏分辨率→3840×2160")
			_check(_cs() == Vector2i(480, 270), "内部画布 16:9 → 480×270: " + str(_cs()))
			_press("left") # → 回 3120×2080（3:2）
			_step = 24
		24:
			_check(_cs() == Vector2i(480, 270), "独占全屏切 3:2 档仍 480×270（黑边兼容）: " + str(_cs()))
			_press("up") # → sel2 窗口模式
			_step = 25
		25:
			_press("right") # 独占全屏 → 回窗口
			_step = 26
		26:
			_check(int(_cfg_display("window_mode", -1)) == 0, "独占全屏→窗口（3 档循环闭合）")
			# 窗口档：内部画布固定 16:9，窗口拖成 21:9/3:2 等非 16:9 均不再改变 content_scale_size
			root.size = Vector2i(2100, 900) # 21:9 窗口
			_step = 27
		27:
			_check(_cs() == Vector2i(480, 270), "窗口拖成21:9 → 内部仍 480×270（黑边兼容）: " + str(_cs()))
			root.size = Vector2i(1920, 1080)
			_step = 28
		28:
			_check(_cs() == Vector2i(480, 270), "窗口回16:9 → 内部 480×270")
			_check(root.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_FRACTIONAL, "缩放=分数倍（窗口填满无黑边）")
			_check(root.content_scale_aspect == Window.CONTENT_SCALE_ASPECT_KEEP, "stretch aspect=keep")
			_press("down") # → sel3 分辨率
			_step = 29
		29:
			_press("down") # → sel4 帧率
			_step = 30
		30:
			_check(_has_label("▶ 帧率"), "选中帧率行")
			_check(_has_label("60"), "帧率默认 60")
			_check(int(Engine.max_fps) == 60, "Engine.max_fps=60（默认）")
			_press("right") # → 无限（headless 档位 [30,60,0]）
			_step = 31
		31:
			_check(int(_cfg_display("max_fps", -1)) == 0, "帧率 cfg max_fps=0（无限）")
			_check(_has_label("无限"), "帧率→无限（不锁帧）")
			_check(int(Engine.max_fps) == 0, "Engine.max_fps=0（不锁帧）")
			_press("right") # → 30
			_step = 32
		32:
			_check(int(_cfg_display("max_fps", -1)) == 30, "帧率 cfg max_fps=30")
			_check(_has_label("30"), "帧率→30")
			_check(int(Engine.max_fps) == 30, "Engine.max_fps=30")
			_press("down") # → sel5 垂直同步
			_step = 33
		33:
			_check(_has_label("▶ 垂直同步"), "选中垂直同步行")
			_check(int(_cfg_display("vsync", -1)) == 1, "垂直同步默认开 cfg=1")
			_press("right") # → 关
			_step = 34
		34:
			_check(int(_cfg_display("vsync", -1)) == 0, "垂直同步→关 cfg=0")
			# cfg 不再写旧键
			var cfg2 := ConfigFile.new()
			cfg2.load("user://settings.cfg")
			_check(not cfg2.has_section_key("display", "resolution_idx"), "cfg 不再写 resolution_idx")
			_check(not cfg2.has_section_key("display", "aspect_idx"), "cfg 不再写 aspect_idx（内部画布固定 16:9）")
			_check(not cfg2.has_section_key("display", "scale_mode"), "cfg 不再写 scale_mode")
			_check(not cfg2.has_section_key("display", "fps_idx"), "cfg 不再写 fps_idx（改 max_fps 值）")
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
