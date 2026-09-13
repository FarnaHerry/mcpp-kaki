# 洞天「灵兽闯阵」+ 灵植采集点扩充 harness:
# ①debug_suppress 进入不触发 ②debug_force 进入触发（1~2 只，realm=玩家-1，HUD 提示）
# ③闯阵中出洞天 → 清场不持久化 ④再进入全灭 → 「洞天重归安宁」提示 + 掉落表生效（有 ItemPickup）
# ⑤新采集点×2（冰心莲/赤焰花）：采集 → 枯萎拒采 → debug 拨快复生
# ⑥存档往返：枯萎状态持久化 + 闯阵状态不持久化
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
# 条件轮询（预存修复：固定时长等待改条件等待，消除采集/掉落时序竞态）
var _poll := Callable()
var _poll_deadline := 0.0
# 采集计数基线（见 step 7 注释：掉落草药磁吸污染绝对计数）
var _base_bxl := 0
var _base_cyh := 0
var _saved_bxl := 0
var _saved_cyh := 0

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

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _gm():
	return root.find_child("GameManager", true, false)

func _dt():
	return root.find_child("DongtianManager", true, false)

func _inv():
	return _player().call("get_inventory")

func _inv_count(id: String) -> int:
	return int(_inv().call("get_item_count", id))

func _invaders() -> Array:
	return get_nodes_in_group("dongtian_invaders")

func _dongtian_scene():
	return root.find_child("Dongtian", true, false)

