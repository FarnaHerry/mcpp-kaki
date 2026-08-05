# 灵石四阶货币测试（session 012）：
# ①钱包基本 ②兑换（破零/合成，RATIO=10）③spend 自动找零 ④拾取路由（进钱包不进背包）⑤存档
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

func _cur():
	return root.find_child("CurrencySystem", true, false)

func _amt(tier: int) -> int:
	return int(_cur().call("get_amount", tier))

func _total() -> int:
	return int(_cur().call("get_total"))

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
			_check(_cur() != null, "CurrencySystem 存在")
			_check(_total() == 0, "初始钱包 0")
			_cur().call("add", 0, 100)  # 下品
			_cur().call("add", 1, 3)    # 中品
			_next = _t + 0.3
		2:
			_check(_total() == 130, "总价值 = 下品100 + 中品30 = 130")
			_check(_amt(1) == 3, "中品 3 枚")
			# 破零：中品×3 → 下品×30
			_check(bool(_cur().call("exchange", 1, 3, 0)), "中品→下品 兑换")
			_next = _t + 0.3
		3:
			_check(_amt(0) == 130, "下品 130")
			_check(_amt(1) == 0, "中品 0")
			_check(_total() == 130, "兑换保值 130")
			# 合成：下品×13组(130) → 中品×13
			_check(bool(_cur().call("exchange", 0, 13, 1)), "下品→中品 合成")
			_next = _t + 0.3
		4:
			_check(_amt(1) == 13, "中品 13")
			_check(_amt(0) == 0, "下品 0")
			# spend 25：自动破零+找零回填高档（105 = 上品1 + 下品5）
			_check(bool(_cur().call("spend", 25)), "spend 25")
			_next = _t + 0.3
		5:
			_check(_total() == 105, "130-25=105")
			_check(_amt(2) == 1, "找零上品 1（高档优先回填）")
			_check(_amt(0) == 5, "找零下品 5")
			_check(bool(_cur().call("can_afford", 105)), "可付 105")
			_check(not bool(_cur().call("can_afford", 106)), "不可付 106")
			_check(bool(_cur().call("spend", 105)), "spend 全清")
			_next = _t + 0.3
		6:
			_check(_total() == 0, "钱包清零")
			# 拾取路由：灵石 → 钱包（不进背包）
			var p = _player()
			p.call("pickup_item", "spirit_stone", 7)
			p.call("pickup_item", "spirit_stone_peak", 1) # 极品
			_next = _t + 0.3
		7:
			_check(_amt(0) == 7, "拾取下品 7")
			_check(_amt(3) == 1, "拾取极品 1")
			_check(int(_player().call("get_inventory").call("get_item_count", "spirit_stone")) == 0, "下品灵石不进背包")
			# 极品破零 → 上品
			_check(bool(_cur().call("exchange", 3, 1, 2)), "极品→上品 兑换")
			_next = _t + 0.3
		8:
			_check(_amt(2) == 10, "极品×1 → 上品×10")
			_check(_total() == 1007, "总价值 1000+7=1007")
			# 存档：save_to_dict 往返
			var d = _cur().call("save_to_dict")
			_check(int(d.get("low", -1)) == 7 and int(d.get("high", -1)) == 10, "存档含四阶 (low7 high10)")
			_cur().call("load_from_dict", {"low": 5, "mid": 2, "high": 0, "peak": 0})
			_next = _t + 0.3
		9:
			_check(_total() == 5 + 20, "读档恢复 (下5 中2 = 25)")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
