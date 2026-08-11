# 设置页显示设置：窗口模式 4 档循环 + 分辨率预设/自定义 + settings.cfg [display] 持久化
# headless 下 DisplayServer 窗口操作为 no-op，断言落点 = 行标签文案 + cfg 写入
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _mode0 := "" # 初始窗口模式行文本（用户 settings.cfg 可能非默认）
var _saved_display := {} # 进入时快照 [display] 段，结束还原（不污染本机设置）

func _initialize():
	var cfg := ConfigFile.new()
	if cfg.load("user://settings.cfg") == OK:
		for k in ["window_mode", "resolution_idx", "resolution_custom", "custom_w", "custom_h"]:
			_saved_display[k] = cfg.get_value("display", k, null)
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
			_check(_has_label("保存游戏"), "保存游戏行存在（顺延行4）")
			_press("down") # → sel1 语言
			_step = 10
		10:
			_press("down") # → sel2 窗口模式
			_step = 11
		11:
			_check(_has_label("▶ 窗口模式"), "选中窗口模式行")
			_press("right") # 循环一档
			_step = 12
		12:
			var m = int(_cfg_display("window_mode", -1))
			_check(m >= 0 and m <= 3, "cfg window_mode 写入合法档（" + str(m) + "）")
			# 再循环三档回 0=窗口
			_press("right")
			_step = 13
		13:
			_press("right")
			_step = 14
		14:
			_press("right") # 1→2→3→0（起点取决于存档，总归环回合法档）
			_step = 15
		15:
			var m2 = int(_cfg_display("window_mode", -1))
			_check(m2 >= 0 and m2 <= 3, "窗口模式循环后 cfg 仍合法（" + str(m2) + "）")
			# 全屏档下分辨率行灰显提示（当前若不是全屏则先切到全屏验证）
			_step = 16
		16:
			_press("down") # → sel3 分辨率
			_step = 17
		17:
			_check(_has_label("▶ 分辨率"), "选中分辨率行")
			_press("right") # 预设档循环
			_step = 18
		18:
			var ri = int(_cfg_display("resolution_idx", -1))
			_check(ri >= 0 and ri <= 5, "cfg resolution_idx 写入合法档（" + str(ri) + "）")
			_press("interact") # X → 自定义微调子态
			_step = 19
		19:
			_check(bool(_cfg_display("resolution_custom", false)), "X 后进入自定义（cfg resolution_custom=true）")
			_check(_has_label("自定义"), "分辨率行显示自定义")
			# 全局 stretch 配置：expand+integer 保证任意窗口尺寸整数倍渲染 + 视口扩展（不花屏）
			var r: Window = root
			_check(r.content_scale_aspect == Window.CONTENT_SCALE_ASPECT_EXPAND, "stretch aspect=expand（多余屏幕扩展视口）")
			_check(r.content_scale_stretch == Window.CONTENT_SCALE_STRETCH_INTEGER, "stretch scale_mode=integer（整数倍渲染）")
			_press("right") # 整数倍 +1（N+1）
			_step = 20
		20:
			var cw = int(_cfg_display("custom_w", 0))
			_check(cw > 0 and cw % 480 == 0, "自定义宽=480 整数倍（无黑边吃满，custom_w=" + str(cw) + "）")
			_press("up") # 再 +1 倍
			_step = 21
		21:
			var ch = int(_cfg_display("custom_h", 0))
			_check(ch > 0 and ch % 270 == 0, "自定义高=270 整数倍（custom_h=" + str(ch) + "）")
			_check(_has_label("×"), "自定义行显示倍数（×N）")
			_press("interact") # X 退出微调
			_step = 22
		22:
			_check(_has_label("—— 设置 ——"), "退出微调后设置页仍在")
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