func _pickup_count() -> int:
	var scene = _dongtian_scene()
	if scene == null:
		return 0
	return scene.find_children("*", "ItemPickup", true, false).size()

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

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
	_next = _t + 0.3

	# 条件轮询：条件未满足且未超时则原地等下一拍；超时放行由后续断言如实报 FAIL
	if _poll.is_valid():
		if _t < _poll_deadline and not _poll.call():
			return false
		_poll = Callable()

	match _step:
		0:
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			_cult().call("set_free_breakthrough", true)
			_cult().call("accumulate_energy", 100000000000)
			while int(_cult().call("get_realm_index")) < 6:
				_cult().call("attempt_breakthrough")
			_cult().call("set_free_breakthrough", false)
			_step = 55 # 等能力解锁落地
		55:
			# 修为降到半管（避免封顶自动请求突破干扰）
			var max_e := int(_cult().call("get_max_energy"))
			_cult().call("set_spiritual_energy", int(max_e * 0.5))
			# ---- ① suppress 路径：进入不触发 ----
			_dt().call("debug_suppress_invasion")
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == true, "suppress 后进入洞天")
			_check(bool(_dt().call("is_invasion_active")) == false, "suppress 路径：闯阵未触发")
			_check(_invaders().is_empty(), "suppress 路径：无入侵者")
			_press("dongtian") # 退出
			_step = 2
		2:
			_check(_dt().call("is_inside") == false, "退出洞天")
			# ---- ② force 路径：进入必触发 ----
			_dt().call("debug_force_invasion")
			_press("dongtian")
			_step = 3
		3:
			_check(_dt().call("is_inside") == true, "force 后进入洞天")
			_check(bool(_dt().call("is_invasion_active")) == true, "force 路径：闯阵触发")
			var left := int(_dt().call("get_invaders_left"))
			_check(left >= 1 and left <= 2, "入侵者数量 1~2（实际 %d）" % left)
			var inv := _invaders()
			_check(inv.size() == left, "dongtian_invaders 组大小==剩余数")
			var realm_ok := true
			var id_ok := true
			for e in inv:
				if int(e.get("realm")) != 5: # 玩家炼虚(6) → 入侵者 realm=5
					realm_ok = false
				if not ["qie_ling_shu", "tan_ling_feng"].has(String(e.get("enemy_id"))):
					id_ok = false
			_check(realm_ok, "入侵者 realm=玩家-1（5）")
			_check(id_ok, "入侵者为窃灵鼠/贪灵蜂")
			_check(_hud_has("有灵兽闯入洞天"), "HUD 提示「有灵兽闯入洞天！」")
			# ---- ③ 闯阵中出洞天 → 清场 ----
			_press("dongtian")
			_step = 4
		4:
			_check(_dt().call("is_inside") == false, "闯阵中退出洞天")
			_check(bool(_dt().call("is_invasion_active")) == false, "出洞天闯阵状态清除")
			_check(_invaders().is_empty(), "出洞天入侵者清场（不持久化）")
			# 再次 force 进入，这次全灭
			_dt().call("debug_force_invasion")
			_press("dongtian")
			_step = 5
		5:
			_check(bool(_dt().call("is_invasion_active")) == true, "再次进入闯阵触发")
			for e in _invaders():
				e.call("take_damage", 99999.0, null)
			_step = 6
		6:
			_check(bool(_dt().call("is_invasion_active")) == false, "全灭后闯阵结束")
			_check(int(_dt().call("get_invaders_left")) == 0, "全灭后剩余数归零")
			_check(_hud_has("洞天重归安宁"), "全灭后提示「洞天重归安宁」")
			_next = _t + 0.5 # 掉落生成走 call_deferred，多等一拍
			_step = 7
		7:
			# 两表各有 chance=1.0 条目，每次击杀必掉 ≥1 个
			_check(_pickup_count() >= 1, "击杀掉落表生效（ItemPickup 出现 %d 个）" % _pickup_count())
			# 预存修复（间歇 FAIL 根因）：窃灵鼠掉落表含 bing_xin_lian/chi_yan_hua（各 20%），
			# 玩家炼虚纳戒磁吸 150px 自动拾取 → 污染后续采集「×1 入包」绝对计数断言。
			# 这里清掉全部掉落物，并以当前计数为基线，后续一律增量断言。
			var scene = _dongtian_scene()
			if scene != null:
				for pk in scene.find_children("*", "ItemPickup", true, false):
					pk.queue_free()
			_base_bxl = _inv_count("bing_xin_lian")
			_base_cyh = _inv_count("chi_yan_hua")
			# ---- ⑤ 新采集点 2：冰心莲（灵田左端上空 84,168；从上方落入防穿台）----
			_player().global_position = Vector2(84, 140)
			# 条件等待：掉落物清完 + 玩家落台贴近出提示（替代原固定 0.5s）
			_poll = func(): return _pickup_count() == 0 and _hud_has("冰心莲")
			_poll_deadline = _t + 3.0
			_step = 8
		8:
			_check(_hud_has("冰心莲"), "贴近提示：[X] 采集 ·冰心莲")
			_press("interact")
			# 条件等待入包（未采到则补按，替代固定一拍）
			_poll = func():
				if _inv_count("bing_xin_lian") >= _base_bxl + 1:
					return true
				_press("interact")
				return false
			_poll_deadline = _t + 2.0
			_step = 9
		9:
			_check(_inv_count("bing_xin_lian") == _base_bxl + 1, "采得冰心莲 ×1 入包（增量）")
			var s2: Dictionary = _dt().call("get_herb_spot", 2)
			_check(not bool(s2.get("available", true)), "冰心莲采集后枯萎")
			_press("interact") # 枯萎期再采 → 拒采
			_step = 10
		10:
			_check(_inv_count("bing_xin_lian") == _base_bxl + 1, "冰心莲枯萎期拒采（数量不变）")
			_dt().call("debug_age_herb_spot", 2, 400.0) # 拨快 400s > 300s 刷新
			var s2b: Dictionary = _dt().call("get_herb_spot", 2)
			_check(bool(s2b.get("available", false)), "冰心莲拨快后复生")
			# ---- 新采集点 3：赤焰花（灵田右端上空 176,156）----
			_player().global_position = Vector2(176, 128)
			# 条件等待：玩家落台贴近出提示（替代原固定 0.5s）
			_poll = func(): return _hud_has("赤焰花")
			_poll_deadline = _t + 3.0
			_step = 11
		11:
			_check(_hud_has("赤焰花"), "贴近提示：[X] 采集 ·赤焰花")
			_press("interact")
			_poll = func():
				if _inv_count("chi_yan_hua") >= _base_cyh + 1:
					return true
				_press("interact")
				return false
			_poll_deadline = _t + 2.0
			_step = 12
		12:
			_check(_inv_count("chi_yan_hua") == _base_cyh + 1, "采得赤焰花 ×1 入包（增量）")
			var s3: Dictionary = _dt().call("get_herb_spot", 3)
			_check(not bool(s3.get("available", true)), "赤焰花采集后枯萎")
			# ---- ⑥ 存档往返：枯萎持久化 + 闯阵不持久化 ----
			# 记录存档前计数（与 save 同帧，无物理帧穿插），读档后逐项比对
			_saved_bxl = _inv_count("bing_xin_lian")
			_saved_cyh = _inv_count("chi_yan_hua")
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 13
		13:
			_check(_dt().call("is_inside") == false, "读档后已退出洞天")
			var s2c: Dictionary = _dt().call("get_herb_spot", 2)
			_check(bool(s2c.get("available", false)), "读档后冰心莲点保持复生（已拨快）")
			var s3c: Dictionary = _dt().call("get_herb_spot", 3)
			_check(not bool(s3c.get("available", true)), "读档后赤焰花点保持枯萎")
			_check(_inv_count("bing_xin_lian") == _saved_bxl, "读档后冰心莲保留")
			_check(_inv_count("chi_yan_hua") == _saved_cyh, "读档后赤焰花保留")
			_check(bool(_dt().call("is_invasion_active")) == false, "读档后闯阵状态不持久化")
			_check(_invaders().is_empty(), "读档后无入侵者")
			return _finish()
	return false
