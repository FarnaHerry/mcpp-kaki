# 古剑冢秘境（断崖绝壁深处，Portal 房间模式）：
# ①物品注册（青锋古剑 地品武器 攻+25）②入口 Portal 在断崖绝壁段(2600~3900) ③↑ 进秘境
# ④敌情断言（锈剑傀儡×3+精英×1/剑灵×2/守灵 Boss，realm/精英词缀）⑤击杀守灵 → 必掉青锋古剑 ⑥出秘境
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

# 全场景扫 ItemPickup，返回 {item_id: count}
func _collect_pickups() -> Dictionary:
	var result := {}
	var stack := [current_scene]
	while not stack.is_empty():
		var n = stack.pop_back()
		for c in n.get_children():
			stack.push_back(c)
		var v = n.get("item_id")
		if typeof(v) == TYPE_STRING_NAME or typeof(v) == TYPE_STRING:
			var id := String(v)
			if id != "":
				result[id] = int(result.get(id, 0)) + 1
	return result

# 房间内按 enemy_id 计数
func _count_enemy(room: Node, enemy_id: String) -> int:
	var cnt := 0
	var stack := [room]
	while not stack.is_empty():
		var n = stack.pop_back()
		for c in n.get_children():
			stack.push_back(c)
		var v = n.get("enemy_id")
		if typeof(v) == TYPE_STRING or typeof(v) == TYPE_STRING_NAME:
			if String(v) == enemy_id:
				cnt += 1
	return cnt

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			# ①物品注册：青锋古剑（地品武器 攻+25）
			var info = _db().call("get_item_info", "qing_feng_gu_jian")
			_check(not info.is_empty(), "物品注册: qing_feng_gu_jian")
			_check(int(info.get("type", -1)) == 3, "青锋古剑=装备")
			_check(int(info.get("grade", -1)) == 2, "青锋古剑=地品")
			# 攻+25 实测（凡人期乘区=1.0，避免境界倍率摊入；get_item_info 不含 attack_bonus，走装备管线）
			var p0 = _player()
			var atk0 = float(p0.call("get_effective_attack"))
			var inv0 = p0.call("get_inventory")
			inv0.call("add_item", "qing_feng_gu_jian", 1)
			var equipped := false
			for i in range(inv0.call("get_capacity")):
				var sd = inv0.call("get_slot", i)
				if not sd.is_empty() and String(sd["id"]) == "qing_feng_gu_jian":
					equipped = bool(p0.call("equip_item", i))
					break
			_check(equipped, "装备青锋古剑")
			var atk1 = float(p0.call("get_effective_attack"))
			_check(abs(atk1 - atk0 - 25.0) < 0.01, "青锋古剑攻+25（%.0f→%.0f）" % [atk0, atk1])
			# 金丹（血厚防秘境怪围殴致死，干扰断言）
			_breakthrough_to(3)
			# ②入口 Portal 在断崖绝壁段
			var portal: Node = null
			for n in current_scene.get_children():
				var v = n.get("target_scene")
				if typeof(v) == TYPE_STRING or typeof(v) == TYPE_STRING_NAME:
					if String(v) == "res://scenes/rooms/gu_jian_zhong.tscn":
						portal = n
						break
			_check(portal != null, "古剑冢入口 Portal 存在")
			if portal:
				_check(portal.position.x >= 2600.0 and portal.position.x <= 3900.0, "入口位于断崖绝壁段（x=%.0f）" % portal.position.x)
			# 传送门口
			_player().global_position = Vector2(3650, 210)
			_next = _t + 0.6
			_step = 1
		1:
			Input.action_press("up") # 按住一帧以上再释放（action 轮询可靠）
			_next = _t + 0.3
			_step = 2
		2:
			Input.action_release("up")
			_next = _t + 0.6
			_step = 3
		3:
			# ③秘境已挂载
			var room = current_scene.get_node_or_null("GuJianZhong")
			_check(room != null, "古剑冢已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进秘境")
			if room:
				# ④敌情：数量 / enemy_id / realm
				_check(_count_enemy(room, "xiu_jian_kui_lei") == 4, "锈剑傀儡×4（3普通+1精英）")
				_check(_count_enemy(room, "jian_ling") == 2, "剑灵×2")
				_check(_count_enemy(room, "jian_zhong_shou_ling") == 1, "剑冢守灵在场")
				var k0 = room.find_child("XiuJianKuiLei0", true, false)
				_check(k0 != null and int(k0.call("get_realm")) == 1, "锈剑傀儡 realm=1")
				var jl = room.find_child("JianLing0", true, false)
				_check(jl != null and int(jl.call("get_realm")) == 2, "剑灵 realm=2")
				_check(jl != null and bool(jl.call("get_is_flying")), "剑灵=飞行")
				# 精英已精英化（厚甲词缀）
				var elite = room.find_child("XiuJianJingYing", true, false)
				_check(elite != null, "精英傀儡在场")
				if elite:
					_check(int(elite.call("get_elite_tier")) == 1, "精英 elite_tier==1")
					_check(String(elite.call("get_affix_id")) == "hou_jia", "精英词缀=hou_jia（厚甲）")
				var boss = room.find_child("JianZhongShouLing", true, false)
				_check(boss != null and bool(boss.call("get_is_boss")), "守灵=Boss")
				_check(boss != null and int(boss.call("get_realm")) == 3, "守灵 realm=3")
				_check(boss != null and String(boss.call("get_drop_table")) == "gu_jian_zhong", "守灵掉落表=gu_jian_zhong")
			_step = 4
		4:
			# ⑤击杀剑冢守灵
			var boss = current_scene.find_child("JianZhongShouLing", true, false)
			_check(boss != null and float(boss.call("get_current_health")) > 0, "守灵存活待击")
			if boss:
				boss.call("take_damage", 999999.0, _player())
			_next = _t + 0.8
			_step = 5
		5:
			var boss2 = current_scene.find_child("JianZhongShouLing", true, false)
			_check(boss2 == null or float(boss2.call("get_current_health")) <= 0, "守灵已被击杀")
			# 必掉青锋古剑（gu_jian_zhong 表 chance=1.0）
			var picks = _collect_pickups()
			_check(int(picks.get("qing_feng_gu_jian", 0)) >= 1, "场景出现青锋古剑拾取物")
			# 走向出口（ExitPortal 在房间 x=200 处）
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 6
		6:
			Input.action_press("up") # 出秘境
			_next = _t + 0.3
			_step = 7
		7:
			Input.action_release("up")
			_next = _t + 0.6
			_step = 8
		8:
			# ⑥出秘境
			_check(current_scene.get_node_or_null("GuJianZhong") == null, "古剑冢已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到主场景")
			var pos = _player().global_position
			_check(abs(pos.x - 3650.0) < 60.0, "出秘境回到断崖绝壁门口（x=%.0f）" % pos.x)
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
