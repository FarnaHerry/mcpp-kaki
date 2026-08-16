# 机缘突破事件数据化 + 失败惩罚细化 + 心魔镜像 端到端测试
# ① 事件定义来自 events.json（overlay 标题/正文与 JSON 条目逐项比对——JSON 即真源）
# ② 叙事事件全流程（引气入体 intro → outro → realm 0→1）
# ③ 心魔劫战败 → 道心不稳 debuff（攻-5% 防-5% 300s，经 BuffSystem）
# ④ 心魔镜像：display_name = 心魔·<玩家称号>；本体色 = 玩家角色颜色
extends SceneTree

var _t := 0.0
var _fail := 0
var _phase := 0
var _phase_t := 0.0
var _press_cd := 0.0
var _holding := false
var _events := []
var _atk_before := 0.0
var _def_before := 0.0

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	var f = FileAccess.open("res://data/events.json", FileAccess.READ)
	_events = JSON.parse_string(f.get_as_text())
	print("[TEST] main scene loaded, events.json entries=", _events.size())

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _event_for(realm: int) -> Dictionary:
	for e in _events:
		if int(e["realm"]) == realm:
			return e
	return {}

func _player():
	return root.find_child("Player", true, false)

func _cs():
	var p = _player()
	return p.call("get_cultivation") if p != null else null

func _buffs():
	var p = _player()
	return p.call("get_buffs") if p != null else null

func _bus():
	return root.find_child("SignalBus", true, false)

func _overlay() -> CanvasLayer:
	var bm = root.find_child("BreakthroughManager", true, false)
	if bm == null:
		return null
	for c in bm.get_children():
		if c is CanvasLayer:
			return c
	return null

func _overlay_visible() -> bool:
	var o = _overlay()
	return o != null and o.visible

func _labels() -> Array:
	var o = _overlay()
	if o == null:
		return []
	var out = []
	for c in o.get_children():
		if c is Label:
			out.append(c)
	return out # [title, body, hint]（创建顺序）

func _press_interact():
	# 按住一帧再释放（同帧 press+release 对 just_pressed 轮询不可靠）
	Input.action_press("interact")
	_holding = true

func _advance_overlay():
	if _overlay_visible() and _press_cd <= 0.0:
		_press_interact()
		_press_cd = 0.35

