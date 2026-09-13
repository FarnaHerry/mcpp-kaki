# 炼丹失败机制 harness（session 022）：
# ①成功率 JSON 加载（凡/灵 100%、地 80%、天 70%）②凡品恒成（注入高 roll 仍成）
# ③地品失败：丹渣×1 + 材料损毁一半（向上取整）退还另一半 + 失败话术
# ④失败不触发丹毒 ⑤地品成功：全材料消耗 ⑥默认确定性序列首炉必成（锁定种子）
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

func _rate_of(list: Array, id: String) -> float:
	for r in list:
		if String(r["id"]) == id:
			return float(r["success_rate"])
	return -1.0

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
			_check(abs(_rate_of(list, "healing_pill") - 1.0) < 0.001, "rate: 回春丹 凡品 100%")
			_check(abs(_rate_of(list, "qi_pill") - 1.0) < 0.001, "rate: 聚气丹 凡品 100%")
			_check(abs(_rate_of(list, "bing_xin_dan") - 1.0) < 0.001, "rate: 冰心丹 灵品 100%")
			_check(abs(_rate_of(list, "chi_yan_dan") - 1.0) < 0.001, "rate: 赤焰丹 灵品 100%")
			_check(abs(_rate_of(list, "jin_gang_dan") - 1.0) < 0.001, "rate: 金刚丹 灵品 100%")
			_check(abs(_rate_of(list, "wu_dao_dan") - 0.8) < 0.001, "rate: 悟道丹 地品 80%")
			_check(abs(_rate_of(list, "da_huan_dan") - 0.8) < 0.001, "rate: 大还丹 地品 80%")
			_check(abs(_rate_of(list, "xuan_long_dan") - 0.7) < 0.001, "rate: 玄龙丹 天品 70%")
		2:
			_next = _t + 0.3
			# 凡品恒成：注入必败 roll（0.999）仍炼成，且不消耗注入以外的序列
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var inv = p.call("get_inventory")
			inv.call("add_item", "zhi_xue_cao", 3)
			al.call("debug_set_next_roll", 0.999)
			_check(al.call("craft", "healing_pill"), "凡品注入 0.999 仍炼成（恒 100%）")
			_check("炼成" in al.call("get_last_message"), "message: 炼成")
			_check(_count_item(inv, "healing_pill") >= 1, "回春丹入包")
			_check(_count_item(inv, "dan_zha") == 0, "成功无丹渣")
			_check(_count_item(inv, "zhi_xue_cao") == 0, "成功全材料消耗")
		3:
			_next = _t + 0.3
			# 突破到金丹（realm 3）开地品门控
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			while int(cult.call("get_realm_index")) < 3:
				cult.call("attempt_breakthrough")
			_check(int(cult.call("get_realm_index")) >= 3, "reached 金丹")
		4:
			_next = _t + 0.3
			# 地品失败：注入 0.99 > 0.8 → 炉火失控
			# 悟道丹材料 悟道茶×1 + 聚灵草×2 → 损毁 ceil(1/2)=1 茶 + ceil(2/2)=1 草，退草×1
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var inv = p.call("get_inventory")
			inv.call("add_item", "wu_dao_cha", 1)
			inv.call("add_item", "ju_ling_cao", 2)
			al.call("debug_set_next_roll", 0.99)
			_check(not al.call("craft", "wu_dao_dan"), "地品注入 0.99 炼制失败")
			_check("丹毁渣存" in al.call("get_last_message"), "message: 炉火失控，丹毁渣存")
			_check(_count_item(inv, "wu_dao_dan") == 0, "失败无丹产出")
			_check(_count_item(inv, "dan_zha") == 1, "失败得丹渣×1")
			_check(_count_item(inv, "wu_dao_cha") == 0, "悟道茶×1 全毁（ceil(1/2)=1）")
			_check(_count_item(inv, "ju_ling_cao") == 1, "聚灵草×2 毁一半退一半")
			# 失败不触发丹毒计数（没出丹）
			var buffs = p.call("get_buffs")
			_check(buffs == null or not bool(buffs.call("has", "buff_dan_du")), "失败不触发丹毒")
		5:
			_next = _t + 0.3
			# 注入为一次性：上炉注入已消费，本炉注入 0.0 必成
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var inv = p.call("get_inventory")
			inv.call("add_item", "wu_dao_cha", 1)
			inv.call("add_item", "ju_ling_cao", 2)
			al.call("debug_set_next_roll", 0.0)
			_check(al.call("craft", "wu_dao_dan"), "地品注入 0.0 炼成")
			_check("炼成" in al.call("get_last_message"), "message: 炼成 悟道丹")
			_check(_count_item(inv, "wu_dao_dan") == 1, "悟道丹入包")
			_check(_count_item(inv, "wu_dao_cha") == 0, "成功：悟道茶全消耗")
			_check(_count_item(inv, "ju_ling_cao") == 1, "成功：聚灵草全消耗（上炉退的 1 留存）")
		6:
			_next = _t + 0.3
			# 默认确定性序列：不注入时首 roll ≈0.0418 ≤ 0.8 → 地品首炉必成（锁定种子）
			var p = root.find_child("Player", true, false)
			var al = p.call("get_alchemy")
			var inv = p.call("get_inventory")
			inv.call("add_item", "wu_dao_cha", 1)
			inv.call("add_item", "ju_ling_cao", 2)
			_check(al.call("craft", "wu_dao_dan"), "默认序列首炉地品炼成（固定种子可复现）")
		7:
			# 丹渣已注册为凡品材料（售 1 灵石）
			var db = root.find_child("ItemDatabase", true, false)
			_check(db.call("has_item", "dan_zha"), "dan_zha 已注册")
			var info = db.call("get_item_info", "dan_zha")
			_check(int(info.get("type", -1)) == 1, "dan_zha 为材料类型")
			_check(int(info.get("sell_price", 0)) == 1, "dan_zha 售 1 灵石")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
