# 城镇安全区 + NPC harness（SafeZone + TownNpc + WC.create_town 五洲落地）：
# ① 落霞村装配（SafeZone 组/城镇名/NPC 名单）② is_point_safe 内外判定
# ③ 区内敌人不索敌（不越界 + 玩家不掉血）④ 区内缓速休整（HP 回升）
# ⑤ NPC 对话气泡循环（X）⑥ 客栈歇息全恢复 ⑦ 五洲城镇端到端（travel_to_direct）
extends SceneTree

const WC = preload("res://scripts/world_common.gd")

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

var _hp_before := 0.0
var _line1 := ""
var _thug := {} # 区外敌人引用

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

func _cm():
	return root.find_child("ContinentManager", true, false)

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _scene_path() -> String:
	return str(current_scene.scene_file_path) if current_scene else ""

func _zones() -> Array:
	return get_nodes_in_group("safe_zones")

func _npcs() -> Array:
	return get_nodes_in_group("town_npcs")

func _npc_by_name(n: String):
	for npc in _npcs():
		if String(npc.call("get_npc_name")) == n:
			return npc
	return null

func _zone_by_name(n: String):
	for z in _zones():
		if String(z.call("get_town_name")) == n:
			return z
	return null

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			# ① 落霞村装配
			_check(_zones().size() >= 1, "主场景有安全区")
			_check(_zone_by_name("落霞村") != null, "落霞村存在")
			_check(_npcs().size() == 3, "落霞村 3 名 NPC")
			_check(_npc_by_name("王婆客栈") != null and _npc_by_name("王婆客栈").call("is_healer"), "王婆客栈=歇息型")
			_check(_npc_by_name("王村长") != null and not _npc_by_name("王村长").call("is_healer"), "王村长=交谈型")
			_step = 1
		1:
			# ② is_point_safe 内外判定
			_player().global_position = Vector2(700, 210) # 村内
			_next = _t + 0.4
			_step = 2
		2:
			_check(bool(SafeZone.is_point_safe(Vector2(700, 210))), "村内点=安全")
			_check(not bool(SafeZone.is_point_safe(Vector2(1000, 210))), "村外点=不安全")
			# ③④ 打伤入村：区外敌人不索敌 + 区内缓回
			_player().call("take_damage", 30, null)
			_hp_before = float(_player().call("get_current_health"))
			_thug["e"] = WC.spawn_enemy_by_id(current_scene, Vector2(940, 200), "shan_xiao", "T_Thug")
			_check(_thug["e"] != null, "村口外哨怪生成（940）")
			_next = _t + 3.0 # 3s：区内若被追击必掉血；若休整生效必回升
			_step = 3
		3:
			_check(_thug["e"].global_position.x > 830.0, "敌人不越村界（仍在 830 右侧）")
			var hp = float(_player().call("get_current_health"))
			_check(hp >= _hp_before - 0.5, "村内不掉血（敌人未索敌）")
			_check(hp > _hp_before + 1.0, "村内缓回生效（HP 回升）")
			_check(bool(SafeZone.is_point_safe(_player().global_position)), "玩家仍在村内")
			_step = 4
		4:
			# ⑤ NPC 对话气泡循环
			var elder = _npc_by_name("王村长")
			_player().global_position = elder.global_position + Vector2(18, 0)
			_next = _t + 0.5
			_step = 5
		5:
			_press("interact")
			_next = _t + 0.4
			_step = 6
		6:
			var elder = _npc_by_name("王村长")
			_line1 = String(elder.call("get_bubble_text"))
			_check(_line1 != "", "村长气泡显示（%s）" % _line1)
			_press("interact")
			_next = _t + 0.4
			_step = 7
		7:
			var elder = _npc_by_name("王村长")
			var line2 = String(elder.call("get_bubble_text"))
			_check(line2 != "" and line2 != _line1, "村长气泡翻行（%s → %s）" % [_line1, line2])
			# ⑥ 客栈歇息全恢复
			_player().call("take_damage", 40, null)
			var inn = _npc_by_name("王婆客栈")
			_player().global_position = inn.global_position + Vector2(18, 0)
			_next = _t + 0.5
			_step = 8
		8:
			_press("interact")
			_next = _t + 0.5
			_step = 9
		9:
			var hp = float(_player().call("get_current_health"))
			var mh = float(_player().call("get_max_health"))
			_check(hp >= mh - 0.5, "客栈歇息后 HP 全恢复（%d/%d）" % [hp, mh])
			_step = 10
		10:
			# ⑦ 五洲城镇端到端（直达旅行；travel_to_direct 仍受境界门控，先突破到天尊）
			_breakthrough_to(10)
			_check(int(_cult().call("get_realm_index")) == 10, "突破到真仙（跨洲门控放开）")
			_check(_cm().call("travel_to_direct", "xiniuhe"), "直达西牛贺洲")
			_next = _t + 1.5
			_step = 11
		11:
			_check(_scene_path() == "res://scenes/continents/xiniuhe.tscn", "已切西牛贺洲")
			_check(_zone_by_name("避火庄") != null, "避火庄存在")
			_check(_npcs().size() == 3, "避火庄 3 名 NPC")
			_check(_cm().call("travel_to_direct", "nanzhanbu"), "直达南赡部洲")
			_next = _t + 1.5
			_step = 12
		12:
			_check(_zone_by_name("长安坊市") != null, "长安坊市安全区存在")
			_check(_npcs().size() == 2, "长安坊市 2 名新增 NPC（另有商店掌柜）")
			_check(_cm().call("travel_to_direct", "beijulu"), "直达北俱芦洲")
			_next = _t + 1.5
			_step = 13
		13:
			_check(_zone_by_name("苦寒驿") != null, "苦寒驿存在")
			_check(_npcs().size() == 3, "苦寒驿 3 名 NPC")
			_check(_cm().call("travel_to_direct", "tianjie"), "直达天界")
			_next = _t + 1.5
			_step = 14
		14:
			_check(_zone_by_name("天庭街市") != null, "天庭街市安全区存在")
			_check(_npcs().size() == 2, "天庭街市 2 名 NPC")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
