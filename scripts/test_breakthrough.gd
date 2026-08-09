# 临时测试驱动：自动爬境界 → 心魔劫 → 三尸劫，复现 "Object was deleted while awaiting a callback" 刷屏
extends SceneTree

var _t := 0.0
var _next_action := 2.0
var _step := 0
var _attack_toggle := false
var _crowd_ticks := 0
var _fail := 0
var _realm_checked := {}

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
		# 回归断言：心魔/三尸 realm 必须=玩家当前 realm（否则被威压 V 直接慑服，劫数虚设）
		var eid = enemy.get_instance_id()
		if not _realm_checked.has(eid):
			_realm_checked[eid] = true
			if int(enemy.get("realm")) == realm:
				print("[PASS] 劫敌 realm 与玩家同境: ", realm)
			else:
				_fail += 1
				print("[FAIL] 劫敌 realm=", enemy.get("realm"), " 玩家 realm=", realm)
		# 挤压场景：把心魔压在玩家身上，跳跃+攻击连按——攻击给击杀路径，验证战斗可否终结
		var p = _get_player()
		if p != null:
			enemy.position = p.position
			_press("attack")
			if _attack_toggle:
				_press("jump")
			_attack_toggle = not _attack_toggle
			print("[TEST] crowd: floor=", p.is_on_floor(), " vel=", p.velocity)
		_crowd_ticks += 1
		if _crowd_ticks > 120:
			# 硬上限：战斗若无法自然终结（心魔死或玩家死），判失败收束，杜绝 suite 悬挂
			_fail += 1
			print("[TEST] crowd FAIL: enemy survived ", _crowd_ticks, " ticks")
			print("[TEST] ", _fail, " FAILURES")
			return true
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
		if _fail == 0:
			print("[TEST] reached realm 7, done — ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true
	return false
