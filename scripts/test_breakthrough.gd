# 临时测试驱动：自动爬境界 → 心魔劫 → 三尸劫，复现 "Object was deleted while awaiting a callback" 刷屏
#
# 已知真实游戏 bug（2026-09-13 护栏化时确认，本测试如实 FAIL 报出，勿为绿删断言）：
#   战斗秘境（心魔劫/三尸劫）中玩家战死 → event_finished(ok=false) 时存活劫敌随 arena 释放，
#   但 BreakthroughManager._enemies_alive 不归零（_start_event/_finish 均不重置，仅 _spawn_wave++ /
#   _on_event_enemy_died--）；重试事件首波杀完后 _wave_check 因 _enemies_alive>0 永远早退
#   → 事件永久卡 active，再无波次，realm 不再推进。
#   信号轨迹实证：event_started id=5 → 恶念 killed → player_died → event_finished ok=false
#   → 重开 id=5 → 恶念 killed → 卡死（src/cultivation/breakthrough_manager.cpp:479/714/884/896/902）。
extends SceneTree

var _t := 0.0
var _next_action := 2.0
var _step := 0
var _attack_toggle := false
var _crowd_ticks := 0
var _stall_ticks := 0
var _fail := 0
var _realm_checked := {}

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

var _traced := false
func _hook_trace():
	if _traced:
		return
	var bus = _get_bus()
	if bus == null:
		return
	_traced = true
	bus.connect("player_died", func(): print("[TRACE] player_died"))
	bus.connect("enemy_killed", func(e, k): print("[TRACE] enemy_killed ", e))
	bus.connect("breakthrough_event_started", func(id): print("[TRACE] event_started id=", id))
	bus.connect("breakthrough_event_finished", func(id, ok): print("[TRACE] event_finished id=", id, " ok=", ok))

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
	_hook_trace()
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
			_crowd_ticks = 0 # 每只劫敌独立计时（预存修复：原累计不归零，多波后误触硬上限）
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
		var cs = _get_player().call("get_cultivation") if _get_player() != null else null
		var energy := int(cs.call("get_current_energy")) if cs != null else -1
		var mx := int(cs.call("get_max_energy")) if cs != null else -1
		print("[TEST] realm=", realm, " energy=", energy, "/", mx, " requesting breakthrough")
		var bus = _get_bus()
		if bus != null:
			bus.emit_signal("breakthrough_requested")

	# ---- 护栏（预存修复：realm=5 处曾无限重复请求不终结）----
	# 卡死形态：无劫敌可打 + bm 事件仍 active + 玩家挂在秘境内 → waves 不推进。
	# 已定位为真实游戏 bug（非 harness 假象）：战斗秘境中玩家死亡触发 _fail_cleanup/_finish(false)
	# 时，存活的波次敌人随 arena 释放但 _enemies_alive 不归零（_start_event/_finish 均不重置），
	# 重试事件首波杀完后 _wave_check 因 _enemies_alive>0 永远早退 → 事件永久卡 active。
	# （src/cultivation/breakthrough_manager.cpp：_enemies_alive 仅 884++/896--，902 早退）
	var bm = root.find_child("BreakthroughManager", true, false)
	var p = _get_player()
	var pparent: String = String(p.get_parent().name) if p != null else "?"
	if bm != null and bool(bm.call("is_active")) and pparent != "Main":
		_stall_ticks += 1
	else:
		_stall_ticks = 0
	if _stall_ticks > 40: # ~24s 无波次推进 → 判卡死，如实 FAIL 并收束
		_fail += 1
		print("[FAIL] 机缘事件卡死：无劫敌且事件 active（", _stall_ticks, " ticks 无波次推进）")
		print("[FAIL]   玩家父节点=", pparent, " realm=", realm,
			" —— 真实 bug：玩家秘境战死后 _enemies_alive 未归零，重试 _wave_check 永远早退（见文件头注释）")
		print("[TEST] ", _fail, " FAILURES")
		return true
	if _step > 500: # 总步进硬上限（~5 分钟），杜绝任何形态的悬挂
		_fail += 1
		print("[FAIL] 超时：", _step, " 步仍未到 realm 7（当前 realm=", realm, " bm_active=",
			bm.call("is_active") if bm != null else "?", " player_parent=", pparent, "）")
		print("[TEST] ", _fail, " FAILURES")
		return true

	if realm >= 7:
		if _fail == 0:
			print("[TEST] reached realm 7, done — ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true
	return false
