# 步骤C harness: ①buff def 表 ②丹药→buff 施加 ③同名刷新不叠加 ④攻击乘区钩子
#      ⑤元素抗性钩子 ⑥到期自动消失 ⑦存档往返 ⑧HUD buff 行
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
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(buffs != null, "player has BuffSystem")
			_check(buffs.call("apply", "buff_bing_xin"), "def buff_bing_xin exists")
			_check(buffs.call("apply", "buff_chi_yan"), "def buff_chi_yan exists")
			_check(buffs.call("apply", "buff_jin_gang"), "def buff_jin_gang exists")
			_check(not buffs.call("apply", "buff_nonexist"), "unknown buff rejected")
			buffs.call("clear")
		2:
			_next = _t + 0.3
			# 丹药 → buff：赤焰丹
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			var buffs = p.call("get_buffs")
			inv.call("add_item", "chi_yan_dan", 2)
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan")
			_check(buffs.call("has", "buff_chi_yan"), "buff_chi_yan active")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.15) < 0.001, "atk mult 1.15")
		3:
			_next = _t + 0.3
			# 同名刷新不叠加
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(p.call("use_consumable", "chi_yan_dan"), "use second chi_yan_dan")
			var list = buffs.call("get_active_list")
			_check(list.size() == 1, "same-name refresh: still 1 entry")
			_check(float(list[0]["remaining"]) > 299.0, "duration refreshed to ~300")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.15) < 0.001, "no stacking: atk still 1.15")
			# 攻击乘区钩子：get_effective_attack 上涨
			buffs.call("clear")
			var atk0 = float(p.call("get_effective_attack"))
			buffs.call("apply", "buff_chi_yan")
			var atk1 = float(p.call("get_effective_attack"))
			_check(atk1 > atk0 * 1.1, "effective attack boosted by buff")
		4:
			_next = _t + 0.3
			# 元素抗性钩子（冰心 = 水抗+15%，ELEM_SHUI=3）
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			buffs.call("apply", "buff_bing_xin")
			_check(abs(float(buffs.call("get_elem_resist_bonus", 3)) - 0.15) < 0.001, "shui resist +0.15")
			# HUD buff 行
			var hud = root.find_child("GameHUD", true, false)
			var label = hud.find_child("BuffLabel", true, false)
			_check(label != null and label.visible, "HUD buff label visible")
			print("[TEST] buff label text: ", label.text)
			_check(label.text.length() > 0, "HUD buff label has text")
		5:
			_next = _t + 0.6
			# 到期自动消失：载入 0.3s 剩余，等它过期
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			buffs.call("clear")
			buffs.call("load_from_dict", {"active": [{"id": "buff_jin_gang", "remaining": 0.3}]})
			_check(buffs.call("has", "buff_jin_gang"), "loaded short buff active")
		6:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(not buffs.call("has", "buff_jin_gang"), "buff expired after 0.3s")
			_check(abs(float(buffs.call("get_def_mult")) - 1.0) < 0.001, "def mult back to 1.0")
			var hud = root.find_child("GameHUD", true, false)
			var label = hud.find_child("BuffLabel", true, false)
			_check(not label.visible, "HUD buff label hidden after expiry")
		7:
			_next = _t + 0.3
			# 存档往返
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			buffs.call("apply", "buff_jin_gang")
			buffs.call("apply", "buff_bing_xin")
			var saved = buffs.call("save_to_dict")
			buffs.call("clear")
			_check(buffs.call("get_active_list").size() == 0, "cleared before load")
			buffs.call("load_from_dict", saved)
			_check(buffs.call("has", "buff_jin_gang") and buffs.call("has", "buff_bing_xin"), "save/load roundtrip 2 buffs")
			buffs.call("clear")
		8:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
