# 验证: ①凡人起步武技装槽 ②武技施放+冷却 ③技能挥击实际伤害敌人 ④炼气授予法术
#      ⑤法术耗灵+投射物+冷却 ⑥技能存档往返
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _enemy = null
var _enemy_hp0 := 0.0

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

func _find_enemy():
	var best = null
	var best_d := 1e9
	for n in root.find_children("*", "Enemy", true, false):
		var d = abs(n.global_position.x - 480.0)
		if d < best_d:
			best_d = d
			best = n
	return best

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			_check(p != null, "player exists")
			var sk = p.call("get_skills")
			_check(sk != null, "skill system exists")
			var s0 = sk.call("get_slot_info", 0)
			var s1 = sk.call("get_slot_info", 1)
			print("[TEST] slot A: ", s0.get("id"), " slot S: ", s1.get("id"))
			_check(String(s0.get("id")) == "po_kong_zhan", "slot A = po_kong_zhan (starter martial)")
			_check(String(s1.get("id")) == "tu_jin_zhan", "slot S = tu_jin_zhan")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var ok1 = bool(sk.call("cast_slot", 0))
			var ok2 = bool(sk.call("cast_slot", 0))
			print("[TEST] cast A: first=", ok1, " immediate recast=", ok2)
			_check(ok1, "martial cast succeeds")
			_check(not ok2, "cooldown blocks immediate recast")
		3:
			# 技能挥击伤害：把玩家挪到敌人旁，面朝，再施放（冷却3s已过）
			_next = _t + 3.2
			var p = root.find_child("Player", true, false)
			_enemy = _find_enemy()
			_check(_enemy != null, "enemy found")
			if _enemy:
				_enemy_hp0 = float(_enemy.get("current_health"))
				p.global_position = _enemy.global_position + Vector2(-20, 0)
				print("[TEST] enemy hp0=", _enemy_hp0, " player at ", p.global_position)
		4:
			_next = _t + 0.6
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var ok = bool(sk.call("cast_slot", 0)) # 破空斩 ×2.5
			_check(ok, "martial recast after cooldown")
		5:
			_next = _t + 0.3
			if _enemy and is_instance_valid(_enemy):
				var hp = float(_enemy.get("current_health"))
				print("[TEST] enemy hp after skill: ", hp, " (was ", _enemy_hp0, ")")
				_check(hp < _enemy_hp0, "skill swing damaged enemy")
			else:
				print("[TEST] enemy died from skill (freed)")
				_check(true, "skill swing killed enemy")
		6:
			# 突破到炼气 → 授予法术
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 1000000000)
			cult.call("attempt_breakthrough")
			print("[TEST] realm idx=", cult.call("get_realm_index"))
			_check(int(cult.call("get_realm_index")) >= 1, "reached QI_REFINING")
		7:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var s2 = sk.call("get_slot_info", 2)
			var s3 = sk.call("get_slot_info", 3)
			print("[TEST] slot D: ", s2.get("id"), " slot F: ", s3.get("id"))
			_check(String(s2.get("id")) == "huo_dan_shu", "slot D = huo_dan_shu (granted at QI_REFINING)")
			_check(String(s3.get("id")) == "bing_zhui_shu", "slot F = bing_zhui_shu")
		8:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			var sk = p.call("get_skills")
			cult.call("set_mana", 100.0)
			var mana0 = float(cult.call("get_mana"))
			var proj0 = root.find_children("*", "Projectile", true, false).size()
			var ok = bool(sk.call("cast_slot", 2))
			var mana1 = float(cult.call("get_mana"))
			var proj1 = root.find_children("*", "Projectile", true, false).size()
			print("[TEST] spell cast=", ok, " mana ", mana0, "->", mana1, " projectiles ", proj0, "->", proj1)
			_check(ok, "spell cast succeeds")
			_check(mana1 < mana0, "spell consumed mana")
			_check(proj1 > proj0, "projectile spawned")
			_check(not bool(sk.call("cast_slot", 2)), "spell cooldown blocks recast")
		9:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var d = sk.call("save_to_dict")
			sk.call("load_from_dict", d)
			var s0 = sk.call("get_slot_info", 0)
			var s2 = sk.call("get_slot_info", 2)
			_check(String(s0.get("id")) == "po_kong_zhan", "save/load martial slot roundtrip")
			_check(String(s2.get("id")) == "huo_dan_shu", "save/load spell slot roundtrip")
		10:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
