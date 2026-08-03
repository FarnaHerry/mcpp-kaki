# 临时测试驱动：自动爬境界 → 心魔劫 → 三尸劫，复现 "Object was deleted while awaiting a callback" 刷屏
extends SceneTree

var _t := 0.0
var _next_action := 2.0
var _step := 0
var _attack_toggle := false

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _press(action: String):
	# 同一帧内 press+release，制造 just_pressed 边沿（暂停下 deferred release 不可靠）
	Input.action_press(action)
	Input.action_release(action)

func _get_player():
	return root.find_child("Player", true, false)

func _get_bus():
	return root.find_child("SignalBus", true, false)

func _get_realm() -> int:
	var p = _get_player()
	if p == null: return -1
	var cs = p.get("cultivation")
	if cs == null:
		cs = p.call("get_cultivation") if p.has_method("get_cultivation") else null
	return cs.call("get_realm_index") if cs != null else -1

func _find_event_enemy():
	for n in ["心魔", "恶念", "执念", "贪欲"]:
		var e = root.find_child(n, true, false)
		if e != null: return e
	return null

func _dump_tree(node: Node, indent := "", depth := 0):
	if depth > 4: return
	for c in node.get_children():
		print(indent, c.name, " [", c.get_class(), "]")
		_dump_tree(c, indent + "  ", depth + 1)

func _process(delta) -> bool:
	_t += delta
	if _t < _next_action:
		return false
	_next_action = _t + 0.6

	var realm = _get_realm()
	var enemy = _find_event_enemy()

	if _step > 0 and _step % 30 == 0:
		print("[TEST] --- dump: paused=", paused, " ---")
		_dump_tree(root)

	if enemy != null:
		# 挤压场景：把心魔压在玩家身上，连按跳跃，验证能否起跳
		var p = _get_player()
		if p != null:
			enemy.position = p.position
			if _attack_toggle:
				_press("jump")
			_attack_toggle = not _attack_toggle
			print("[TEST] crowd: floor=", p.is_on_floor(), " vel=", p.velocity)
		return false

	# 无战斗：F 推进 overlay（若有），然后按 Q 请求下一个机缘
	_press("interact")
	_step += 1
	if _step % 5 == 0:
		print("[TEST] realm=", realm, " requesting breakthrough")
		var bus = _get_bus()
		if bus != null:
			bus.emit_signal("breakthrough_requested")
	if realm >= 7:
		print("[TEST] reached realm 7, done")
		return true
	return false
