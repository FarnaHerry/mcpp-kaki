# 验证功法品级体系定案：
# ①四阶层数精简 黄3/玄4/地5/天6（定义表） ②天品×2 存在且大乘(realm 8)自动领悟换装
# ③老档层数超新上限 clamp（load 截断不丢档） ④飞升真仙(realm 10)全功法晋升仙品
#   （品级显「仙品」/名前缀「仙·」/每层效果 ×1.5） ⑤幂等（重复 realm_changed 不叠加）
# ⑥存档往返仙化保持（xian_promoted 字段） ⑦飞升后新学功法立即仙化
# ⑧先天仙品 大品天仙诀：grade==4 存在、常规 equip 拒绝、不被晋升逻辑重复处理
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

func _g():
	return root.find_child("Player", true, false).call("get_gongfa")

func _cult():
	return root.find_child("Player", true, false).call("get_cultivation")

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var g = _g()
			_check(g != null, "gongfa system exists")
			g.call("equip_gongfa", "mang_niu_jin")
			g.call("equip_gongfa", "tu_na_jue")
			# ① 定义表：四阶层数精简 + 天品×2
			var tie = g.call("get_def_info", "tie_bu_shan")
			var jg = g.call("get_def_info", "jin_gang_jue")
			var lx = g.call("get_def_info", "long_xiang_gong")
			var tq = g.call("get_def_info", "tai_qing_jing")
			print("[TEST] tie_bu_shan=", tie, " jin_gang_jue=", jg)
			print("[TEST] long_xiang_gong=", lx, " tai_qing_jing=", tq)
			_check(int(tie.get("max_layer")) == 4, "玄品 铁布衫 max_layer 4（精简）")
			_check(int(jg.get("max_layer")) == 5, "地品 金刚诀 max_layer 5（精简）")
			_check(int(lx.get("grade")) == 3 and int(lx.get("max_layer")) == 6, "天品 龙象功 存在 max_layer 6")
			_check(int(tq.get("grade")) == 3 and int(tq.get("max_layer")) == 6, "天品 太清经 存在 max_layer 6")
			_check(int(lx.get("school")) == 0 and int(tq.get("school")) == 1, "天品 炼体/练气 各一")
			# ⑧ 先天仙品 大品天仙诀：存在、grade==4（仙）
			var dp = g.call("get_def_info", "da_pin_tian_xian_jue")
			print("[TEST] da_pin_tian_xian_jue=", dp)
			_check(not dp.is_empty() and int(dp.get("grade")) == 4, "大品天仙诀 存在且品级==仙(4)")
			_check(bool(dp.get("innate_xian")), "大品天仙诀 先天仙品标记")
			_check(String(dp.get("grade_name")) == "仙品", "大品天仙诀 grade_name 仙品")
		2:
			_next = _t + 0.3
			var g = _g()
			# ⑧ 先天仙品常规途径拒绝习得
			_check(not bool(g.call("equip_gongfa", "da_pin_tian_xian_jue")), "大品天仙诀 常规 equip 拒绝")
			# ③ 老档层数 clamp：金刚诀旧档 7 层（旧上限）→ 截 5；known 太玄经 9 层 → 截 5
			g.call("load_from_dict", {
				"body": {"id": "jin_gang_jue", "layer": 7, "prof": 0.0},
				"known": {"tai_xuan_jing": {"layer": 9, "prof": 50.0}},
			})
			var b = g.call("get_slot_info", 0)
			print("[TEST] clamped body: ", b)
			_check(String(b.get("id")) == "jin_gang_jue" and int(b.get("layer")) == 5, "老档 7 层 clamp 到 5（不丢档）")
			g.call("equip_gongfa", "tai_xuan_jing")
			var q = g.call("get_slot_info", 1)
			_check(String(q.get("id")) == "tai_xuan_jing" and int(q.get("layer")) == 5, "known 9 层 clamp 到 5")
		3:
			_next = _t + 0.3
			var g = _g()
			# ② 大乘（realm 8）自动领悟天品并换装
			_cult().call("set_realm", 8)
			var b = g.call("get_slot_info", 0)
			var q = g.call("get_slot_info", 1)
			print("[TEST] after realm 8: body=", b.get("id"), " qi=", q.get("id"))
			_check(String(b.get("id")) == "long_xiang_gong", "大乘自动领悟换装 龙象功（炼体）")
			_check(String(q.get("id")) == "tai_qing_jing", "大乘自动领悟换装 太清经（练气）")
			_check(String(b.get("grade_name")) == "天品", "龙象功 显示天品")
			_check(not bool(b.get("xian")), "未飞升前非仙品显示")
			_check(not bool(g.call("is_xian_promoted")), "未飞升 xian_promoted=false")
		4:
			_next = _t + 0.3
			var g = _g()
			# ④ 飞升真仙（realm 10）→ 全功法晋升仙品
			_cult().call("set_realm", 10)
			_check(bool(g.call("is_xian_promoted")), "飞升后 xian_promoted=true")
			var b = g.call("get_slot_info", 0)
			print("[TEST] promoted body: ", b)
			_check(String(b.get("grade_name")) == "仙品", "晋升后品级显「仙品」")
			_check(String(b.get("name")).begins_with("仙·"), "晋升后名前缀「仙·」")
			_check(bool(b.get("xian")), "slot info xian 标记")
			# 乘区：龙象功 1 层 hp 0.12 → 仙化 ×1.5 → 1+0.18=1.18
			var hp = float(g.call("get_hp_mult"))
			print("[TEST] hp_mult promoted=", hp)
			_check(abs(hp - 1.18) < 0.001, "仙化每层效果 ×1.5（龙象功 1 层 hp 1.18）")
		5:
			_next = _t + 0.3
			var g = _g()
			# ⑤ 幂等：重复 realm_changed 不叠加（×1.5 不再 ×1.5）
			var bus = root.find_child("SignalBus", true, false)
			bus.emit_signal("realm_changed", 10, 10, "真仙")
			bus.emit_signal("realm_changed", 10, 11, "金仙")
			var hp = float(g.call("get_hp_mult"))
			_check(abs(hp - 1.18) < 0.001, "重复 realm_changed 幂等（乘区不叠加）")
			_check(bool(g.call("is_xian_promoted")), "幂等后仍 xian_promoted")
			# ⑧ 先天仙品不被晋升逻辑重复处理：仍不可习得、grade 恒为 4
			_check(not bool(g.call("equip_gongfa", "da_pin_tian_xian_jue")), "飞升后大品天仙诀仍常规拒绝")
			_check(int(g.call("get_def_info", "da_pin_tian_xian_jue").get("grade")) == 4, "大品天仙诀 grade 恒为仙（不被改写）")
		6:
			_next = _t + 0.3
			var g = _g()
			# ⑥ 存档往返：xian_promoted 字段持久化
			var d = g.call("save_to_dict")
			_check(bool(d.get("xian_promoted")), "存档含 xian_promoted=true")
			g.call("load_from_dict", d)
			_check(bool(g.call("is_xian_promoted")), "读档后仙化保持")
			var hp = float(g.call("get_hp_mult"))
			_check(abs(hp - 1.18) < 0.001, "读档后乘区保持 ×1.5")
			var b = g.call("get_slot_info", 0)
			_check(String(b.get("name")).begins_with("仙·"), "读档后名前缀保持")
		7:
			_next = _t + 0.3
			var g = _g()
			# ⑦ 飞升后新学/换装功法立即仙化：铁布衫（从未学过）
			g.call("equip_gongfa", "tie_bu_shan")
			var b = g.call("get_slot_info", 0)
			print("[TEST] post-ascension new equip: ", b)
			_check(String(b.get("name")).begins_with("仙·"), "飞升后新学功法立即仙化（仙·铁布衫）")
			_check(String(b.get("grade_name")) == "仙品", "飞升后新学功法显仙品")
			# 铁布衫 1 层 hp 0.06 ×1.5 → 1.09
			_check(abs(float(g.call("get_hp_mult")) - 1.09) < 0.001, "新学功法乘区立即 ×1.5")
		8:
			_next = _t + 0.3
			var g = _g()
			# 老档迁移：存档无 xian_promoted 字段但 realm 已 10 → 读档自动补晋升
			#（body/qi 装满天品，防 _check_current_realm 补领悟换装干扰断言）
			g.call("load_from_dict", {
				"body": {"id": "long_xiang_gong", "layer": 2, "prof": 0.0},
				"qi": {"id": "tai_qing_jing", "layer": 1, "prof": 0.0},
			})
			_check(bool(g.call("is_xian_promoted")), "老档（无标记）读档按当前境界补晋升")
			# 龙象功 2 层 hp 0.12×2 ×1.5 → 1.36
			_check(abs(float(g.call("get_hp_mult")) - 1.36) < 0.001, "老档补晋升乘区生效")
		9:
			print("[TEST] DONE fail=", _fail)
			if _fail == 0:
				print("[TEST] ALL PASS")
			return true
	return false
