# 步骤F harness: ①拾取消耗品自动入栏(首个空位) ②同种不重复 ③数字键直接磕(绕过背包)
#      ④耗尽槽被新丹顶替 ⑤HUD 栏显示首字 ⑥use_consumable_bar_slot 效果落地
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
			# 满血拾取（避免自动用干扰）：回春丹×2 → 栏位0
			p.call("pickup_item", "healing_pill", 2)
			_check(String(p.call("get_consumable_bar_slot", 0)) == "healing_pill", "pickup auto-assigns slot 0")
			p.call("pickup_item", "qi_pill", 1)
			_check(String(p.call("get_consumable_bar_slot", 1)) == "qi_pill", "second consumable → slot 1")
			p.call("pickup_item", "healing_pill", 1)
			_check(String(p.call("get_consumable_bar_slot", 2)) == "", "same item no duplicate slot")
			_check(String(p.call("get_consumable_bar_slot", 0)) == "healing_pill", "slot 0 unchanged")
		2:
			_next = _t + 0.3
			# 数字键槽位直接用（绕过背包面板）
			var p = root.find_child("Player", true, false)
			p.call("set_current_health", 10.0)
			_check(p.call("use_consumable_bar_slot", 0), "use_bar_slot(0) ok")
			_check(float(p.call("get_current_health")) > 10.0, "heal via bar slot")
			_check(_count_item(p.call("get_inventory"), "healing_pill") == 2, "qty 3 -> 2")
			# 空槽返回 false
			_check(not p.call("use_consumable_bar_slot", 5), "empty slot returns false")
		3:
			_next = _t + 0.3
			# 耗尽后新丹顶替该槽
			var p = root.find_child("Player", true, false)
			p.call("set_current_health", 100.0) # 满血防自动用
			var inv = p.call("get_inventory")
			inv.call("remove_item", "healing_pill", 2)
			_check(_count_item(inv, "healing_pill") == 0, "healing_pill depleted")
			p.call("pickup_item", "bing_xin_dan", 1)
			_check(String(p.call("get_consumable_bar_slot", 0)) == "bing_xin_dan", "depleted slot reused by new pill")
		4:
			_next = _t + 0.3
			# 数字键输入：consume_2 = 聚气丹回灵（先突破炼气，凡人 mana 上限 0）
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			cult.call("attempt_breakthrough")
			cult.call("set_mana", 0.0)
			Input.action_press("consume_2")
		5:
			_next = _t + 0.3
			Input.action_release("consume_2")
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			_check(float(cult.call("get_mana")) >= 49.0, "consume_2 key → qi_pill → mana 50")
			_check(_count_item(p.call("get_inventory"), "qi_pill") == 0, "qi_pill consumed via key")
		6:
			_next = _t + 0.5
			# HUD 栏显示：找首字标签（冰=冰心丹 聚=聚气丹）
			var hud = root.find_child("GameHUD", true, false)
			var texts = {}
			for c in hud.get_children():
				if c is Label:
					texts[c.text] = true
			_check(texts.has("冰"), "HUD bar shows 冰 (bing_xin_dan)")
			_check(texts.has("聚"), "HUD bar shows 聚 (qi_pill, depleted dim)")
		7:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
