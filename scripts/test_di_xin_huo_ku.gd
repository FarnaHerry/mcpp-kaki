# 地心火窟秘境（西牛贺洲·火焰山下）：
# ①金丹 travel 西牛贺洲 ②入口 Portal/地标断言 ③进窟 ④敌人 id/数量/realm/精英化 + 岩浆 FireZone
# ⑤岩浆 dot 扣血 ⑥清场 + 灌死地心火麟 ⑦必掉离火珠入包 ⑧出窟回火焰山
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _drop: Node = null # 离火珠掉落实体（挂主场景根，跨房间拾取受父子同层限制——先出窟再拾）

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

func _room():
	return current_scene.get_node_or_null("DiXinHuoKu")

func _enemy_info(e: Node) -> String:
	return "%s(id=%s,realm=%d)" % [e.name, String(e.get("enemy_id")), int(e.get("realm"))]

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	paused = false

	match _step:
		0:
			# ①金丹解锁 + travel
			_player().call("get_cultivation").call("set_realm", 3)
			_check(bool(_cm().call("is_unlocked", "xiniuhe")), "金丹解锁西牛贺洲")
			_check(_cm().call("travel_to_direct", "xiniuhe"), "travel 西牛贺洲")
			_next = _t + 1.5
			_step = 1
		1:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("xiniuhe.tscn"), "到达西牛贺洲: " + sc)
			# ②入口 Portal（按提示语定位）+ 地标
			var portal: Node = null
			for c in current_scene.get_children():
				if c.get_class() == "Portal" and String(c.get("prompt_text")).contains("地心火窟"):
					portal = c
			_check(portal != null, "地心火窟入口 Portal 存在")
			if portal:
				_check(String(portal.get("prompt_text")) == "[↑] 进入地心火窟", "入口提示语正确")
				_check(abs(portal.position.x - 800.0) < 1.0, "入口位于火焰山 x=800")
				_check(Rect2(portal.get("room_bounds")) == Rect2(0, 0, 720, 270), "房间边界 720x270")
			var landmark := false
			for c in current_scene.get_children():
				if c is Label and String(c.text).contains("地心火窟"):
					landmark = true
			_check(landmark, "地心火窟地标存在")
			_player().global_position = Vector2(800, 210)
			_next = _t + 0.5
			_step = 2
		2:
			Input.action_press("up") # 入窟
			_next = _t + 0.3
			_step = 3
		3:
			Input.action_release("up")
			_next = _t + 1.0
			_step = 4
		4:
			# ③房间挂载 + 敌情断言
			var room = _room()
			_check(room != null, "地心火窟已挂载")
			_check(room != null and _player().get_parent() == room, "玩家已重挂载进窟")
			if room:
				for i in range(3):
					var e = room.find_child("HuoSuiShou%d" % i, true, false)
					_check(e != null and String(e.get("enemy_id")) == "huo_sui_shou" and int(e.get("realm")) == 4,
						"火髓兽%d 在场（%s）" % [i, _enemy_info(e) if e else "缺失"])
				for i in range(2):
					var e = room.find_child("RongYanKuiLei%d" % i, true, false)
					_check(e != null and String(e.get("enemy_id")) == "rong_yan_kui_lei" and int(e.get("realm")) == 5,
						"熔岩傀儡%d 在场（%s）" % [i, _enemy_info(e) if e else "缺失"])
				var elite = room.find_child("RongYanElite", true, false)
				_check(elite != null and int(elite.get("elite_tier")) == 1, "精英熔岩傀儡 elite_tier=1（厚甲）")
				var boss = room.find_child("DiXinHuoLin", true, false)
				_check(boss != null and String(boss.get("enemy_id")) == "di_xin_huo_lin", "地心火麟 Boss 在场")
				_check(boss != null and bool(boss.get("is_boss")), "地心火麟 is_boss")
				_check(boss != null and int(boss.get("realm")) == 5, "地心火麟 realm=5")
				# 岩浆 FireZone×3
				var pools := 0
				for i in range(3):
					var fz = room.get_node_or_null("MagmaPool%d" % i)
					if fz != null and fz.is_in_group("fire_zones"):
						pools += 1
				_check(pools == 3, "岩浆池 FireZone x%d（应3）" % pools)
			# 清场外廊+熔厅（防 dot 测试期被围殴致死）
			for n in ["HuoSuiShou0", "HuoSuiShou1", "HuoSuiShou2", "RongYanKuiLei0", "RongYanKuiLei1", "RongYanElite"]:
				var e = current_scene.find_child(n, true, false)
				if e:
					e.call("take_damage", 999999.0, _player())
			_next = _t + 0.6
			_step = 5
		5:
			# ⑤岩浆 dot：满血入池（站池底）→ 掉血
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			p.position = Vector2(205, 240) # MagmaPool0 池底
			_next = _t + 1.2
			_step = 6
		6:
			var p = _player()
			_check(float(p.call("get_current_health")) < float(p.call("get_max_health")),
				"岩浆 dot 扣血 (%.0f/%.0f)" % [p.call("get_current_health"), p.call("get_max_health")])
			# 出池回满，远程灌死 Boss
			p.position = Vector2(280, 210)
			p.call("set_current_health", p.call("get_max_health"))
			var boss = current_scene.find_child("DiXinHuoLin", true, false)
			_check(boss != null and float(boss.call("get_current_health")) > 0, "地心火麟存活待击")
			if boss:
				boss.call("take_damage", 999999.0, p)
			_next = _t + 0.8
			_step = 7
		7:
			var boss = current_scene.find_child("DiXinHuoLin", true, false)
			_check(boss == null or float(boss.call("get_current_health")) <= 0, "地心火麟已被击杀")
			# ⑥必掉离火珠：掉落实体存在（挂主场景根，房间坐标系与世界同原点）
			for c in current_scene.find_children("*", "Node2D", true, false):
				if c.get_class() == "ItemPickup" and String(c.get("item_id")) == "li_huo_zhu":
					_drop = c
			_check(_drop != null, "离火珠掉落实体存在（Boss 必掉）")
			# ⑦出窟（出口 Portal 在房间中央底部 x=360）
			_player().position = Vector2(360, 215)
			_next = _t + 0.5
			_step = 8
		8:
			Input.action_press("up")
			_next = _t + 0.3
			_step = 9
		9:
			Input.action_release("up")
			_next = _t + 1.0
			_step = 10
		10:
			_check(_room() == null, "地心火窟已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到西牛贺洲")
			var pos = _player().global_position
			_check(abs(pos.x - 800.0) < 40.0, "出窟回到入口位置 (x=%.0f)" % pos.x)
			# 贴身拾取离火珠（跨场景不拾取限制：需与掉落物同层）
			if is_instance_valid(_drop):
				_player().global_position = _drop.global_position
			_next = _t + 0.8
			_step = 11
		11:
			var inv = _player().call("get_inventory")
			_check(int(inv.call("get_item_count", "li_huo_zhu")) >= 1, "拾得离火珠（Boss 必掉）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
