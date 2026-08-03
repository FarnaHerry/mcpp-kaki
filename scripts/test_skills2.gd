# G2 harness: ①炼气授予旋风斩/升龙击 ②旋风斩施放 ③升龙击上跃 ④筑基雷咒投射物
#      ⑤土盾自buff ⑥金丹御剑术3发 ⑦化神金刚不坏无敌 ⑧真仙天雷引装Y槽
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _cast_t := 0.0

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

func _count_projectiles() -> int:
	var n = 0
	for c in current_scene.get_children():
		if c.has_method("set_source") and not (c is CharacterBody2D):
			n += 1
	return n

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	paused = false # 机缘事件 overlay 会暂停树（冻结 Player._time），harness 强制解除
	# 叙事 overlay 逐行需 interact/attack/jump 推进——脉冲 interact 自动翻过排队事件
	if int(_t * 10) % 2 == 0:
		Input.action_press("interact")
	else:
		Input.action_release("interact")

	match _step:
		1:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			_breakthrough_to(p.call("get_cultivation"), 1) # 炼气
			var sk = p.call("get_skills")
			_check(sk.call("is_known", "xuan_feng_zhan"), "炼气授予旋风斩")
			_check(sk.call("is_known", "sheng_long_ji"), "炼气授予升龙击")
			# 旋风斩施放（装 A 槽）
			_check(sk.call("assign", 0, "xuan_feng_zhan"), "旋风斩装 A 槽")
			_check(sk.call("cast_slot", 0), "旋风斩施放 ok")
		2:
			_next = _t + 0.3
			# 升龙击：上跃速度
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			sk.call("assign", 1, "sheng_long_ji")
			_check(sk.call("cast_slot", 1), "升龙击施放 ok")
			_check(p.get("velocity").y < -200.0, "升龙击向上跃起 (vel.y < -200)")
		3:
			_next = _t + 0.3
			# 筑基：雷咒/土盾
			var p = root.find_child("Player", true, false)
			_breakthrough_to(p.call("get_cultivation"), 2)
			var sk = p.call("get_skills")
			_check(sk.call("is_known", "lei_zhou_shu"), "筑基授予雷咒术")
			_check(sk.call("is_known", "tu_dun_shu"), "筑基授予土盾术")
			var cult = p.call("get_cultivation")
			cult.call("set_mana", 200.0)
			sk.call("assign", 2, "lei_zhou_shu")
			var n0 = _count_projectiles()
			_check(sk.call("cast_slot", 2), "雷咒术施放 ok")
			_check(_count_projectiles() > n0, "雷咒术生成投射物")
		4:
			_next = _t + 0.3
			# 土盾：自 buff（同名刷新不叠加走 BuffSystem）
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			var buffs = p.call("get_buffs")
			sk.call("assign", 3, "tu_dun_shu")
			_check(sk.call("cast_slot", 3), "土盾术施放 ok")
			_check(buffs.call("has", "buff_tu_dun"), "土盾 buff 生效")
			_check(abs(float(buffs.call("get_def_mult")) - 1.30) < 0.001, "土盾防御 +30%")
		5:
			_next = _t + 0.3
			# 金丹：御剑术 3 发扇形
			var p = root.find_child("Player", true, false)
			_breakthrough_to(p.call("get_cultivation"), 3)
			var sk = p.call("get_skills")
			_check(sk.call("is_known", "yu_jian_shu"), "金丹授予御剑术")
			var cult = p.call("get_cultivation")
			cult.call("set_mana", 200.0)
			sk.call("assign", 2, "yu_jian_shu")
			var n0 = _count_projectiles()
			_check(sk.call("cast_slot", 2), "御剑术施放 ok")
			var n1 = _count_projectiles()
			print("[TEST] projectiles: ", n0, " -> ", n1)
			_check(n1 == n0 + 3, "御剑术 3 发扇形")
		6:
			_next = _t + 0.3
			# 化神：金刚不坏无敌
			var p = root.find_child("Player", true, false)
			_breakthrough_to(p.call("get_cultivation"), 5)
			var sk = p.call("get_skills")
			_check(sk.call("is_known", "jin_gang_bu_huai"), "化神授予金刚不坏")
			_check(sk.call("is_known", "san_mei_zhen_huo"), "化神授予三昧真火")
			sk.call("assign", 6, "jin_gang_bu_huai")
			_check(sk.call("cast_slot", 6), "金刚不坏施放 ok")
			_cast_t = _t
			_check(p.call("is_invulnerable"), "无敌窗口生效")
			var h0 = float(p.call("get_current_health"))
			p.call("take_damage", 5.0, null)
			_check(float(p.call("get_current_health")) == h0, "无敌期间不掉血")
		7:
			# 轮询无敌到期（窗口 2.5s；宽限到 8s 容忍掉帧/事件停顿）
			var p = root.find_child("Player", true, false)
			if p.call("is_invulnerable"):
				if _t > _cast_t + 8.0:
					_check(false, "无敌窗口到期（8s 仍未消失）")
				else:
					_step -= 1 # 下一帧继续轮询
					_next = _t
			else:
				var held = _t - _cast_t
				print("[TEST] invuln held ~", snapped(held, 0.1), "s wall")
				_check(held > 1.5, "无敌窗口持续 ~2.5s（非瞬间消失）")
				_check(true, "无敌窗口到期")
				_next = _t + 0.2
		8:
			_next = _t + 0.3
			# 真仙：天雷引装 Y 槽
			var p = root.find_child("Player", true, false)
			_breakthrough_to(p.call("get_cultivation"), 10)
			var sk = p.call("get_skills")
			_check(sk.call("is_known", "tian_lei_yin"), "真仙授予天雷引")
			var s7 = sk.call("get_slot_info", 7)
			_check(String(s7.get("id")) == "tian_lei_yin", "天雷引自动装 Y 槽")
		10:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
