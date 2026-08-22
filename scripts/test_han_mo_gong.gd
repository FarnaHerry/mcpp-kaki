# 寒墨行宫秘境测试（北俱芦洲·北方海域）：
# ①渡劫+travel 北俱芦洲 ②入口 Portal（target_scene/prompt）③↑ 进行宫
# ④敌情装配（墨鲛×2/realm9、寒渊龟×2/realm9、精英墨鲛·噬灵、寒渊君 Boss realm9 ×5 血）
# ⑤灌死 Boss → 命名表 han_mo_gong 必掉玄冥归元丹+玄冰髓 ⑥↑ 出宫回北俱入口
# ⑦玄冥归元丹 buff_xuan_ming 服用生效
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _qty_before := 0

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

func _find(s: String):
	return root.find_child(s, true, false)

func _find_hmg_portal():
	for p in root.find_children("*", "Portal", true, false):
		if String(p.get("target_scene")).ends_with("han_mo_gong.tscn"):
			return p
	return null

func _find_pickup(item_id: String):
	var stack := [current_scene]
	while not stack.is_empty():
		var n = stack.pop_back()
		for c in n.get_children():
			stack.push_back(c)
		var v = n.get("item_id")
		if (typeof(v) == TYPE_STRING_NAME or typeof(v) == TYPE_STRING) and String(v) == item_id:
			return n
	return null

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	paused = false

	match _step:
		0:
			# ①渡劫解锁 + travel 北俱芦洲
			_player().call("get_cultivation").call("set_realm", 9)
			var cm = _find("ContinentManager")
			_check(cm != null, "ContinentManager 存在")
			_check(bool(cm.call("travel_to_direct", "beijulu")), "travel 北俱芦洲")
			_next = _t + 1.5
			_step = 1
		1:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("beijulu.tscn"), "到达北俱芦洲: " + sc)
			# ②入口 Portal
			var portal = _find_hmg_portal()
			_check(portal != null, "寒墨行宫入口 Portal 存在")
			if portal:
				_check(String(portal.get("prompt_text")) == "[↑] 进入寒墨行宫", "入口提示: " + String(portal.get("prompt_text")))
			_check(_find("Boss_XuanMing") != null, "既有玄冥 Boss 未受影响")
			_player().global_position = Vector2(4950, 210)
			_next = _t + 0.6
			_step = 2
		2:
			# ③↑ 进行宫
			Input.action_press("up")
			_next = _t + 0.2
			_step = 3
		3:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 4
		4:
			# ④行宫挂载 + 敌情装配
			var room = current_scene.get_node_or_null("HanMoGong")
			_check(room != null, "寒墨行宫已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进行宫")
			if room:
				for i in range(2):
					var mj = room.find_child("HanMoJiao%d" % i, true, false)
					_check(mj != null and int(mj.call("get_realm")) == 9, "墨鲛%d 在场 realm9" % i)
				for i in range(2):
					var hg = room.find_child("HanYuanGui%d" % i, true, false)
					_check(hg != null and int(hg.call("get_realm")) == 9, "寒渊龟%d 在场 realm9" % i)
					if hg:
						_check(float(hg.call("get_max_health")) >= 1925.0, "寒渊龟血量 >= 1925（实际 %.0f）" % float(hg.call("get_max_health")))
						_check(float(hg.call("get_move_speed")) <= 40.0, "寒渊龟低速 <= 40（实际 %.0f）" % float(hg.call("get_move_speed")))
				var elite = room.find_child("HanMoJiaoElite", true, false)
				_check(elite != null, "精英墨鲛守殿")
				if elite:
					_check(int(elite.get("elite_tier")) == 1, "精英化 tier=1")
					_check(String(elite.get("affix_id")) == "shi_ling", "噬灵词缀 shi_ling")
				var boss = room.find_child("Boss_HanYuanJun", true, false)
				_check(boss != null, "寒渊君 Boss 在场")
				if boss:
					_check(bool(boss.get("is_boss")), "寒渊君 is_boss")
					_check(int(boss.call("get_realm")) == 9, "寒渊君 realm9")
					_check(abs(float(boss.call("get_max_health")) - 6875.0) < 0.5, "Boss ×5 血量 6875（实际 %.0f）" % float(boss.call("get_max_health")))
			_next = _t + 0.3
			_step = 5
		5:
			# ⑤灌死 Boss → 命名表 han_mo_gong 必掉玄冥归元丹 + 玄冰髓
			var boss = _find("Boss_HanYuanJun")
			_check(boss != null and float(boss.call("get_current_health")) > 0, "寒渊君存活待击")
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			if boss:
				boss.call("take_damage", 999999.0, p)
			_next = _t + 0.6
			_step = 6
		6:
			var boss2 = _find("Boss_HanYuanJun")
			_check(boss2 == null or float(boss2.call("get_current_health")) <= 0, "寒渊君已被击杀")
			# 秘藏掉落（命名表 han_mo_gong + _on_boss_died 双保险）
			var pk_dan = _find_pickup("xuan_ming_dan")
			_check(pk_dan != null, "玄冥归元丹秘藏掉落生成（命名表 han_mo_gong 必掉）")
			var pk_sui = _find_pickup("xuan_bing_sui")
			_check(pk_sui != null, "玄冰髓秘藏掉落生成（命名表 han_mo_gong 必掉）")
			# 隐藏秘藏：极品灵石（宫心高台）
			var pk_peak = _find_pickup("spirit_stone_peak")
			_check(pk_peak != null, "隐藏秘藏极品灵石存在")
			# ⑥出宫（exit portal 在 room_bounds 中心底部 = 240, 220）
			_player().position = Vector2(240, 200)
			_next = _t + 0.6
			_step = 7
		7:
			Input.action_press("up")
			_next = _t + 0.2
			_step = 8
		8:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 9
		9:
			_check(current_scene.get_node_or_null("HanMoGong") == null, "行宫已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到北俱芦洲")
			var pos = _player().global_position
			_check(abs(pos.x - 4950.0) < 40.0, "出宫回到寒墨行宫入口旁（x=%.0f）" % pos.x)
			# 拾取玄冥归元丹验证入包
			var pk3 = _find_pickup("xuan_ming_dan")
			if pk3:
				_player().global_position = pk3.global_position
			_next = _t + 0.6
			_step = 10
		10:
			var inv = _player().call("get_inventory")
			_check(int(inv.call("get_item_count", "xuan_ming_dan")) >= 1, "拾得玄冥归元丹入包")
			# ⑦服用玄冥归元丹 → buff_xuan_ming 生效
			_qty_before = int(inv.call("get_item_count", "xuan_ming_dan"))
			_check(_qty_before >= 1, "服用前有玄冥归元丹（%d）" % _qty_before)
			_player().call("use_consumable", "xuan_ming_dan")
			_next = _t + 0.5
			_step = 11
		11:
			var p = _player()
			var inv = p.call("get_inventory")
			var buffs = p.call("get_buffs")
			_check(bool(buffs.call("has", "buff_xuan_ming")), "服用玄冥归元丹 → buff_xuan_ming 生效")
			if buffs.call("has", "buff_xuan_ming"):
				var list = buffs.call("get_active_list")
				for b in list:
					if String(b["id"]) == "buff_xuan_ming":
						_check(abs(float(b["remaining"]) - 900.0) < 1.0, "buff_xuan_ming 时长 900s（实际 %.0f）" % float(b["remaining"]))
			# 攻防加成验证（渡劫玩家生命基础非 0 才断言；防御极值可能为 0 因境界未推满）
			var atk = float(p.call("get_effective_attack"))
			_check(atk > 0.0, "服用后攻击力正常 %.0f" % atk)
			# 物品消耗验证
			var qty_after = int(inv.call("get_item_count", "xuan_ming_dan"))
			_check(qty_after < _qty_before, "玄冥归元丹消耗（%d → %d）" % [_qty_before, qty_after])
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false