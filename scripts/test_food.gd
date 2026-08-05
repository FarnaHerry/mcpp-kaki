# 食物/辟谷系统测试（design/cultivation-realms.md 饮食）：
# ①饱食度初始 ②时间衰减 ③饥饿 debuff ④食物恢复+解除饥饿 ⑤炼气 120% ⑥筑基辟谷
# ⑦辟谷后食物转纯 buff ⑧HUD 饱食度条 ⑨存档保持
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _fullness_before := 0.0

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

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 40:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			var p = _player()
			_check(p.call("get_fullness") > 99.0, "饱食度初始满 (%.1f)" % p.call("get_fullness"))
			_check(not p.call("is_bigu"), "凡人非辟谷")
			_check(p.call("get_food_mult") == 1.0, "凡人食物倍率 1.0")
		2:
			# 时间衰减（凡人）
			_next = _t + 2.5
		3:
			var p = _player()
			_check(p.call("get_fullness") < 100.0 and p.call("get_fullness") > 95.0, "饱食度随时间衰减 (%.1f)" % p.call("get_fullness"))
			# 饥饿：归零 → debuff
			p.call("set_fullness", 0.0)
			_next = _t + 0.3
		4:
			var p = _player()
			var buffs = p.call("get_buffs")
			_check(bool(buffs.call("has", "buff_hunger")), "饱食归零 → 饥饿 debuff")
			_check(float(buffs.call("get_atk_mult")) < 1.0, "饥饿降攻 (atk x%.2f)" % float(buffs.call("get_atk_mult")))
			_check(float(buffs.call("get_def_mult")) < 1.0, "饥饿降防")
			# 食物恢复 + 解除饥饿
			var inv = p.call("get_inventory")
			inv.call("add_item", "brown_rice", 2)
			_check(p.call("use_consumable", "brown_rice"), "食用糙米饭")
			_next = _t + 0.3
		5:
			var p = _player()
			_check(p.call("get_fullness") > 0.0, "糙米饭回饱食度 (%.1f)" % p.call("get_fullness"))
			_check(not bool(p.call("get_buffs").call("has", "buff_hunger")), "进食解除饥饿")
			# 扣数量
			_check(int(p.call("get_inventory").call("get_item_count", "brown_rice")) == 1, "糙米饭扣 1 个")
			# 炼气 120%：突破炼气
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000)
			cult.call("attempt_breakthrough")
			_next = _t + 0.3
		6:
			var p = _player()
			_check(abs(p.call("get_food_mult") - 1.2) < 0.001, "炼气食物倍率 1.2")
			# 低饱食度吃糙米饭 → +15×1.2=18
			p.call("set_fullness", 20.0)
			_next = _t + 0.2
		7:
			var p = _player()
			p.call("use_consumable", "brown_rice")
			_check(p.call("get_fullness") >= 37.5 and p.call("get_fullness") <= 38.5, "炼气食物+18 (%.1f)" % p.call("get_fullness"))
			# 存档（炼气非辟谷）：set fullness → save → 改 → load → 恢复
			p.call("set_fullness", 55.0)
			var gm = root.find_child("GameManager", true, false)
			gm.call("save_game", "food_test")
			p.call("set_fullness", 10.0)
			gm.call("load_game", "food_test")
			_next = _t + 0.6
		8:
			var p = _player()
			# 存的是 55，读档后仍是高位（>50 而非跌回 10；读档后衰减继续，允许 -1 容差）
			_check(p.call("get_fullness") > 50.0, "存档恢复饱食度 (%.1f)" % p.call("get_fullness"))
			# 筑基辟谷：突破筑基
			var cult = p.call("get_cultivation")
			cult.call("accumulate_energy", 100000000)
			cult.call("attempt_breakthrough")
			_next = _t + 0.4
		9:
			var p = _player()
			_check(p.call("is_bigu"), "筑基辟谷")
			# 辟谷后不再衰减
			_fullness_before = p.call("get_fullness")
			_next = _t + 2.5
		10:
			var p = _player()
			_check(abs(p.call("get_fullness") - _fullness_before) < 0.01, "辟谷后饱食度不再衰减")
			# 辟谷后食物转纯 buff（fullness 不变）
			var inv = p.call("get_inventory")
			inv.call("add_item", "dry_ration", 1)
			p.call("use_consumable", "dry_ration")
			_check(abs(p.call("get_fullness") - _fullness_before) < 0.01, "辟谷食物不回饱食度")
			_check(bool(p.call("get_buffs").call("has", "buff_fullness_mid")), "辟谷食物转 buff（干粮）")
			# HUD：辟谷后饱食度条隐藏
			var label = root.find_child("FullnessLabel", true, false)
			_check(label != null, "FullnessLabel 存在")
			if label:
				_check(not label.is_visible_in_tree(), "辟谷后饱食度条隐藏")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
