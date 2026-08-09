# 仙元体系（飞升后）测试：
# ①凡尘修为不受影响（灵气累计）②渡劫入真仙 → 仙元清零/凡尘修为九九归一/灵力转仙元
# ③仙元累计 + 9 系门槛（真仙 99,999 / 金仙 999,999）④HUD 修为条真仙+ 改显仙元 ⑤存档往返
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

func _cs():
	return _player().call("get_cultivation")

func _gm():
	return root.find_child("GameManager", true, false)

func _xp_label_text() -> String:
	var l = root.find_child("XpLabel", true, false)
	return l.text if l != null else ""

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	_next = _t + 0.4
	if _step > 40:
		print("[TEST] hard cap")
		print("[TEST] ", _fail, " FAILURES") if _fail > 0 else print("[TEST] ALL PASS")
		return true

	match _step:
		1:
			# 凡尘修为不受影响：灵气累计照旧（炼气 cap 99）
			var cs = _cs()
			_check(cs != null, "CultivationSystem 存在")
			_check(not bool(cs.call("is_immortal")), "初始非仙阶")
			_check(int(cs.call("get_xianyuan")) == 0, "初始仙元 0")
			cs.call("set_realm", 1) # 炼气
			cs.call("accumulate_energy", 50.0)
			_check(int(cs.call("get_spiritual_energy")) == 50, "凡尘修为累计 50（灵气）")
			_check(int(cs.call("get_current_energy")) == 50, "凡尘当前修为=灵气 50")
			_check(int(cs.call("get_xianyuan")) == 0, "凡尘累计不进仙元")
			_check(int(cs.call("get_max_energy")) == 99, "炼气门槛 99")
			_check(String(cs.call("get_mana_name")) == "灵力", "凡尘法力名=灵力")
		2:
			# 渡劫过渡态无经验条；渡劫成功 → 真仙（飞升）
			var cs = _cs()
			cs.call("set_realm", 9) # 渡劫
			cs.call("accumulate_energy", 100.0)
			_check(int(cs.call("get_spiritual_energy")) == 50, "渡劫过渡态无经验条（不累计）")
			_check(bool(cs.call("attempt_breakthrough")), "渡劫成功 → 真仙")
		3:
			# 飞升：仙元清零重起数、凡尘修为九九归一、灵力转仙元
			var cs = _cs()
			_check(int(cs.call("get_realm_index")) == 10, "已入真仙(realm10)")
			_check(bool(cs.call("is_immortal")), "is_immortal() = true")
			_check(int(cs.call("get_xianyuan")) == 0, "飞升仙元清零重起数")
			_check(int(cs.call("get_spiritual_energy")) == 0, "凡尘修为九九归一（清零）")
			_check(int(cs.call("get_current_energy")) == 0, "当前修为=仙元 0")
			_check(int(cs.call("get_max_energy")) == 99999, "真仙 9 系门槛 99,999")
			_check(String(cs.call("get_mana_name")) == "仙元", "灵力转仙元（法力名）")
			_check(int(cs.call("get_max_mana")) >= 1000, "真仙仙元池上限≥1000（基础1000×功法乘区，实为 %d）" % int(cs.call("get_max_mana")))
		4:
			# 仙元累计 + 到顶封顶
			var cs = _cs()
			cs.call("accumulate_energy", 500.0)
			_check(int(cs.call("get_xianyuan")) == 500, "仙元累计 500")
			_check(int(cs.call("get_current_energy")) == 500, "仙阶当前修为=仙元")
			_check(int(cs.call("get_spiritual_energy")) == 0, "仙阶累计不进灵气")
			cs.call("accumulate_energy", 100000000.0)
			_check(int(cs.call("get_xianyuan")) == 99999, "仙元到顶封顶 99,999")
		5:
			# HUD：真仙+ 修为条改显仙元
			_check(_xp_label_text().begins_with("仙元"), "HUD 修为条改显「仙元」（实为: " + _xp_label_text() + "）")
			# 9 系门槛：经验不满不可突破
			var cs = _cs()
			cs.call("set_free_breakthrough", false)
			cs.call("set_xianyuan", 99998)
			_check(not bool(cs.call("attempt_breakthrough")), "仙元 99,998 差一丝不可破金仙")
			cs.call("set_xianyuan", 99999)
			_check(bool(cs.call("attempt_breakthrough")), "仙元圆满 99,999 → 金仙")
		6:
			var cs = _cs()
			_check(int(cs.call("get_realm_index")) == 11, "已入金仙(realm11)")
			_check(int(cs.call("get_max_energy")) == 999999, "金仙 9 系门槛 999,999")
			_check(int(cs.call("get_xianyuan")) == 99999, "金仙仙元自 99,999 续累计")
			cs.call("set_xianyuan", 999999)
			_check(String(cs.call("get_stage_name")) == "大圆满", "金仙仙元满=大圆满")
			_check(not bool(cs.call("attempt_breakthrough")), "金仙为常规境界顶（不可再突破）")
			_check(bool(cs.call("attain_hunyuan")), "金仙大圆满 → 混元一气")
			_check(String(cs.call("get_realm_name")) == "混元金仙", "称号=混元金仙")
		7:
			# 存档往返：仙元/境界/混元随档
			var cs = _cs()
			cs.call("set_xianyuan", 123456)
			_gm().call("save_game", "xianyuan_test")
			cs.call("set_xianyuan", 0)
			cs.call("set_realm", 1) # 打乱现场
			_check(int(cs.call("get_xianyuan")) == 0, "读档前现场已打乱")
			_gm().call("load_game", "xianyuan_test")
		9:
			var cs = _cs()
			_check(int(cs.call("get_realm_index")) == 11, "读档恢复金仙")
			_check(int(cs.call("get_xianyuan")) == 123456, "读档恢复仙元 123,456")
			_check(bool(cs.call("is_hunyuan")), "读档恢复混元一气")
			_check(bool(cs.call("is_immortal")), "读档后仍为仙阶")
			_check(_xp_label_text().begins_with("仙元"), "读档后 HUD 仍显「仙元」")
			cs.call("set_free_breakthrough", true) # 还原调试开关
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
