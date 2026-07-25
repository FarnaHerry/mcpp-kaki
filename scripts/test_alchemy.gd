# 步骤E harness: ①配方表×7 ②材料不足拒炼 ③材料足够炼成+扣材料+产物入包
#      ④地品境界门控（金丹前拒炼）⑤金丹后可炼 ⑥GameMenu 炼丹页存在
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

func _count_item(inv, id) -> int:
	for i in range(inv.call("get_capacity")):
		var sd = inv.call("get_slot", i)
		if not sd.is_empty() and String(sd["id"]) == id:
			return int(sd["quantity"])
	return 0

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			_check(al != null, "player has AlchemySystem")
			var list = al.call("get_recipe_list")
			_check(list.size() == 7, "7 recipes")
			# 材料不足拒炼
			_check(not al.call("craft", "healing_pill"), "craft without mats fails")
			_check("材料" in al.call("get_last_message"), "message: 材料不足")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var inv = p.call("get_inventory")
			inv.call("add_item", "zhi_xue_cao", 3)
			_check(al.call("can_craft", "healing_pill"), "can_craft with 3 herbs")
			_check(al.call("craft", "healing_pill"), "craft healing_pill ok")
			_check(_count_item(inv, "healing_pill") >= 1, "pill in inventory")
			_check(_count_item(inv, "zhi_xue_cao") == 0, "herbs consumed")
		3:
			_next = _t + 0.3
			# 地品门控：凡人期悟道丹（有材料也拒炼）
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var inv = p.call("get_inventory")
			inv.call("add_item", "wu_dao_cha", 1)
			inv.call("add_item", "ju_ling_cao", 2)
			_check(not al.call("can_craft", "wu_dao_dan"), "wu_dao_dan locked at mortal")
			_check(not al.call("craft", "wu_dao_dan"), "craft locked recipe fails")
			_check("境界" in al.call("get_last_message"), "message: 境界不足")
		4:
			_next = _t + 0.3
			# 突破到金丹（realm 3）后可炼
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			while int(cult.call("get_realm_index")) < 3:
				cult.call("attempt_breakthrough")
			_check(int(cult.call("get_realm_index")) >= 3, "reached 金丹")
			_check(al.call("can_craft", "wu_dao_dan"), "wu_dao_dan unlocked at 金丹")
			_check(al.call("craft", "wu_dao_dan"), "craft wu_dao_dan ok")
			_check(_count_item(p.call("get_inventory"), "wu_dao_dan") >= 1, "wu_dao_dan in inventory")
		5:
			_next = _t + 0.4
			# GameMenu 炼丹页：开菜单（action_press 一帧后释放才触发 just_pressed 轮询）
			Input.action_press("menu")
		6:
			_next = _t + 0.2
			Input.action_release("menu")
			Input.action_press("right")
		7, 9, 11, 13:
			_next = _t + 0.2
			Input.action_release("right")
			Input.action_press("right")
		8, 10, 12:
			_next = _t + 0.2 # 间隔帧：释放后不立刻再按，保证 just_pressed 语义
		14:
			_next = _t + 0.3
			Input.action_release("right")
			var menu = root.find_child("GameMenu", true, false)
			var found = false
			for c in menu.get_children():
				if c is Label and "炼丹" in c.text:
					found = true
			_check(found, "alchemy page title exists in GameMenu")
			Input.action_press("menu")
		15:
			_next = _t + 0.2
			Input.action_release("menu")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
