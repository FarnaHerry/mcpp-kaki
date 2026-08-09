# 东海龙宫秘境（design/world-map.md 东胜神洲补完）：
# ①物品注册 ②避水珠水抗实测（主场景数值稳定；房间内受击炼体会漂移 max_health）
# ③入口进洞 ④守卫存在+弱水禁飞 ⑤击杀镇守将 ⑥秘藏拾取+珍珠修为 ⑦出洞
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _e_entry := 0

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

func _press(a: String):
	Input.action_press(a)
	Input.action_release(a)

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

func _equip(item_id: String) -> bool:
	var p = _player()
	var inv = p.call("get_inventory")
	inv.call("add_item", item_id, 1)
	for i in range(inv.call("get_capacity")):
		var sd = inv.call("get_slot", i)
		if not sd.is_empty() and String(sd["id"]) == item_id:
			return bool(p.call("equip_item", i))
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			# ①物品注册
			for id in ["bi_shui_zhu", "qian_nian_zhen_zhu"]:
				var info = _db().call("get_item_info", id)
				_check(not info.is_empty(), "物品注册: " + id)
			var bsz = _db().call("get_item_info", "bi_shui_zhu")
			_check(int(bsz.get("type", -1)) == 3, "避水珠=装备")
			var zz = _db().call("get_item_info", "qian_nian_zhen_zhu")
			_check(float(zz.get("energy_amount", 0.0)) == 2000.0, "千年珍珠=修为+2000")
			# 金丹（会飞）→ 弱水禁飞断言才有意义
			_breakthrough_to(3)
			# ②避水珠水抗实测（主场景；房间内受击炼体会漂移 maxh，不作精确断言）
			_check(_equip("bi_shui_zhu"), "装备避水珠")
			var p = _player()
			var maxh = float(p.call("get_max_health"))
			p.call("set_current_health", maxh)
			p.call("take_damage_typed", 50.0, 2, 3, null) # DMG_ELEMENTAL, ELEM_SHUI
			var loss = maxh - float(p.call("get_current_health"))
			_check(abs(loss - 40.0) < 0.6, "避水珠水抗：50→%.1f（应40）" % loss)
			p.call("set_current_health", maxh)
			p.call("take_damage_typed", 50.0, 2, 4, null) # ELEM_HUO 不受益
			var loss2 = maxh - float(p.call("get_current_health"))
			_check(abs(loss2 - 50.0) < 0.6, "火伤不受益：50→%.1f（应50）" % loss2)
			p.call("set_current_health", maxh)
			# 传送龙宫门口
			_player().global_position = Vector2(8600, 210)
			_next = _t + 0.6
			_step = 1
		1:
			_press("up") # 入龙宫
			_next = _t + 0.8
			_step = 2
		2:
			var room = current_scene.get_node_or_null("LongGong")
			_check(room != null, "东海龙宫已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进洞")
			_check(room != null and room.find_child("XiaBing0", true, false) != null, "虾兵在洞")
			_check(room != null and room.find_child("XieJiang", true, false) != null, "蟹将精英在洞")
			_check(room != null and room.find_child("LongGongZhenShou", true, false) != null, "镇守将守关")
			# 出生点(60,200)在弱水走廊内：金丹可飞但此处禁飞
			_check(not bool(_player().call("can_fly")), "弱水走廊禁飞（can_fly false）")
			_e_entry = int(_cult().call("get_current_energy"))
			# 走出走廊（200,220）：恢复可飞
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 3
		3:
			_check(bool(_player().call("can_fly")), "离开弱水走廊恢复飞行")
			var boss = current_scene.find_child("LongGongZhenShou", true, false)
			_check(boss != null and float(boss.call("get_current_health")) > 0, "镇守将存活待击")
			boss.call("take_damage", 999999.0, _player())
			_player().position = Vector2(380, 182)
			_next = _t + 0.6
			_step = 4
		4:
			var boss2 = current_scene.find_child("LongGongZhenShou", true, false)
			_check(boss2 == null or float(boss2.call("get_current_health")) <= 0, "镇守将已被击杀")
			_player().position = Vector2(405, 182)
			_next = _t + 0.6
			_step = 5
		5:
			# ⑥秘藏：避水珠入包；珍珠低能量自动服用 → 修为增幅（或留库存）
			var inv = _player().call("get_inventory")
			_check(int(inv.call("get_item_count", "bi_shui_zhu")) >= 1, "拾得避水珠")
			var e1 = int(_cult().call("get_current_energy"))
			var zhu_cnt = int(inv.call("get_item_count", "qian_nian_zhen_zhu"))
			_check(e1 - _e_entry >= 2000 or zhu_cnt >= 1, "珍珠拾取生效（修为+%d 或库存%d）" % [e1 - _e_entry, zhu_cnt])
			# ⑦出洞
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 6
		6:
			_press("up")
			_next = _t + 0.8
			_step = 7
		7:
			_check(current_scene.get_node_or_null("LongGong") == null, "龙宫已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到主场景")
			var pos = _player().global_position
			_check(abs(pos.x - 8600.0) < 40.0, "出洞回到门口位置")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
