# 设置页显示设置：窗口模式 3 档 + 窗口大小预设/自定义 + 渲染比例 5 档 + 缩放模式 + settings.cfg 持久化
# 渲染分辨率与窗口大小解耦（普通游戏语义）：渲染比例 = content_scale_size，全窗口模式可调
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
		for k in ["window_mode", "resolution_idx", "resolution_custom", "custom_w", "custom_h", "aspect_idx", "scale_mode"]:
			if cfg.has_section_key("display", k):
				_saved_display[k] = cfg.get_value("display", k, null)
	# 测试前置：窗口模式/渲染比例/缩放模式置默认，保证档位循环断言确定
	cfg.set_value("display", "window_mode", 0)
	cfg.set_value("display", "aspect_idx", 0)
	cfg.set_value("display", "scale_mode", 0)
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
			_check(_has_label("窗口大小"), "窗口大小行存在（与渲染解耦）")
			_check(_has_label("渲染比例"), "渲染比例行存在")
			_check(_has_label("缩放模式"), "缩放模式行存在")
			_check(_has_label("保存游戏"), "保存游戏行存在（顺延行6）")
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
			var m = int(_cfg_display("window_mode", -1))
			_check(m == 1, "窗口→无边框全屏（cfg window_mode=1）")
			_check(_has_label("无边框全屏"), "档位 label=无边框全屏")
			_press("right") # → 独占全屏
			_step = 13
		13:
			var m2 = int(_cfg_display("window_mode", -1))
			_check(m2 == 2, "无边框全屏→独占全屏（cfg window_mode=2）")
			_press("right") # → 回窗口
			_step = 14
		14:
			var m3 = int(_cfg_display("window_mode", -1))
			_check(m3 == 0, "独占全屏→窗口（3 档循环闭合）")
			_press("down") # → sel3 窗口大小
			_step = 15
		15:
			_check(_has_label("▶ 窗口大小"), "选中窗口大小行")
			_press("right") # 预设档循环
			_step = 16
		16:
			var ri = int(_cfg_display("resolution_idx", -1))
			_check(ri >= 0 and ri <= 5, "cfg resolution_idx 写入合法档（" + str(ri) + "）")
			_press("interact") # X → 自定义微调子态
			_step = 17
		17:
			_check(bool(_cfg_display("resolution_custom", false)), "X 后进入自定义（cfg resolution_custom=true）")
			_check(_has_label("自定义"), "窗口大小行显示自定义")
			_press("right") # 整数倍 +1
			_step = 18
		18:
			var cw = int(_cfg_display("custom_w", 0))
			_check(cw > 0 and cw % 480 == 0, "自定义宽=480 整数倍（custom_w=" + str(cw) + "）")
			_press("interact") # X 退出微调
			_step = 19
		19:
			_check(_has_label("—— 设置 ——"), "退出微调后设置页仍在")
			_press("down") # → sel4 渲染比例
			_step = 20
		20:
			_check(_has_label("▶ 渲染比例"), "选中渲染比例行")
			_check(_cs() == Vector2i(480, 270), "初始渲染=16:9 480×270（基准）: " + str(_cs()))
			_press("right") # → 16:10
			_step = 21
		21:
			_check(_cs() == Vector2i(480, 300), "16:10 → 480×300（纵向延伸）: " + str(_cs()))
			_check(int(_cfg_display("aspect_idx", -1)) == 1, "cfg aspect_idx=1")
			_check(_has_label("16:10"), "档位 label=16:10")
			_press("right") # → 3:2
			_step = 22
		22:
			_check(_cs() == Vector2i(480, 320), "3:2 → 480×320: " + str(_cs()))
			_press("right") # → 4:3
			_step = 23
		23:
			_check(_cs() == Vector2i(480, 360), "4:3 → 480×360: " + str(_cs()))
			_press("right") # → 21:9
			_step = 24
		24:
			_check(_cs() == Vector2i(630, 270), "21:9 → 630×270（横向延伸）: " + str(_cs()))
			_press("right") # → 回 16:9
			_step = 25
		25:
			_check(_cs() == Vector2i(480, 270), "21:9→16:9（5 档循环闭合）")
			_press("down") # → sel5 缩放模式
			_step = 26
		26:
			_check(_has_label("▶ 缩放模式"), "选中缩放模式行")
			_check(root.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_INTEGER, "初始缩放=整数倍")
			_press("right") # → 铺满
			_step = 27
		27:
			_check(root.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_FRACTIONAL, "铺满 → fractional")
			_check(int(_cfg_display("scale_mode", -1)) == 1, "cfg scale_mode=1")
			_check(_has_label("铺满"), "档位 label=铺满")
			_press("right") # → 回整数倍
			_step = 28
		28:
			_check(root.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_INTEGER, "铺满→整数倍（循环闭合）")
			# 全局 stretch 配置：aspect=keep（引擎托管缩放+居中黑边）
			_check(root.content_scale_aspect == Window.CONTENT_SCALE_ASPECT_KEEP, "stretch aspect=keep")
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
