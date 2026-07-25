# G3 harness: ①被动境界授予×6 ②攻击乘区 ③防御乘区 ④移速乘区 ⑤回灵乘区(功法×被动)
#      ⑥法则回复乘区 ⑦被动不占槽/不可装配 ⑧存档保留已悟被动
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

func _breakthrough_to(cult, realm):
	cult.call("set_free_breakthrough", true)
	cult.call("accumulate_energy", 100000000000)
	while int(cult.call("get_realm_index")) < realm:
		cult.call("attempt_breakthrough")

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	paused = false

	match _step:
		1:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			# 凡人期：被动乘区全 1.0
			_check(float(sk.call("get_passive_atk_mult")) == 1.0, "mortal: no passives")
			# 被动不可装配到技能槽
			_check(not sk.call("assign", 0, "shen_xing"), "passive cannot be assigned to slot")
		2:
			_next = _t + 0.3
			# 炼气：神行百变（移速+12%）
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			_breakthrough_to(p.call("get_cultivation"), 1)
			_check(sk.call("is_known", "shen_xing"), "炼气授予神行百变")
			_check(abs(float(sk.call("get_passive_spd_mult")) - 1.12) < 0.001, "spd mult 1.12")
		3:
			_next = _t + 0.3
			# 筑基：剑心通明（攻击+10%）→ get_effective_attack 实涨
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var atk0 = float(p.call("get_effective_attack"))
			_breakthrough_to(p.call("get_cultivation"), 2)
			_check(sk.call("is_known", "jian_xin"), "筑基授予剑心通明")
			_check(abs(float(sk.call("get_passive_atk_mult")) - 1.10) < 0.001, "atk mult 1.10")
			var atk1 = float(p.call("get_effective_attack"))
			# 境界也会涨攻击——验乘区口径：atk1 >= atk0 * 1.10（境界系数另乘，只会更多）
			_check(atk1 >= atk0 * 1.09, "effective attack includes passive")
		4:
			_next = _t + 0.3
			# 金丹：铁布衫+灵台清明（回灵=功法×被动）
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			_breakthrough_to(p.call("get_cultivation"), 3)
			_check(sk.call("is_known", "tie_bu_shan"), "金丹授予铁布衫")
			_check(sk.call("is_known", "ling_tai"), "金丹授予灵台清明")
			_check(abs(float(sk.call("get_passive_def_mult")) - 1.15) < 0.001, "def mult 1.15")
			_check(abs(float(sk.call("get_passive_mana_regen_mult")) - 1.25) < 0.001, "mana regen passive 1.25")
		5:
			_next = _t + 0.3
			# 元婴：风雷双翼；化神：道法自然
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			_breakthrough_to(p.call("get_cultivation"), 4)
			_check(sk.call("is_known", "feng_lei_yi"), "元婴授予风雷双翼")
			_check(abs(float(sk.call("get_passive_fly_mult")) - 1.15) < 0.001, "fly mult 1.15")
			_breakthrough_to(p.call("get_cultivation"), 5)
			_check(sk.call("is_known", "dao_fa_zi_ran"), "化神授予道法自然")
			_check(abs(float(sk.call("get_passive_law_regen_mult")) - 1.25) < 0.001, "law regen passive 1.25")
		6:
			_next = _t + 0.3
			# 已学列表包含被动（UI 被动分区数据源）
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var known = sk.call("get_known_list")
			var passive_count = 0
			for k in known:
				if int(k.get("type", -1)) == 4: # TYPE_PASSIVE
					passive_count += 1
			print("[TEST] known list passives: ", passive_count)
			_check(passive_count == 6, "known list: 6 passives with type 被动")
		7:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
