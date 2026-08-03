# 步骤B harness: ①7草药+5新丹药注册 ②use_consumable 统一入口（回血/回灵/修为，扣数量）
#      ③聚气丹=回灵（mana_amount 迁移）④ext_use 改道（面板用丹同样生效）
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
			var db = root.find_child("ItemDatabase", true, false)
			for id in ["zhi_xue_cao", "ju_ling_cao", "bing_xin_lian", "chi_yan_hua", "jin_gang_teng", "wu_dao_cha", "qian_nian_ling_zhi"]:
				_check(db.call("has_item", id), "herb registered: " + id)
			for id in ["bing_xin_dan", "chi_yan_dan", "jin_gang_dan", "wu_dao_dan", "da_huan_dan"]:
				_check(db.call("has_item", id), "pill registered: " + id)
			# 聚气丹迁移：mana_amount=50, energy_amount=0
			var info = db.call("get_item_info", "qi_pill")
			_check(float(info.get("mana_amount", 0.0)) == 50.0, "qi_pill mana_amount 50")
			_check(float(info.get("energy_amount", 0.0)) == 0.0, "qi_pill no energy")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			var cult = p.call("get_cultivation")
			# 修为：悟道丹（凡人期未封顶，先做能量验证再突破）
			var e0 = float(cult.call("get_current_energy"))
			inv.call("add_item", "wu_dao_dan", 1)
			_check(p.call("use_consumable", "wu_dao_dan"), "use wu_dao_dan ok")
			_check(float(cult.call("get_current_energy")) > e0, "energy increased by wu_dao_dan")
			# 突破到炼气（凡人灵力上限 0，mana 测试必须先突破）
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			cult.call("attempt_breakthrough")
		3:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			var cult = p.call("get_cultivation")
			# 回血：回春丹（用绑定 getter，.get() 拿不到 C++ 裸成员）
			p.call("set_current_health", 10.0)
			inv.call("add_item", "healing_pill", 2)
			var h0 = float(p.call("get_current_health"))
			_check(p.call("use_consumable", "healing_pill"), "use healing_pill ok")
			_check(float(p.call("get_current_health")) > h0, "heal applied")
			# 回灵：聚气丹（mana_amount 迁移验证）
			cult.call("set_mana", 0.0)
			inv.call("add_item", "qi_pill", 1)
			_check(p.call("use_consumable", "qi_pill"), "use qi_pill ok")
			_check(float(cult.call("get_mana")) >= 49.0, "mana restored ~50")
		4:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			# 数量扣减：回春丹 2 → 1
			var cnt = 0
			for i in range(inv.call("get_capacity")):
				var sd = inv.call("get_slot", i)
				if not sd.is_empty() and String(sd["id"]) == "healing_pill":
					cnt = int(sd["quantity"])
			_check(cnt == 1, "healing_pill qty 2 -> 1")
			# 空用：包里没有的丹返回 false
			_check(not p.call("use_consumable", "da_huan_dan"), "use_consumable missing item returns false")
		5:
			_next = _t + 0.3
			# ext_use 改道验证：选中回血丹槽位 → 面板用丹回血
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			var panel = root.find_child("InventoryPanel", true, false)
			p.call("set_current_health", 10.0)
			var slot = -1
			for i in range(inv.call("get_capacity")):
				var sd = inv.call("get_slot", i)
				if not sd.is_empty() and String(sd["id"]) == "healing_pill":
					slot = i
			_check(slot >= 0, "found healing_pill slot for panel test")
			panel.call("set_selected_index", slot)
			var h0 = float(p.call("get_current_health"))
			panel.call("ext_use")
			_check(float(p.call("get_current_health")) > h0, "ext_use heal via use_consumable")
		6:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
