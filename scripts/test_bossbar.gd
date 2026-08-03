# 验证: ①平时 Boss 条隐藏 ②aggro 触发 Boss 条上 HUD(顶部居中,显示名字)
#      ③伤害更新血量 ④Boss 死亡撤条
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _boss = null

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

func _find_boss():
	for n in root.find_children("*", "Enemy", true, false):
		if bool(n.get("is_boss")):
			return n
	return null

func _hud():
	return root.find_child("GameHUD", true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			_boss = _find_boss()
			_check(_boss != null, "boss exists")
			var hud = _hud()
			_check(hud != null, "hud exists")
			_check(not bool(hud.call("is_boss_bar_visible")), "boss bar hidden before fight")
		2:
			# 把玩家挪到 Boss 视野内 → aggro(Chase)→ Boss 条出现
			_next = _t + 0.8
			var p = root.find_child("Player", true, false)
			p.global_position = _boss.global_position + Vector2(-60, 0)
		3:
			_next = _t + 0.3
			var hud = _hud()
			print("[TEST] bar visible=", hud.call("is_boss_bar_visible"),
				" name=", hud.call("get_boss_bar_name"))
			_check(bool(hud.call("is_boss_bar_visible")), "boss bar shows on aggro")
			_check(String(hud.call("get_boss_bar_name")) == "赤瞳魔狼", "boss name displayed")
		4:
			_next = _t + 0.3
			# 直接造成伤害 → 血条数值更新（信号通路）
			var hp0 = float(_boss.get("current_health"))
			_boss.call("take_damage", 3.0, null)
			var hp1 = float(_boss.get("current_health"))
			print("[TEST] boss hp ", hp0, "->", hp1)
			_check(hp1 < hp0, "boss damaged")
			_check(bool(_hud().call("is_boss_bar_visible")), "bar still visible after damage")
		5:
			_next = _t + 0.5
			_boss.call("take_damage", 9999.0, null)
		6:
			_next = _t + 0.3
			_check(not bool(_hud().call("is_boss_bar_visible")), "boss bar hidden after death")
		7:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
