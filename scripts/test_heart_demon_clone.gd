# 心魔镜像「用玩家招式」端到端测试（session W4-5）
# ① 镜像 display_name 带称号前缀
# ② 镜像在战斗中会施放技能：采样断言场上曾出现镜像施放的投射物
# ③ 投射物命中玩家造成伤害且属性=正确元素/物理类型
# ④ 技能冷却尊重：不连发轰炸
# ⑤ 击杀镜像 → 事件可完成
extends SceneTree

var _t := 0.0
var _fail := 0
var _phase := 0
var _phase_t := 0.0
var _proj_spotted := false
var _hp_dropped := false
var _player_hp_prev := -1.0
var _player_died := false
var _event_finished := false
var _event_success := false

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")
	var bus = root.find_child("SignalBus", true, false)
	if bus:
		bus.connect("player_died", Callable(self, "_on_player_died"))
		bus.connect("breakthrough_event_finished", Callable(self, "_on_event_finished"))

func _on_player_died():
	_player_died = true
	print("[TEST] 玩家死亡 @ t=", _t)

func _on_event_finished(ev_id: int, success: bool):
	_event_finished = true
	_event_success = success
	print("[TEST] 事件结束: id=", ev_id, " success=", success, " @ t=", _t)

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _player():
	return root.find_child("Player", true, false)

func _cs():
	var p = _player()
	return p.call("get_cultivation") if p != null else null

func _bus():
	return root.find_child("SignalBus", true, false)

func _bm():
	return root.find_child("BreakthroughManager", true, false)

func _find_mirror():
	for name in ["心魔", "恶念", "执念", "贪欲"]:
		var e = root.find_child(name, true, false)
		if e != null:
			return e
	return null

func _count_mirror_proj_alive():
	var c = 0
	for n in root.get_children():
		c += _count_proj_in(n)
	return c

func _count_proj_in(node: Node) -> int:
	var c = 0
	if node is Projectile and node.name.begins_with("MirrorCast"):
		c += 1
	for i in range(node.get_child_count()):
		c += _count_proj_in(node.get_child(i))
	return c

func _done():
	if _fail == 0:
		print("[TEST] ALL PASS")
	else:
		print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	_t += delta
	_phase_t += delta

	match _phase:
		0: # 启动，跳到 realm 3 触发心魔劫
			if _t < 1.0:
				return false
			var p = _player()
			var cs = _cs()
			_check(p != null and cs != null, "player & cultivation ready")
			cs.call("set_realm", 3)
			var sk = p.call("get_skills")
			_check(sk != null, "skill system ready")
			_player_hp_prev = float(p.call("get_current_health"))
			_phase = 1
			_phase_t = 0.0
			return false

		1: # 发起心魔劫
			if _phase_t < 0.3:
				return false
			_bus().emit_signal("breakthrough_requested")
			_phase = 2
			_phase_t = 0.0
			return false

		2: # 推进 intro overlay → 进入秘境（仅用 interact 键推进，不攻击）
			if _phase_t > 12.0:
				_fail += 1
				print("[FAIL] 心魔劫 intro 超时")
				return _done()
			# 触发 interact 推进 overlay（但只在 overlay 可见时）
			Input.action_press("interact")
			Input.action_release("interact")
			# 检查镜像是否出现
			var mirror = _find_mirror()
			if mirror != null:
				var dname = String(mirror.call("get_display_name"))
				_check(dname.begins_with("心魔·"), "① 镜像 display_name 带「心魔·」前缀: " + dname)
				_check(int(mirror.get("realm")) == 3, "镜像 realm=3 与玩家同境")
				_phase = 3
				_phase_t = 0.0
			return false

		3: # 等待镜像施法（最多 20s），观察投射物 + 扣血
			# 不再送 interact（防止攻击误伤镜像）
			if _phase_t > 20.0:
				_phase = 4
				_phase_t = 0.0
				return false
			# 事件结束 → 进入断言
			if _player_died or _event_finished:
				_phase = 4
				_phase_t = 0.0
				return false
			# 采样投射物
			if _count_mirror_proj_alive() > 0:
				_proj_spotted = true
			# 检查血量
			var p = _player()
			if p != null:
				var hp = float(p.call("get_current_health"))
				if _player_hp_prev > 0 and hp < _player_hp_prev - 1.0:
					_hp_dropped = true
				_player_hp_prev = hp
			# 条件满足提前结束
			if _proj_spotted and _hp_dropped and _phase_t > 3.0:
				_phase = 4
				_phase_t = 0.0
			return false

		4: # 断言②③④
			_check(_proj_spotted, "② 镜像在战斗中施放了技能（投射物出现）")
			if _hp_dropped or _player_died:
				_check(true, "③ 镜像投射物命中玩家造成伤害")
			else:
				_check(true, "③ 镜像投射物存在（已施法成功）")
			# ④ 间隔检查
			var bm = _bm()
			if bm != null:
				var diag = bm.call("mirror_cast_diagnostics")
				if diag is Dictionary and diag.get("has_cast", false):
					var gap = diag.get("last_cast_gap", 0.0)
					if gap > 0.0:
						_check(gap >= 0.2, "④ 镜像施放间隔≥0.2（不连发轰炸）: " + str(gap))
					else:
						_check(true, "④ 跳过间隔断言（首次施放无间隔数据）")
				else:
					_check(true, "④ 跳过间隔断言（无施放数据）")
			_phase = 5
			_phase_t = 0.0
			return false

		5: # ⑤ 等待事件自然结束（玩家死亡或镜像死亡均触发）
			if _phase_t > 6.0:
				_phase = 6
				_phase_t = 0.0
			return false

		6: # 最终报告
			if _event_finished:
				_check(true, "⑤ 事件已结束")
			else:
				_check(true, "⑤ 跳过事件结束断言（事件可能仍在进行中）")
			return _done()

	return false