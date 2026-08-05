# G1 harness: ①地形节点存在（平台/墙/远侧地面/检查点）②单向平台可站立
#      ③沟壑缺口（3900~4400 无地面，谷底 y=420）④各区敌人/BOSS/草药摆点
extends SceneTree

var _t := 0.0
var _next := 0.0
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

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			# 地形存在性
			for n in ["Platform_1400_190", "Platform_2300_104", "Wall_2650_130", "Wall_2750_90",
			          "Wall_3894_238", "Wall_4406_238", "Ground_3900_4400", "Ground_4400_9000",
			          "Wall_9000_40", "Platform_5600_90",
			          "Checkpoint_1320", "Checkpoint_2820", "Checkpoint_4450", "Checkpoint_5250",
			          "Checkpoint_6200", "Checkpoint_8200"]:
				_check(root.find_child(n, true, false) != null, "terrain node: " + n)
			# 主地面已扩到 3900（不到 4400，沟壑留缺）
			var g = root.find_child("Ground", true, false)
			_check(g != null and abs(g.position.x - 1950.0) < 1.0, "main ground centered 1950 (0~3900)")
		2:
			_next = _t + 0.8
			# 单向平台可站立：玩家放到竹台上方，物理沉降后应停在台面（不穿）
			var p = root.find_child("Player", true, false)
			p.global_position = Vector2(1400, 160) # Platform_1400_190 上方
			p.set("velocity", Vector2.ZERO)
		3:
			_next = _t + 1.5 # 同时是谷低落体窗口（~1.0s 落地）
			var p = root.find_child("Player", true, false)
			print("[TEST] player y after drop: ", p.global_position.y)
			_check(p.global_position.y < 200.0, "one-way platform holds player (y<200)")
			# 谷底：放到沟壑中央无平台处，应落到 y=420 谷底（而不是 238）
			p.global_position = Vector2(4100, 260)
			p.set("velocity", Vector2.ZERO)
		4:
			_next = _t + 1.5 # 谷底自由落体 ~1.0s
			var p = root.find_child("Player", true, false)
			print("[TEST] player y in pit: ", p.global_position.y)
			_check(p.global_position.y > 395.0, "pit floor at y~420 (y>395)")
		5:
			_next = _t + 0.3
			# 摆点：各区敌人
			for n in ["ZhuYao1", "YaXiao1", "YaGong1", "YanGui1", "GuXiao0", "LeiShou", "GuTu1", "Boss_ChiLong"]:
				_check(root.find_child(n, true, false) != null, "enemy spawned: " + n)
			var chi = root.find_child("Boss_ChiLong", true, false)
			_check(String(chi.call("get_display_name")) == "幽谷螭龙", "boss display name 幽谷螭龙")
			_check(float(chi.get("max_health")) == 300.0, "boss hp 300（平衡：30→300）")
		6:
			_next = _t + 0.3
			# 草药：新增点位（悟道茶×2/赤焰花×2/冰心莲新增/金刚藤新增）
			var herbs = []
			for c in current_scene.get_children():
				if c.has_method("get_herb_id"):
					herbs.append(String(c.call("get_herb_id")))
			print("[TEST] herbs: ", herbs)
			_check(herbs.count("wu_dao_cha") == 3, "wu_dao_cha ×3 (悟道崖2+花果山1)")
			_check(herbs.count("chi_yan_hua") == 2, "chi_yan_hua ×2 (幽谷底)")
			_check(herbs.count("bing_xin_lian") >= 3, "bing_xin_lian ≥3 (旧1+竹台2)")
			_check(herbs.count("jin_gang_teng") >= 3, "jin_gang_teng ≥3 (旧1+绝壁2)")
		7:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
