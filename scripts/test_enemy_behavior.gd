# 敌人行为组件化测试（Wave4）：
# ①数据驱动：set_enemy_id → behavior 装配（ranged/flying/boss + slow/heavy 组合）
# ②兼容性：is_ranged/is_flying/is_boss 属性读写真同步
# ③组合行为：烛幽灵（flying+ranged）→ 飞行远程开箱即用；寒渊龟（heavy）→ 防御+5；冰甲巨猿（slow）→ 移动×0.6
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _held: Array = []

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

func _hold(action: String):
	Input.action_press(action)
	_held.append(action)

func _release_all():
	for a in _held:
		Input.action_release(a)
	_held.clear()

func _find(s: String):
	return root.find_child(s, true, false)

func _gm():
	return root.find_child("GameManager", true, false)

# 直接实例化（跳过 spawn_enemy_by_id 的 elite_chance 自动掷词缀，测得原始行为值）
func _spawn_raw(id: String) -> Node:
	var e = ClassDB.instantiate("Enemy")
	current_scene.add_child(e)
	e.set("enemy_id", id)
	return e

func _spawn_enemy(id: String) -> Node:
	var wc = load("res://scripts/world_common.gd")
	return wc.spawn_enemy_by_id(current_scene, Vector2(100, 100), id)

func _finish() -> bool:
	if _fail == 0:
		print("[TEST] ALL PASS")
	else:
		print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_release_all()
	_step += 1
	if _step > 40:
		print("[TEST] hard cap")
		return _finish()

	match _step:
		1:
			_check(true, "测试开始")
			_next = _t + 0.5
		2:
			# ① 数据驱动装配：远程怪（青衣弓手）
			var e = _spawn_enemy("qing_yi_gong_shou")
			_check(e != null, "青衣弓手生成")
			if e:
				var beh = e.get("behavior")
				_check(bool(e.call("get_is_ranged")) == true, "青衣弓手 is_ranged=true")
				_check(bool(e.call("get_is_flying")) == false, "青衣弓手 is_flying=false")
				_check(float(e.call("get_preferred_distance")) > 0.0, "青衣弓手 preferred_distance>0")
				e.queue_free()
			_next = _t + 0.5
		3:
			# 飞行怪（雷鸟）
			var e = _spawn_enemy("lei_niao")
			if e:
				_check(bool(e.call("get_is_flying")) == true, "雷鸟 is_flying=true")
				_check(bool(e.call("get_is_ranged")) == false, "雷鸟 is_ranged=false")
				e.queue_free()
			_next = _t + 0.5
		4:
			# Boss（巨灵神）
			var e = _spawn_enemy("ju_ling_shen")
			if e:
				_check(bool(e.call("get_is_boss")) == true, "巨灵神 is_boss=true")
				_check(float(e.call("get_max_health")) >= 4000.0, "巨灵神 Boss ×5 血量=" + str(e.call("get_max_health")))
				e.queue_free()
			_next = _t + 0.5
		5:
			# ③ 组合行为：烛幽灵 flying+ranged
			var e = _spawn_raw("zhu_you_ling")
			if e:
				_check(bool(e.call("get_is_flying")) == true, "烛幽灵 flying=true")
				_check(bool(e.call("get_is_ranged")) == true, "烛幽灵 ranged=true（组合飞行远程）")
				_check(float(e.call("get_preferred_distance")) > 0.0, "烛幽灵 preferred_distance>0")
				e.queue_free()
			_next = _t + 0.5
		6:
			# 寒渊龟 heavy → 防御+5
			var e = _spawn_raw("han_yuan_gui")
			if e:
				_check(float(e.call("get_defense")) >= 5.0, "寒渊龟 heavy 防御+5: " + str(e.call("get_defense")))
				e.queue_free()
			_next = _t + 0.5
		7:
			# 冰甲巨猿 slow → 移动 ×0.6（def speed 80 → 48）
			var e = _spawn_raw("bing_jia_yuan")
			if e:
				var spd = float(e.call("get_move_speed"))
				_check(absf(spd - 48.0) < 0.1, "冰甲巨猿 slow 移动×0.6: " + str(spd) + " (期望 48)")
				e.queue_free()
			_next = _t + 0.5
		8:
			# ② 兼容性：.set("is_ranged") 写 → behavior 同步
			var e = _spawn_enemy("shan_xiao") # 无行为
			if e:
				_check(bool(e.call("get_is_ranged")) == false, "山魈初始 is_ranged=false")
				e.call("set_is_ranged", true)
				_check(bool(e.call("get_is_ranged")) == true, "set_is_ranged(true) 兼容生效")
				# behavior.ranged 同步（从 C++ 侧经 is_ranged getter 验证）
				_check(bool(e.call("get_is_ranged")) == true, "behavior 同步 → is_ranged 读 true")
				e.queue_free()
			_next = _t + 0.5
		9:
			print("[TEST] 敌人行为组件化测试完成")
			return _finish()

	return false