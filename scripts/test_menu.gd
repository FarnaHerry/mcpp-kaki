# 验证 GameMenu: ESC 开关、切页、I 直开背包、设置页、嵌套暂停还原
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

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
		1, 2, 3, 4, 5: # 向右切 5 页: 背包→能力→功法→技能→法宝→设置
			_press("right")
			print("[TEST] page right ", _step)
			_step += 1
		6:
			_press("interact") # 设置页 F = 音量+
			_press("down")
			_press("interact") # 保存游戏
			print("[TEST] settings ok, still paused=", paused)
			_step = 7
		7:
			_press("inventory") # I: 设置页→背包页
			print("[TEST] I to inventory page")
			_step = 8
		8:
			_press("down") # 背包内导航
			_press("down")
			_press("inventory") # 再按 I 关闭
			print("[TEST] I close: paused=", paused, " (expect false)")
			_step = 9
		9:
			print("[TEST] final state: paused=", paused, " (expect false)")
			print("[TEST] DONE")
			return true
	return false
