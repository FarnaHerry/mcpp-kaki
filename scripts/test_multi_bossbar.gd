# 多 Boss 血条：黑白无常同场两条 + 任意多个动态增删（拓展性）
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

func _hud():
	return root.find_child("GameHUD", true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			var hud = _hud()
			_check(hud != null, "GameHUD 存在")
			_check(int(hud.call("get_boss_bar_count")) == 0, "初始 0 条血条")
			# 黑白无常同场：两条血条
			hud.call("on_boss_fight_update", "黑无常", 100.0, 100.0)
			hud.call("on_boss_fight_update", "白无常", 80.0, 100.0)
			_check(int(hud.call("get_boss_bar_count")) == 2, "黑白无常同场 = 2 条血条")
			_check(bool(hud.call("is_boss_bar_visible")), "血条可见")
			_check(String(hud.call("get_boss_bar_name")) == "黑无常", "首条=黑无常")
			_step = 1
		1:
			# 拓展性：任意多个（第 3、4 个）
			var hud = _hud()
			hud.call("on_boss_fight_update", "秦广王", 50.0, 100.0)
			hud.call("on_boss_fight_update", "判官", 200.0, 200.0)
			_check(int(hud.call("get_boss_bar_count")) == 4, "4 个 Boss 同场 = 4 条血条（任意多个）")
			# 更新已存在（同名）不新增
			hud.call("on_boss_fight_update", "白无常", 40.0, 100.0)
			_check(int(hud.call("get_boss_bar_count")) == 4, "同名更新不新增条")
			_step = 2
		2:
			# 按名移除：黑无常死了 → 剩 3 条，其余位置不动
			var hud = _hud()
			hud.call("on_boss_fight_ended", "黑无常")
			_check(int(hud.call("get_boss_bar_count")) == 3, "黑无常死亡后剩 3 条")
			_check(String(hud.call("get_boss_bar_name")) == "白无常", "移除首条后新首条=白无常")
			# 再移除全部
			hud.call("on_boss_fight_ended", "白无常")
			hud.call("on_boss_fight_ended", "秦广王")
			hud.call("on_boss_fight_ended", "判官")
			_check(int(hud.call("get_boss_bar_count")) == 0, "全部移除后 0 条")
			_check(not bool(hud.call("is_boss_bar_visible")), "血条全部隐藏")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
