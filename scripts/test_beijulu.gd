# 北俱芦洲测试：①渡劫解锁+travel ②冰面打滑（IceZone）③极寒（减速+冰伤）
# ④玄冰窟秘境（进洞→龙骨/玄冰参秘藏→出洞）⑤上古巨兽 Boss+龙骨遗骸
# ⑥炼体圣地 buff ⑦南天门序章+玄龙丹配方（渡劫解锁）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

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

func _cm():
	return root.find_child("ContinentManager", true, false)

func _find(s: String):
	return root.find_child(s, true, false)

func _has_label(text: String) -> bool:
	var labels = root.find_children("*", "Label", true, false)
	for l in labels:
		if l.is_visible_in_tree() and text in String(l.text):
			return true
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 60:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			# 渡劫解锁 + travel
			_player().call("get_cultivation").call("set_realm", 9)
			_next = _t + 0.3
		2:
			_check(bool(_cm().call("is_unlocked", "beijulu")), "渡劫解锁北俱芦洲")
			_check(_cm().call("travel_to_direct", "beijulu"), "travel 北俱芦洲")
			_next = _t + 1.5
		3:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("beijulu.tscn"), "到达北俱芦洲: " + sc)
			# 冰面滑：站上冰原（IceZone 600,234 覆盖 x200~1000）
			var p = _player()
			p.global_position = Vector2(600, 230)
			_next = _t + 0.5
		4:
			var p = _player()
			_check(bool(p.call("is_slippery")), "极北冰原冰面打滑标记")
			# 出冰原（x<200 出 IceZone 左缘）→ 恢复
			p.global_position = Vector2(120, 230)
			_next = _t + 0.5
		5:
			var p = _player()
			_check(not bool(p.call("is_slippery")), "离开冰原恢复")
			# 极寒：满血进极寒区（ColdZone 1800,234 覆盖 x1500~2100）
			p.call("set_current_health", p.call("get_max_health"))
			p.global_position = Vector2(1800, 230)
			_next = _t + 1.5
		6:
			var p = _player()
			_check(bool(p.call("is_chilled")), "玄冰高原极寒减速标记")
			_check(float(p.call("get_current_health")) < float(p.call("get_max_health")), "极寒冰伤扣血 (%.0f/%.0f)" % [p.call("get_current_health"), p.call("get_max_health")])
			# 出极寒区 → 恢复
			p.global_position = Vector2(1400, 230)
			_next = _t + 0.5
		7:
			var p = _player()
			_check(not bool(p.call("is_chilled")), "离开极寒区恢复")
			# 玄冰窟秘境：入口
			p.global_position = Vector2(1950, 220)
			_next = _t + 0.5
		8:
			Input.action_press("up")
		9:
			Input.action_release("up")
			_next = _t + 1.0
		10:
			# 玩家已重挂载进玄冰窟（房间是子节点，current_scene 仍是 beijulu）
			var p = _player()
			_check(p.get_parent() != current_scene, "玩家进入玄冰窟（父节点≠洲根）")
			# 秘藏：龙骨 + 玄冰参 + 灵石
			p.position = Vector2(200, 190)
			_next = _t + 0.6
		11:
			var p = _player()
			var inv = p.call("get_inventory")
			_check(int(inv.call("get_item_count", "long_gu")) >= 2, "玄冰窟秘藏龙骨×2")
			_check(int(inv.call("get_item_count", "xuan_bing_shen")) >= 2, "玄冰窟秘藏玄冰参×2")
			# 出洞（出口在房间底中央）
			p.position = Vector2(200, 210)
			_next = _t + 0.5
		12:
			Input.action_press("up")
		13:
			Input.action_release("up")
			_next = _t + 1.0
		14:
			var p = _player()
			_check(p.get_parent() == current_scene, "出洞回北俱芦洲")
			# 上古巨兽 Boss + 遗骸龙骨
			_check(_find("Boss_XuanMing") != null, "上古巨兽·玄冥 守关")
			p.global_position = Vector2(2950, 230)
			_next = _t + 0.8
		15:
			var p = _player()
			var inv = p.call("get_inventory")
			_check(int(inv.call("get_item_count", "long_gu")) >= 3, "巨兽遗骸拾得龙骨（累计×3）")
			# 炼体圣地：贴近 X 炼体
			p.global_position = Vector2(3200, 215)
			_next = _t + 0.4
		16:
			Input.action_press("interact")
		17:
			Input.action_release("interact")
			_next = _t + 0.4
		18:
			var p = _player()
			var buffs = p.call("get_buffs")
			_check(bool(buffs.call("has", "buff_lianti")), "炼体圣地 → 炼体 buff（防+20%）")
			# 南天门序章 + 玄龙丹配方（渡劫解锁）
			_check(_has_label("南天门"), "南天门序章地标存在")
			var alch = p.call("get_alchemy")
			var found = false
			var realm_ok = false
			var realm_locked = true
			for r in alch.call("get_recipe_list"):
				if String(r["id"]) == "xuan_long_dan":
					found = true
					realm_ok = int(r["min_realm"]) == 9
					realm_locked = bool(r["realm_locked"])
			_check(found, "炼丹页有玄龙丹配方")
			_check(realm_ok, "玄龙丹门槛渡劫 (min_realm=9)")
			_check(not realm_locked, "渡劫已解锁玄龙丹")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