func _fail_out(msg: String) -> bool:
	_fail += 1
	print("[FAIL] ", msg)
	print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	_t += delta
	_phase_t += delta
	_press_cd -= delta
	if _holding:
		Input.action_release("interact")
		_holding = false

	match _phase:
		0: # 启动 + JSON 数据基本断言 + 发起叙事突破（realm 0）
			if _t < 1.0:
				return false
			_check(_player() != null and _cs() != null, "player & cultivation ready")
			var ev0 = _event_for(0)
			var ev3 = _event_for(3)
			_check(not ev0.is_empty() and ev0.get("name", "") == "引气入体", "events.json realm0 = 引气入体")
			_check(not ev3.is_empty() and int(ev3.get("kind", -1)) == 1 and ev3.get("name", "") == "心魔劫", "events.json realm3 = 心魔劫 kind1")
			_check(int(_cs().call("get_realm_index")) == 0, "start at realm 0")
			_bus().emit_signal("breakthrough_requested")
			_phase = 1
			_phase_t = 0.0
			return false

		1: # 叙事 intro overlay 出现，标题/正文以 JSON 为准
			if _phase_t < 0.3:
				return false
			if not _overlay_visible():
				return _fail_out("叙事 intro overlay 未出现")
			var ev0 = _event_for(0)
			var labels = _labels()
			_check(labels.size() >= 2, "overlay labels present")
			if labels.size() >= 2:
				_check(labels[0].text == String(ev0["name"]), "overlay 标题来自 JSON: " + labels[0].text)
				_check(labels[1].text == String(ev0["intro"][0]), "overlay 首行 intro 来自 JSON: " + labels[1].text)
			_advance_overlay()
			_phase = 2
			_phase_t = 0.0
			return false

		2: # 逐行推进 intro → outro → 境界提升
			if _phase_t > 20.0:
				return _fail_out("叙事事件超时未完结")
			_advance_overlay()
			var realm = int(_cs().call("get_realm_index"))
			if realm == 1 and not _overlay_visible():
				_check(true, "叙事事件全流程完结：realm 0→1，overlay 关闭")
				# 阶段 3：心魔劫（realm 3）——先记录攻防基线
				_cs().call("set_realm", 3)
				_atk_before = float(_player().call("get_effective_attack"))
				_def_before = float(_player().call("get_effective_defense"))
				_bus().emit_signal("breakthrough_requested")
				_phase = 3
				_phase_t = 0.0
			return false

		3: # 心魔劫 intro overlay
			if _phase_t < 0.3:
				return false
			if not _overlay_visible():
				return _fail_out("心魔劫 intro overlay 未出现")
			var ev3 = _event_for(3)
			var labels = _labels()
			if labels.size() >= 2:
				_check(labels[0].text == String(ev3["name"]), "心魔劫标题来自 JSON: " + labels[0].text)
				_check(labels[1].text == String(ev3["intro"][0]), "心魔劫首行 intro 来自 JSON")
			_advance_overlay()
			_phase = 4
			_phase_t = 0.0
			return false

		4: # 推进 intro → 进入战斗秘境
			if _phase_t > 15.0:
				return _fail_out("心魔劫秘境超时未开启")
			_advance_overlay()
			var mirror = root.find_child("心魔", true, false)
			if mirror != null:
				# ④ 镜像断言
				var title = String(_cs().call("get_full_title"))
				var dname = String(mirror.call("get_display_name"))
				print("[TEST] 镜像 display_name: ", dname, " 称号: ", title)
				_check(dname.begins_with("心魔·"), "镜像显示名带「心魔·」前缀")
				_check(title.is_empty() or dname.contains(title), "镜像显示名含玩家称号")
				# 本体色 = 玩家角色颜色（最后一个 Polygon2D 子节点是本体，前一个是辉光）
				var p_poly = _player().get_node_or_null("Polygon2D")
				if p_poly != null:
					var polys = []
					for c in mirror.get_children():
						if c is Polygon2D:
							polys.append(c)
					if polys.size() >= 1:
						var body_c: Color = polys[polys.size() - 1].color
						var pc: Color = p_poly.color
						_check(abs(body_c.r - pc.r) < 0.01 and abs(body_c.g - pc.g) < 0.01 and abs(body_c.b - pc.b) < 0.01,
							"镜像本体色=玩家色 " + str(body_c))
				# 战败：一击致命（秘境内玩家父节点非场景根，不触发勾魂使）
				_player().call("take_damage", 999999.0, null)
				_phase = 5
				_phase_t = 0.0
			return false

		5: # 战败清理（idle 帧 _fail_cleanup）→ 道心不稳 debuff
			if _phase_t < 1.0:
				return false
			var buffs = _buffs()
			_check(buffs.call("has", "buff_dao_xin_bu_wen"), "心魔劫战败 → 道心不稳 buff 生效")
			_check(abs(float(buffs.call("get_atk_mult")) - 0.95) < 0.001, "攻 -5%（atk_mult 0.95）")
			_check(abs(float(buffs.call("get_def_mult")) - 0.95) < 0.001, "防 -5%（def_mult 0.95）")
			var atk_after = float(_player().call("get_effective_attack"))
			var def_after = float(_player().call("get_effective_defense"))
			_check(atk_after < _atk_before, "有效攻击下降 %.1f→%.1f" % [_atk_before, atk_after])
			# 无装备/功法时防御基线为 0（0×0.95=0），严格下降断言仅在基线>0 时成立；
			# 防御侧由上方 get_def_mult 0.95 断言覆盖
			if _def_before > 0.01:
				_check(def_after < _def_before, "有效防御下降 %.1f→%.1f" % [_def_before, def_after])
			else:
				_check(true, "有效防御基线为 0，跳过严格下降断言")
			_check(int(_cs().call("get_realm_index")) == 3, "战败境界不变（仍金丹）")
			_phase = 6
			_phase_t = 0.0
			return false

		6: # 重生后 debuff 仍在（不被重生清掉）
			if _phase_t > 6.0:
				return _fail_out("重生超时")
			var p = _player()
			if p != null and float(p.call("get_current_health")) > 0.0 and not paused:
				_check(_buffs().call("has", "buff_dao_xin_bu_wen"), "重生后道心不稳仍存续")
				if _fail == 0:
					print("[TEST] ALL PASS")
				else:
					print("[TEST] ", _fail, " FAILURES")
				return true
			return false

	return false
