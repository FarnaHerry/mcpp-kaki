# 洞天设施补全 harness:
# ①灵泉打坐点：贴近提示 → X 入坐（提示含聚灵阵倍率）→ 修为增长 → 再按 X 收功
# ②丹房：X 打开炼丹面板（暂停）→ 选回春丹 X 炼制成功话术 → ESC 关闭恢复
# ③灵植采集点×2：X 采集入包+枯萎 → 未刷新拒采 → debug 拨快复生
# ④采集点枯萎状态存档/读档持久化
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _energy_before := 0.0

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

func _dt():
	return root.find_child("DongtianManager", true, false)

func _gm():
	return root.find_child("GameManager", true, false)

func _inv():
	return _player().call("get_inventory")

func _panel():
	return root.find_child("PillLabPanel", true, false)

func _inv_count(id: String) -> int:
	return int(_inv().call("get_item_count", id))

func _clear_inv():
	var inv = _inv()
	for i in int(inv.call("get_capacity")):
		inv.call("set_slot", i, "", 0)

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

	match _step:
		0:
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			_cult().call("set_free_breakthrough", true)
			_cult().call("accumulate_energy", 100000000000)
			while int(_cult().call("get_realm_index")) < 6:
				_cult().call("attempt_breakthrough")
			_cult().call("set_free_breakthrough", false)
			_clear_inv()
			_step = 55 # 等能力解锁落地
		55:
			# 修为降到半管（避免封顶后打坐不增长 + 1s 自动请求突破干扰）
			var max_e := int(_cult().call("get_max_energy"))
			_cult().call("set_spiritual_energy", int(max_e * 0.5))
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == true, "进入洞天")
			# 传送到灵泉打坐点（与 dongtian.gd 摆点一致：415, 216）
			_player().global_position = Vector2(415, 212)
			_next = _t + 0.5 # 等落地（打坐需在地面）
			_step = 2
		2:
			_check(_hud_has("灵泉打坐"), "贴近提示：[X] 灵泉打坐（含聚灵阵倍率）")
			_check(_hud_has("聚灵阵×2.0"), "打坐点提示含聚灵阵×2.0（炼虚）")
			_energy_before = float(_cult().call("get_current_energy"))
			_press("interact") # X 入坐（模拟 cultivate 走既有打坐管线）
			_step = 3
		3:
			_check(bool(_player().call("is_meditating")), "X 入坐：进入打坐状态")
			_check(_hud_has("聚灵阵×"), "打坐中提示含聚灵阵倍率标识")
			_step = 4 # 打坐 0.3s（<1s，避免触发封顶自动突破请求）
		4:
			_check(float(_cult().call("get_current_energy")) > _energy_before, "打坐修为增长")
			_press("interact") # 再按 X 收功
			_step = 5
		5:
			_check(not bool(_player().call("is_meditating")), "再按 X 收功：退出打坐")
			# ---- 丹房 ----
			_inv().call("add_item", "zhi_xue_cao", 3)
			_player().global_position = Vector2(316, 212)
			_next = _t + 0.5
			_step = 6
		6:
			_check(_hud_has("丹房炼丹"), "贴近提示：[X] 丹房炼丹")
			_press("interact")
			_step = 7
		7:
			_check(_panel() != null and bool(_panel().call("is_open")), "X 打开丹房炼丹面板")
			_check(self.paused, "面板打开时暂停")
			# 选回春丹（healing_pill：止血草×3）——按 id 定位，防配方顺序变动
			var recipes: Array = _player().call("get_alchemy").call("get_recipe_list")
			var idx := -1
			for i in recipes.size():
				if String(recipes[i]["id"]) == "healing_pill":
					idx = i
					break
			_check(idx >= 0, "丹方列表含回春丹")
			_panel()._grid.call("set_selected", idx)
			_press("interact") # X 炼制
			_step = 8
		8:
			_check(_inv_count("healing_pill") == 1, "炼成回春丹入包")
			_check(_inv_count("zhi_xue_cao") == 0, "材料止血草 ×3 已消耗")
			_press("menu") # ESC 关闭面板
			_step = 9
		9:
			_check(not bool(_panel().call("is_open")), "ESC 关闭丹房面板")
			_check(not self.paused, "关闭后恢复非暂停（GameMenu 未抢 ESC）")
			# ---- 灵植采集点 0：聚灵草（浮空苗圃 296,176 台上；从上方落入防穿台）----
			_player().global_position = Vector2(296, 148)
			_next = _t + 0.5
			_step = 10
		10:
			_check(_hud_has("聚灵草"), "贴近提示：[X] 采集 ·聚灵草")
			_press("interact")
			_step = 11
		11:
			_check(_inv_count("ju_ling_cao") == 2, "采得聚灵草 ×2 入包")
			var s0: Dictionary = _dt().call("get_herb_spot", 0)
			_check(not bool(s0.get("available", true)), "采集后枯萎（available=false）")
			_check(int(s0.get("remaining", 0)) > 0, "刷新倒计时生效")
			_press("interact") # 枯萎期再采 → 拒采
			_step = 12
		12:
			_check(_inv_count("ju_ling_cao") == 2, "枯萎期拒采（数量不变）")
			_dt().call("debug_age_herb_spot", 0, 200.0) # 拨快 200s > 120s 刷新
			var s0b: Dictionary = _dt().call("get_herb_spot", 0)
			_check(bool(s0b.get("available", false)), "拨快后灵植复生（available=true）")
			# ---- 灵植采集点 1：千年灵芝（420,168 台上）----
			_player().global_position = Vector2(420, 140)
			_next = _t + 0.5
			_step = 13
		13:
			_check(_hud_has("千年灵芝"), "贴近提示：[X] 采集 ·千年灵芝")
			_press("interact")
			_step = 14
		14:
			_check(_inv_count("qian_nian_ling_zhi") == 1, "采得千年灵芝 ×1 入包")
			# ---- 存档往返：枯萎状态持久化 ----
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 15
		15:
			_check(_dt().call("is_inside") == false, "读档后已退出洞天")
			var s0c: Dictionary = _dt().call("get_herb_spot", 0)
			_check(bool(s0c.get("available", false)), "读档后聚灵草点保持复生（已拨快）")
			var s1c: Dictionary = _dt().call("get_herb_spot", 1)
			_check(not bool(s1c.get("available", true)), "读档后千年灵芝点保持枯萎")
			_check(_inv_count("qian_nian_ling_zhi") == 1, "读档后背包灵芝保留")
			return _finish()
	return false
