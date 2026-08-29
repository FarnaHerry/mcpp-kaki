# 境界表外抽 harness（data/realms.json + DataLoader get_all_realms/get_realm_tuning
# + CultivationSystem ensure_defs_loaded）：
# ① DataLoader 契约：13 行数组 + tuning 五项 ② 行为不变：经验上限/灵力底数/攻防速乘区
# ③ 全表 JSON↔兜底同步（caps/stats/mana_base 逐项，防两表漂移）④ 灵力回复走 mana_regen_pct
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _raw: Object = null

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

func _dl():
	return current_scene.find_child("DataLoader", true, false)

func _cult():
	return root.find_child("Player", true, false).call("get_cultivation")

# 兜底表基准（cultivation_system.cpp REALM_ROWS，JSON 必须与之同步）
const EXP_CAPS := [9, 99, 999, 3999, 13999, 43999, 143999, 443999, 1443999, 0, 99999, 999999, 0]
const EXP_DMG := [1.0, 2.0, 3.5, 6.0, 10.0, 16.0, 25.0, 40.0, 65.0, 90.0, 140.0, 220.0, 350.0]
const EXP_DEF := [1.0, 1.5, 2.5, 4.0, 7.0, 11.0, 17.0, 27.0, 42.0, 60.0, 90.0, 150.0, 250.0]
const EXP_SPD := [1.0, 1.1, 1.2, 1.4, 1.6, 1.9, 2.2, 2.6, 3.0, 3.5, 4.5, 6.0, 8.0]
const EXP_MANA := [0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 3000.0, 6000.0, 20000.0]

func _feq(a, b) -> bool:
	return abs(float(a) - float(b)) < 0.001

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	match _step:
		0:
			_next = _t + 0.5 # 等 bootstrap call_deferred 装配完成
			_step = 1
		1:
			# ① DataLoader 契约
			var dl = _dl()
			_check(dl != null, "DataLoader 存在")
			var realms = dl.call("get_all_realms")
			_check(realms.size() == 13, "realms 行数=13（实际 %d）" % realms.size())
			if realms.size() == 13:
				_check(String(realms[0]["name"]) == "凡人" and String(realms[12]["name"]) == "天尊",
					"境界名首尾=凡人/天尊")
				_check(int(realms[11]["cap"]) == 999999, "金仙 cap=999999（9 系门槛）")
				_check(_feq(realms[10]["mana_base"], 3000.0), "真仙 mana_base=3000")
			var tuning = dl.call("get_realm_tuning")
			_check(tuning.size() > 0, "realm_tuning 非空")
			_check(_feq(tuning["stage_factor"][3], 1.2), "tuning.stage_factor[3]=1.2")
			_check(_feq(tuning["hunyuan_dmg"], 400.0) and _feq(tuning["hunyuan_def"], 300.0)
				and _feq(tuning["hunyuan_spd"], 8.0), "tuning 混元三值")
			_check(_feq(tuning["mana_regen_pct"], 0.02), "tuning.mana_regen_pct=0.02")
			_step = 2
		2:
			# ③ 全表 JSON↔兜底同步（防漂移）
			var realms = _dl().call("get_all_realms")
			var ok_cap := true
			var ok_stats := true
			for i in range(13):
				if int(realms[i]["cap"]) != EXP_CAPS[i]:
					ok_cap = false
					print("[FAIL] cap 漂移 realm", i, ": ", realms[i]["cap"], " != ", EXP_CAPS[i])
				if not _feq(realms[i]["dmg"], EXP_DMG[i]) or not _feq(realms[i]["def"], EXP_DEF[i]) \
						or not _feq(realms[i]["spd"], EXP_SPD[i]) or not _feq(realms[i]["mana_base"], EXP_MANA[i]):
					ok_stats = false
					print("[FAIL] stats 漂移 realm", i)
			_check(ok_cap, "13 行 cap 与兜底表全同步")
			_check(ok_stats, "13 行 dmg/def/spd/mana_base 与兜底表全同步")
			_step = 3
		3:
			# ② 行为不变：境界表各项走 JSON 后数值照旧
			# 用裸 CultivationSystem 实例（无功法/宗门/分叉乘区干扰）做绝对值断言；
			# Player 挂的默认功法带 ×1.05 灵力乘区，会随境界改层数，不适合做比值锚点
			var cult = _cult()
			_check(int(cult.call("get_max_energy")) == 9, "凡人经验上限=9")
			_check(_feq(cult.call("get_damage_multiplier"), 1.0), "凡人攻乘区=1")
			var raw = ClassDB.instantiate("CultivationSystem")
			root.add_child(raw)
			raw.call("set_realm", 2) # 筑基
			_check(String(raw.call("get_realm_name")) == "筑基", "境界名=筑基（LOC 查键不破）")
			_check(_feq(raw.call("get_max_mana"), 200.0), "筑基灵力底数=200")
			_check(_feq(raw.call("get_damage_multiplier"), 3.5), "筑基攻乘区=3.5（前期×1.0）")
			_check(_feq(raw.call("get_defense_multiplier"), 2.5), "筑基防乘区=2.5")
			_check(_feq(raw.call("get_speed_multiplier"), 1.2), "筑基速乘区=1.2")
			raw.call("set_realm", 9) # 渡劫（无经验条）
			_check(int(raw.call("get_max_energy")) == 0, "渡劫无经验条 cap=0")
			_check(_feq(raw.call("get_max_mana"), 900.0), "渡劫灵力底数=900（原 switch default=realm×100）")
			raw.call("set_realm", 10)
			_check(_feq(raw.call("get_max_mana"), 3000.0), "真仙灵力底数=3000（原 switch 特例）")
			raw.call("set_realm", 11)
			_check(_feq(raw.call("get_max_mana"), 6000.0), "金仙灵力底数=6000（原 switch 特例）")
			raw.call("set_realm", 12)
			_check(_feq(raw.call("get_max_mana"), 20000.0), "天尊灵力底数=20000（原 switch 特例）")
			cult.call("set_realm", 0)
			_raw = raw
			_next = _t + 0.2
			_step = 4
		4:
			# ④ 灵力回复走 mana_regen_pct：筑基满蓝 200 → 10s 回 0.02×10×200=40
			var raw = _raw
			raw.call("set_realm", 2)
			raw.call("set_mana", 0.0)
			raw.call("tick_mana_regen", 10.0)
			_check(_feq(raw.call("get_mana"), 40.0), "回灵 10s=200×0.02×10=40")
			raw.free()
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
