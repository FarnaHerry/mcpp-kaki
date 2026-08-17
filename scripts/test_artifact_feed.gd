# 法宝温养来源扩充 harness:
#   ①精英击杀 elite_killed → 全部已装备法宝 +tier×2（tier2=+4 / tier3=+6）
#   ②Boss 击杀 boss_died → 全部已装备法宝 +15（boss_fight_ended 不重复计）
#   ③服丹 item_used（丹药=炼丹产物）→ 本命 +1/颗；非丹药（仙桃）不触发
#   ④打坐 spiritual_energy_changed 且打坐中 → 本命 +0.1/次；非打坐不触发
#   ⑤真实打坐管线 ticks 也会温养（松散校验）
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _n0 := 0.0
var _n1 := 0.0

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

func _player():
	return root.find_child("Player", true, false)

func _ar():
	return _player().call("get_artifacts")

func _bus():
	return root.find_child("SignalBus", true, false)

func _benming_nurture() -> float:
	return float(_player().call("get_benming_nurture"))

func _slot1_nurture() -> float:
	return float(_ar().call("get_slot_info", 1).get("nurture"))

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var p = _player()
			_check(p != null, "player exists")
			_check(_bus() != null, "signal bus exists")
			var ar = _ar()
			_check(ar != null, "artifact system exists")
			ar.call("acquire", "fei_jian")
			ar.call("equip", 1, "fei_jian")       # 次要槽
			ar.call("acquire", "zhao_yao_hu")
			ar.call("equip", 0, "zhao_yao_hu")    # 本命槽
			_check(abs(_benming_nurture()) < 0.001, "benming nurture baseline 0")
			_check(abs(_slot1_nurture()) < 0.001, "slot1 nurture baseline 0")
			# 清场：防止附近敌人游走攻击打断打坐（受击收功）
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			# 关掉调试默认开启的突破无门槛——否则打坐 1s 自动请求机缘突破暂停整棵树
			p.call("get_cultivation").call("set_free_breakthrough", false)
		2:
			# 精英击杀 tier2 → 全部已装备 +4（tier×2）
			_next = _t + 0.3
			_bus().emit_signal("elite_killed", Vector2(0, 0), 2, 1)
			print("[TEST] after elite tier2: benming=", _benming_nurture(), " slot1=", _slot1_nurture())
			_check(abs(_benming_nurture() - 4.0) < 0.001, "elite tier2 nurtures benming +4")
			_check(abs(_slot1_nurture() - 4.0) < 0.001, "elite tier2 nurtures slot1 +4")
			# tier3 → 再 +6
			_bus().emit_signal("elite_killed", Vector2(0, 0), 3, 1)
			_check(abs(_benming_nurture() - 10.0) < 0.001, "elite tier3 nurtures +6 (total 10)")
			_check(abs(_slot1_nurture() - 10.0) < 0.001, "elite tier3 slot1 +6 (total 10)")
		3:
			# Boss 击杀 → 全部已装备 +15；boss_fight_ended 不得重复计
			_next = _t + 0.3
			_bus().emit_signal("boss_died")
			_check(abs(_benming_nurture() - 25.0) < 0.001, "boss_died nurtures benming +15 (total 25)")
			_check(abs(_slot1_nurture() - 25.0) < 0.001, "boss_died nurtures slot1 +15 (total 25)")
			_bus().emit_signal("boss_fight_ended", "测试Boss")
			_check(abs(_benming_nurture() - 25.0) < 0.001, "boss_fight_ended does NOT double-count")
			_check(abs(_slot1_nurture() - 25.0) < 0.001, "boss_fight_ended slot1 unchanged")
		4:
			# 服丹：回春丹（炼丹产物）→ 本命 +1；仙桃（天材地宝非丹药）不触发；聚气丹×2 → +2
			_next = _t + 0.3
			_bus().emit_signal("item_used", "healing_pill", 1)
			_check(abs(_benming_nurture() - 26.0) < 0.001, "pill nurtures benming +1 (total 26)")
			_check(abs(_slot1_nurture() - 25.0) < 0.001, "pill does not touch secondary slot")
			_bus().emit_signal("item_used", "xian_tao", 1)
			_check(abs(_benming_nurture() - 26.0) < 0.001, "xian_tao (not alchemy product) ignored")
			_bus().emit_signal("item_used", "qi_pill", 2)
			_check(abs(_benming_nurture() - 28.0) < 0.001, "pill x2 nurtures +2 (total 28)")
		5:
			# 非打坐状态：spiritual_energy_changed 不触发温养
			_next = _t + 0.3
			_check(not bool(_player().call("is_meditating")), "not meditating initially")
			_bus().emit_signal("spiritual_energy_changed", 0, 100, 0.0)
			_check(abs(_benming_nurture() - 28.0) < 0.001, "energy signal while idle does NOT nurture")
			# 凡人 cap 仅 9，打坐 1.8s 即封顶触发自动突破暂停树——先突破炼气（cap 99）再坐
			var cult = _player().call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			while int(cult.call("get_realm_index")) < 1:
				cult.call("attempt_breakthrough")
			cult.call("set_free_breakthrough", false)
			cult.call("set_spiritual_energy", 0.0) # 清空修为防封顶（未入坐，不触发温养）
			_check(abs(_benming_nurture() - 28.0) < 0.001, "breakthrough prep does NOT nurture")
		6:
			# 入坐（按住一帧再释放，action 轮询可靠）
			_next = _t + 0.2
			Input.action_press("cultivate")
		7:
			_next = _t + 0.2
			Input.action_release("cultivate")
		8:
			# 打坐中：同帧手动 emit ×3 → 精确 +0.3（同步回调，无真实 tick 插队）
			_next = _t + 0.3
			_check(bool(_player().call("is_meditating")), "meditating after cultivate")
			_n0 = _benming_nurture()
			_bus().emit_signal("spiritual_energy_changed", 1, 100, 0.01)
			_bus().emit_signal("spiritual_energy_changed", 2, 100, 0.02)
			_bus().emit_signal("spiritual_energy_changed", 3, 100, 0.03)
			_n1 = _benming_nurture()
			print("[TEST] meditate manual emits: ", _n0, " -> ", _n1)
			_check(abs(_n1 - _n0 - 0.3) < 0.01, "meditating energy signal nurtures +0.1/tick (x3)")
		9:
			# 真实打坐管线：坐 1.2s，真实 tick 也温养（速率>=5/s，松散校验增量 >0）
			_next = _t + 1.2
		10:
			_next = _t + 0.3
			var n2 = _benming_nurture()
			print("[TEST] after 1.2s real meditation: ", _n1, " -> ", n2)
			_check(n2 > _n1, "real meditation ticks nurture benming")
		11:
			# 移动收功 → 再 emit 不触发
			_next = _t + 0.2
			Input.action_press("right")
		12:
			_next = _t + 0.3
			Input.action_release("right")
		13:
			_next = _t + 0.3
			_check(not bool(_player().call("is_meditating")), "stopped meditating on move")
			var n3 = _benming_nurture()
			_bus().emit_signal("spiritual_energy_changed", 10, 100, 0.1)
			_check(abs(_benming_nurture() - n3) < 0.001, "energy signal after exiting meditate does NOT nurture")
		14:
			print("[TEST] DONE fail=", _fail)
			if _fail == 0:
				print("[TEST] ALL PASS")
			return true
	return false
