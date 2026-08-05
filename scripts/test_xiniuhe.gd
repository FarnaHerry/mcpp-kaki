# 西牛贺洲测试：①金丹解锁+travel ②火焰山环境火伤 ③芭蕉扇灭火 ④弱水禁飞
# ⑤斜月三星洞秘境（进洞→菩提心法奖励→出洞）
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
			# 金丹解锁 + travel
			_player().call("get_cultivation").call("set_realm", 3)
			_next = _t + 0.3
		2:
			_check(bool(_cm().call("is_unlocked", "xiniuhe")), "金丹解锁西牛贺洲")
			_check(_cm().call("travel_to_direct", "xiniuhe"), "travel 西牛贺洲")
			_next = _t + 1.5
		3:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("xiniuhe.tscn"), "到达西牛贺洲: " + sc)
			var p = _player()
			# 火焰山火伤：满血进火区 → 掉血
			p.call("set_current_health", p.call("get_max_health"))
			var zones = get_nodes_in_group("fire_zones")
			_check(zones.size() >= 3, "火焰山有环境火区 (x%d)" % zones.size())
			if zones.size() > 0:
				var pos = zones[0].global_position
				p.global_position = Vector2(pos.x, 220)
			_next = _t + 1.5
		4:
			var p = _player()
			_check(p.call("get_current_health") < p.call("get_max_health"), "火焰山环境火伤扣血 (%.0f/%.0f)" % [p.call("get_current_health"), p.call("get_max_health")])
			# 芭蕉扇拾取 + 使用 → 得法宝
			p.call("set_current_health", p.call("get_max_health"))
			p.global_position = Vector2(1350, 114)
			_next = _t + 0.6
		5:
			var p = _player()
			var inv = p.call("get_inventory")
			_check(int(inv.call("get_item_count", "ba_jiao_shan")) >= 1, "拾得芭蕉扇")
			_check(p.call("use_consumable", "ba_jiao_shan"), "使用芭蕉扇 → 得法宝")
			var arts = p.call("get_artifacts")
			_check(bool(arts.call("is_owned", "ba_jiao_shan")), "法宝芭蕉扇已获得")
			# 装备并祭出（风刃 + 灭火）
			arts.call("equip", 1, "ba_jiao_shan")
			arts.call("activate_slot", 1)
			_next = _t + 0.6
		6:
			# 灭火：所有火区熄灭
			var zones = get_nodes_in_group("fire_zones")
			var all_out = true
			for z in zones:
				if not bool(z.call("is_extinguished")):
					all_out = false
			_check(all_out, "芭蕉扇祭出扇灭环境火")
			# 弱水禁飞
			var p = _player()
			p.global_position = Vector2(2700, 150)
			_next = _t + 0.5
		7:
			var p = _player()
			_check(bool(p.call("is_flight_blocked")), "弱水区禁飞标记")
			_check(not bool(p.call("can_fly")), "弱水区 can_fly 失效")
			# 出弱水区 → 恢复
			p.global_position = Vector2(2400, 200)
			_next = _t + 0.5
		8:
			var p = _player()
			_check(not bool(p.call("is_flight_blocked")), "离开弱水区恢复飞行")
			# 三星洞秘境：入口
			p.global_position = Vector2(1950, 205)
			_next = _t + 0.5
		9:
			Input.action_press("up")
		10:
			Input.action_release("up")
			_next = _t + 1.0
		11:
			# 玩家已重挂载进三星洞（房间是子节点，current_scene 仍是 xiniuhe）
			var p = _player()
			_check(p.get_parent() != current_scene, "玩家进入三星洞（父节点≠洲根）")
			# 捡菩提心法残卷
			p.position = Vector2(200, 182)
			_next = _t + 0.6
		12:
			var p = _player()
			var inv = p.call("get_inventory")
			_check(int(inv.call("get_item_count", "pu_ti_xin_fa_juan")) >= 1, "拾得菩提心法残卷")
			_check(p.call("use_consumable", "pu_ti_xin_fa_juan"), "参悟残卷")
			var skills = p.call("get_skills")
			_check(bool(skills.call("is_known", "pu_ti_xin_fa")), "习得被动·菩提心法")
			_check(abs(float(skills.call("get_passive_elem_resist")) - 0.10) < 0.001, "全元素抗性 +10%")
			# 出洞（出口在房间底中央）
			p.position = Vector2(200, 215)
			_next = _t + 0.5
		13:
			Input.action_press("up")
		14:
			Input.action_release("up")
			_next = _t + 1.0
		15:
			var p = _player()
			_check(p.get_parent() == current_scene, "出洞回西牛贺洲")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
