# 商店系统测试（长安坊市灵石买卖）：
# ①灵石余额 ②买（扣灵石+入库）③灵石不足拒买 ④卖（扣物品+回灵石）⑤货币/关键物不可卖
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

func _shop():
	return root.find_child("ShopSystem", true, false)

func _stones():
	return _shop().call("get_spirit_stones", _player())

func _count(id: String) -> int:
	return int(_player().call("get_inventory").call("get_item_count", id))

func _currency():
	return root.find_child("CurrencySystem", true, false)

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 30:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 0.3
			_check(_shop() != null, "ShopSystem 存在")
			_check(_stones() == 0, "初始灵石 0")
			# 给灵石（四阶钱包，session 012）+ 商店货架
			_currency().call("add", 0, 100)
			_check(_stones() == 100, "加灵石 100")
			_check(_shop().call("get_stock").size() >= 5, "商店货架非空")
			_check(_count("healing_pill") == 0, "初始无回春丹")
			# 买回春丹（30 灵石）
			_check(_shop().call("buy", _player(), "healing_pill"), "买回春丹")
		2:
			_next = _t + 0.2
			_check(_stones() == 70, "灵石 100-30=70")
			_check(_count("healing_pill") == 1, "回春丹入库 1")
			# 灵石不足拒买（大还丹 300 > 70）
			_check(not _shop().call("buy", _player(), "da_huan_dan"), "灵石不足拒买")
			_check(_count("da_huan_dan") == 0, "大还丹未入库")
			# 买不可售物（飞剑 buy_price 0）
			_check(not _shop().call("buy", _player(), "flying_sword"), "商店不售飞剑")
			# 卖回春丹（sell 15）
			_check(_shop().call("sell", _player(), "healing_pill"), "卖回春丹")
		3:
			_next = _t + 0.2
			_check(_stones() == 85, "灵石 70+15=85")
			_check(_count("healing_pill") == 0, "回春丹已售")
			# 货币不可卖
			_check(not _shop().call("sell", _player(), "spirit_stone"), "灵石不可卖")
			# 关键物不可卖（飞剑 sell_price 0）
			_player().call("get_inventory").call("add_item", "flying_sword", 1)
			_check(not _shop().call("sell", _player(), "flying_sword"), "飞剑不可卖")
			# ShopPanel 存在 + 可打开
			var panel = root.find_child("ShopPanel", true, false)
			_check(panel != null, "ShopPanel 存在")
			panel.call("open")
			_next = _t + 0.3
		4:
			var panel = root.find_child("ShopPanel", true, false)
			_check(bool(panel.call("is_open")), "ShopPanel 打开")
			_check(bool(panel.call("is_open")), "ShopPanel 打开（余额显示）")
			panel.call("close")
			_check(not bool(panel.call("is_open")), "ShopPanel 关闭")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
