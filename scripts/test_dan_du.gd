# 丹毒机制 harness（design/alchemy.md「丹毒」实装）：
# ①丹药正常服用全效 ②连磕递减（剩余>50% 再服 → 本次 6 折）③60s 窗口 ≥3 次积毒（buff_dan_du 攻防-10%）
# ④丹毒期同种丹再减半且刷新丹毒 ⑤异种丹不受丹毒减半 ⑥存档往返（potency/窗口/丹种）
# ⑦窗口过期重置 + 剩余≤50% 正常刷新 ⑧老档缺省安全 ⑨食物不走丹毒
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
			_check(buffs.call("apply", "buff_dan_du"), "def buff_dan_du exists")
			_check(buffs.call("has", "buff_dan_du"), "buff_dan_du applied")
			_check(abs(float(buffs.call("get_atk_mult")) - 0.9) < 0.001, "dan_du atk -10%")
			_check(abs(float(buffs.call("get_def_mult")) - 0.9) < 0.001, "dan_du def -10%")
			buffs.call("clear")
		2:
			_next = _t + 0.3
			# ① 正常服用：全效
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			inv.call("add_item", "chi_yan_dan", 8)
			inv.call("add_item", "jin_gang_dan", 2)
			inv.call("add_item", "dry_ration", 4)
			var buffs = p.call("get_buffs")
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan #1")
			_check(buffs.call("has", "buff_chi_yan"), "buff_chi_yan active")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.15) < 0.001, "first dose full effect 1.15")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 1.0) < 0.001, "first dose potency 1.0")
			_check(int(buffs.call("get_dose_count", "buff_chi_yan")) == 1, "dose count 1")
		3:
			_next = _t + 0.3
			# ② 连磕递减：剩余 >50% 再服 → 本次 6 折（持续刷新）
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan #2")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 0.6) < 0.01, "second dose potency 0.6")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.09) < 0.01, "diminished atk 1.09 (0.15*0.6)")
			var list = buffs.call("get_active_list")
			_check(list.size() == 1, "same-name refresh: still 1 entry")
			_check(float(list[0]["remaining"]) > 299.0, "duration refreshed to ~300")
			_check(int(buffs.call("get_dose_count", "buff_chi_yan")) == 2, "dose count 2")
		4:
			_next = _t + 0.3
			# ③ 积毒：60s 窗口内第 3 次 → buff_dan_du；触发本次仍按递减 0.6（减半只限「丹毒期间再服」）
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan #3")
			_check(buffs.call("has", "buff_dan_du"), "dan_du triggered at 3rd dose")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 0.6) < 0.01, "triggering dose potency 0.6 (refresh rule)")
			_check(abs(float(buffs.call("get_atk_mult")) - 0.99) < 0.01, "atk 0.99 (0.09 diminished - 0.1 dan_du)")
			_check(abs(float(buffs.call("get_def_mult")) - 0.9) < 0.01, "def 0.9 (dan_du)")
		5:
			_next = _t + 0.3
			# ④ 丹毒期同种丹：效果再减半维持 0.3，且刷新丹毒 120s
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan #4 during dan_du")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 0.3) < 0.01, "still halved during dan_du")
			var dd: Dictionary = {}
			for e in buffs.call("get_active_list"):
				if e["id"] == "buff_dan_du":
					dd = e
			_check(not dd.is_empty() and float(dd["remaining"]) > 115.0, "dan_du refreshed to ~120s")
		6:
			_next = _t + 0.3
			# ⑤ 异种丹不受丹毒减半（「同种丹」语义）
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			_check(p.call("use_consumable", "jin_gang_dan"), "use jin_gang_dan during dan_du")
			_check(abs(float(buffs.call("get_potency", "buff_jin_gang")) - 1.0) < 0.001, "different pill full potency")
			_check(abs(float(buffs.call("get_def_mult")) - 1.1) < 0.01, "def 1.1 (jin_gang 0.2 - dan_du 0.1)")
		7:
			_next = _t + 0.3
			# ⑥ 存档往返：potency/服药窗口/丹毒丹种全保留
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			var saved = buffs.call("save_to_dict")
			buffs.call("load_from_dict", {})
			_check(buffs.call("get_active_list").size() == 0, "wiped before reload")
			_check(int(buffs.call("get_dose_count", "buff_chi_yan")) == 0, "doses wiped before reload")
			buffs.call("load_from_dict", saved)
			_check(buffs.call("has", "buff_chi_yan") and buffs.call("has", "buff_dan_du"), "roundtrip: chi_yan + dan_du")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 0.3) < 0.01, "roundtrip: potency 0.3 kept")
			_check(int(buffs.call("get_dose_count", "buff_chi_yan")) == 4, "roundtrip: dose window kept (4)")
			# 丹种保留：读档后再服同种仍减半 + 刷新丹毒
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan #5 after reload")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 0.3) < 0.01, "post-load toxic halving via saved source")
			_check(buffs.call("has", "buff_dan_du"), "post-load dan_du refreshed")
		8:
			_next = _t + 0.3
			# ⑦ 窗口过期重置：拨快 160s（丹毒 120s 到期、 chi_yan 剩余 ~140s ≤50%）→ 正常全效刷新
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			buffs.call("tick", 160.0)
			_check(not buffs.call("has", "buff_dan_du"), "dan_du expired after 160s")
			_check(int(buffs.call("get_dose_count", "buff_chi_yan")) == 0, "dose window pruned after 60s")
			_check(p.call("use_consumable", "chi_yan_dan"), "use chi_yan_dan #6 after window expiry")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 1.0) < 0.01, "remaining <=50%: full refresh potency 1.0")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.15) < 0.01, "atk back to 1.15")
			_check(not buffs.call("has", "buff_dan_du"), "no dan_du after window reset")
		9:
			_next = _t + 0.3
			# ⑧ 老档缺省安全：无 potency/doses 字段的旧格式
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			buffs.call("load_from_dict", {"active": [{"id": "buff_chi_yan", "remaining": 100.0}]})
			_check(buffs.call("has", "buff_chi_yan"), "legacy save loads")
			_check(abs(float(buffs.call("get_potency", "buff_chi_yan")) - 1.0) < 0.001, "legacy save potency defaults 1.0")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.15) < 0.001, "legacy save full effect")
			_check(int(buffs.call("get_dose_count", "buff_chi_yan")) == 0, "legacy save doses empty")
			buffs.call("clear")
		10:
			_next = _t + 0.3
			# ⑨ 食物不走丹毒：连磕 4 次干粮，无递减无积毒不计数
			var p = root.find_child("Player", true, false)
			var buffs = p.call("get_buffs")
			for i in 4:
				_check(p.call("use_consumable", "dry_ration"), "eat dry_ration #%d" % (i + 1))
			_check(buffs.call("has", "buff_fullness_mid"), "food buff active")
			_check(abs(float(buffs.call("get_potency", "buff_fullness_mid")) - 1.0) < 0.001, "food no diminishing")
			_check(abs(float(buffs.call("get_atk_mult")) - 1.05) < 0.001, "food full effect 1.05")
			_check(not buffs.call("has", "buff_dan_du"), "food never triggers dan_du")
			_check(int(buffs.call("get_dose_count", "buff_fullness_mid")) == 0, "food not counted in dose window")
			buffs.call("clear")
		11:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
