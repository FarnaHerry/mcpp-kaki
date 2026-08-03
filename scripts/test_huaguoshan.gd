# H3 harness: 东胜神洲补完（花果山+东海之滨+水帘洞）
# ①新物品注册 ②花果山内容（桃林/猿怪/水帘洞门户）③东海之滨（夜叉/神针铁）
# ④水帘洞进出+白猿老祖 ⑤残卷习得身外化身+施放buff ⑥地图延伸（6000 无墙）
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

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _db():
	return root.find_child("ItemDatabase", true, false)

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			# ①物品注册
			for id in ["xian_tao", "shen_wai_can_juan", "ding_hai_shen_zhen"]:
				var info = _db().call("get_item_info", id)
				_check(not info.is_empty(), "物品注册: " + id)
			var tao = _db().call("get_item_info", "xian_tao")
			_check(float(tao.get("heal_pct", 0.0)) == 0.5 and float(tao.get("energy_amount", 0.0)) == 300.0, "仙桃=50%回血+300修为")
			var scroll = _db().call("get_item_info", "shen_wai_can_juan")
			_check(String(scroll.get("learn_skill", "")) == "shen_wai_hua_shen", "残卷 learn_skill=身外化身")
			# ②③世界内容存在性
			_check(current_scene.find_child("YuanGuai0", true, false) != null, "花果山猿怪存在")
			_check(current_scene.find_child("XunHaiYeCha0", true, false) != null, "巡海夜叉存在")
			# ⑥地图延伸：传送到 6500，落地站稳（6000 旧墙已拆）
			_player().global_position = Vector2(6500, 100)
			_next = _t + 1.0
			_step = 1
		1:
			var pos = _player().global_position
			print("[TEST] player at ", pos)
			_check(pos.x > 6400.0 and pos.x < 6600.0, "6500 处无墙阻挡（未被弹回）")
			_check(pos.y > 100.0 and pos.y < 240.0, "花果山桃台/地面承托（落在 6500 桃台 y≈128）")
			# 传送东海之滨 8800
			_player().global_position = Vector2(8800, 100)
			_next = _t + 1.0
			_step = 2
		2:
			var pos2 = _player().global_position
			_check(pos2.x > 8700.0, "东海之滨 8800 可达")
			# ④水帘洞：传送门口，进洞
			_player().global_position = Vector2(7000, 210)
			_next = _t + 0.6 # 等 Area 重叠注册
			_step = 3
		3:
			_press("interact")
			_next = _t + 0.8
			_step = 4
		4:
			var room = current_scene.get_node_or_null("ShuiLianDong")
			_check(room != null, "水帘洞场景已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进洞")
			_check(room != null and room.find_child("BaiYuanLaoZu", true, false) != null, "白猿老祖在洞")
			# 捡残卷（石台）
			_player().position = Vector2(330, 182)
			_next = _t + 0.6
			_step = 5
		5:
			var inv = _player().call("get_inventory")
			_check(int(inv.call("get_item_count", "shen_wai_can_juan")) >= 1, "拾得身外化身残卷")
			# ⑤使用残卷 → 习得神通
			_check(_player().call("use_consumable", "shen_wai_can_juan"), "使用残卷")
			var skills = _player().call("get_skills")
			_check(bool(skills.call("is_known", "shen_wai_hua_shen")), "习得身外化身")
			# 化神+法则之力 → 装配 T 槽施放
			_breakthrough_to(5)
			_cult().call("set_law_power", 100.0)
			_check(bool(skills.call("assign", 6, "shen_wai_hua_shen")), "身外化身装配 T 槽")
			_check(bool(skills.call("cast_slot", 6)), "施放身外化身")
			var buffs = _player().call("get_buffs")
			_check(buffs != null and bool(buffs.call("has", "buff_shen_wai")), "毫毛助威 buff 生效")
			# 出洞：ExitPortal 在 (200,220)
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 6
		6:
			_press("interact")
			_next = _t + 0.8
			_step = 7
		7:
			_check(current_scene.get_node_or_null("ShuiLianDong") == null, "水帘洞已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到主场景")
			var pos3 = _player().global_position
			_check(abs(pos3.x - 7000.0) < 40.0, "出洞回到门口位置")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
