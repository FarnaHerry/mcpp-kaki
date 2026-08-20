# 大品天仙诀获得线（斜月三星洞·菩提祖师传法）测试：
# ①define：大品天仙诀 存在且先天仙品 ②门槛前交互被拒（realm 不足 → 拒绝叙事，未习得）
# ③门槛满足 → 传法 → 习得 + 装备成功 ④幂等（重复交互不重复给）⑤效果乘区（灵力/属性对比）
# ⑥存档往返（pd["gongfa"] 段含它）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _held: Array = []
var _mana_before := 0.0

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

func _hold(action: String):
	Input.action_press(action)
	_held.append(action)

func _release_all():
	for a in _held:
		Input.action_release(a)
	_held.clear()

func _player():
	return root.find_child("Player", true, false)

func _gm():
	return root.find_child("GameManager", true, false)

func _cm():
	return root.find_child("ContinentManager", true, false)

func _find(s: String):
	return root.find_child(s, true, false)

func _finish() -> bool:
	if _fail == 0:
		print("[TEST] ALL PASS")
	else:
		print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_release_all()
	_step += 1
	if _step > 60:
		print("[TEST] hard cap")
		return _finish()

	match _step:
		1:
			_next = _t + 0.3
			var p = _player()
			var g = p.call("get_gongfa")
			_check(g != null, "gongfa system exists")
			# ① 定义：大品天仙诀先天仙品
			var dp = g.call("get_def_info", "da_pin_tian_xian_jue")
			print("[TEST] def: ", dp)
			_check(not dp.is_empty(), "大品天仙诀 定义存在")
			_check(int(dp.get("grade")) == 4 and bool(dp.get("innate_xian")), "先天仙品 grade 4")
			_check(int(dp.get("school")) == 1, "练气系（qi）")
			_check(not bool(g.call("equip_gongfa", "da_pin_tian_xian_jue")), "常规 equip 仍拒绝（未获途径）")
			# 初始：升到金丹（realm 3）解锁西牛贺洲
			_check(not bool(_gm().call("has_flag", "da_pin_tian_xian_jue_learned")), "初始无传法 flag")
			_player().call("get_cultivation").call("set_realm", 3)
			_check(_cm().call("travel_to_direct", "xiniuhe"), "travel 西牛贺洲")
			_next = _t + 1.5
		2:
			_next = _t + 0.3
			_check(_find("PuTiZuShi") == null, "三星洞不在当前场景（verify）")
			# 进三星洞（入口 x=1950）
			_player().global_position = Vector2(1950, 205)
			_next = _t + 0.5
		3:
			_hold("up")
			_next = _t + 0.5
		4:
			var p = _player()
			_check(p.get_parent() != current_scene, "玩家进入三星洞（父节点≠洲根）")
			_check(_find("PuTiZuShi") != null, "菩提祖师 NarrativeNode 存在")
			_next = _t + 0.3
		5:
			# ② 门槛前：realm < 11（当前 炼气/金丹 均不足）→ 拒绝叙事，未习得
			var cs = _player().call("get_cultivation")
			print("[TEST] realm=", cs.call("get_realm_index"))
			_check(int(cs.call("get_realm_index")) < 11, "当前境界未至金仙")
			_player().global_position = Vector2(60, 230)
			_next = _t + 0.5
		6:
			_hold("interact")
			_next = _t + 0.3
		7:
			var pt = _find("PuTiZuShi")
			_check(bool(pt.call("is_overlay_open")), "门槛前交互=拒绝叙事开")
			_check(String(pt.call("get_current_line")).contains("金仙"), "拒绝原因=修为未至金仙")
			_hold("interact") # 关闭
			_next = _t + 0.3
		8:
			var pt = _find("PuTiZuShi")
			var g = _player().call("get_gongfa")
			_check(not bool(pt.call("is_overlay_open")), "拒绝叙事关闭（未落 flag）")
			_check(not bool(_gm().call("has_flag", "da_pin_tian_xian_jue_learned")), "拒绝未落 flag")
			var q = g.call("get_slot_info", 1)
			_check(String(q.get("id")) != "da_pin_tian_xian_jue", "未习得（qi 槽非大品天仙诀）")
			# ③ 门槛满足：金仙（realm 11）
			_player().call("get_cultivation").call("set_realm", 11)
			_next = _t + 0.3
		9:
			var p = _player()
			_check(int(p.call("get_cultivation").call("get_realm_index")) == 11, "境界=金仙")
			# 飞升仙化已触发（realm 10+）；主角 qi 槽可能被太清经占据——传法会换装
			var before = float(p.call("get_cultivation").call("get_max_mana"))
			print("[TEST] mana before=", before)
			_mana_before = before
			_hold("interact")
			_next = _t + 0.3
		10:
			var pt = _find("PuTiZuShi")
			_check(bool(pt.call("is_overlay_open")), "传法叙事开")
			_check(String(pt.call("get_current_line")).begins_with("你一路行来"), "传法首行")
			_hold("interact")
			_next = _t + 0.25
		11:
			_hold("interact")
			_next = _t + 0.25
		12:
			_hold("interact")
			_next = _t + 0.25
		13:
			_hold("interact")
			_next = _t + 0.25
		14:
			_hold("interact") # 第 5 行后关闭 → gm_method 回调
			_next = _t + 0.3
		15:
			var pt = _find("PuTiZuShi")
			var g = _player().call("get_gongfa")
			_check(not bool(pt.call("is_overlay_open")), "传法叙事关闭")
			_check(bool(_gm().call("has_flag", "da_pin_tian_xian_jue_learned")), "传法 once_flag 落档")
			var q = g.call("get_slot_info", 1)
			print("[TEST] qi slot: ", q)
			_check(String(q.get("id")) == "da_pin_tian_xian_jue", "习得并装备 qi 槽=大品天仙诀")
			_check(int(q.get("layer")) == 1, "层数从 1 起")
			_check(bool(q.get("xian")) and String(q.get("grade_name")) == "仙品", "先天仙品显示")
			var after = float(_player().call("get_cultivation").call("get_max_mana"))
			print("[TEST] mana after=", after, " (before=", _mana_before, ")")
			_check(abs(float(g.call("get_mana_mult")) - 1.18) < 0.001, "装备后灵力乘区 1.18（先天仙品 0.18/层，不乘 1.5）×1layer=1.18")
			_next = _t + 0.3
		16:
			var g = _player().call("get_gongfa")
			# ⑤ 效果乘区：先天仙品 1 层不吃 ×1.5 仙化（数值即仙品档）
			# 灵力 0.18×1=1.18；回灵 0.15×1=1.15；法强 0.12×1=1.12；速度 0.05×1=1.05
			var mana = float(g.call("get_mana_mult"))
			var regen = float(g.call("get_regen_mult"))
			var spell = float(g.call("get_spell_mult"))
			var spd = float(g.call("get_speed_mult"))
			print("[TEST] mults: mana=", mana, " regen=", regen, " spell=", spell, " spd=", spd)
			_check(abs(mana - 1.18) < 0.001, "灵力 ×1.18（0.18/层 不乘 1.5）")
			_check(abs(regen - 1.15) < 0.001, "回灵 ×1.15")
			_check(abs(spell - 1.12) < 0.001, "法强 ×1.12")
			_check(abs(spd - 1.05) < 0.001, "速度 ×1.05")
			# ④ 幂等：再交互 → after_lines（不再次触发 gm_method）
			_hold("interact")
			_next = _t + 0.3
		17:
			var pt = _find("PuTiZuShi")
			_check(bool(pt.call("is_overlay_open")), "传法后再交互=after_lines 开")
			_check(String(pt.call("get_current_line")).begins_with("大品天仙诀已传你身"), "after_lines=授法已毕")
			_hold("interact")
			_next = _t + 0.3
		18:
			var pt = _find("PuTiZuShi")
			_check(not bool(pt.call("is_overlay_open")), "after_lines 关闭")
			var g = _player().call("get_gongfa")
			var q = g.call("get_slot_info", 1)
			_check(String(q.get("id")) == "da_pin_tian_xian_jue" and int(q.get("layer")) == 1, "重复交互层数不变（幂等）")
			# grant_gongfa 二重调用幂等
			_check(not bool(g.call("grant_gongfa", "da_pin_tian_xian_jue")), "grant_gongfa 重复调用返回 false（幂等）")
			# ⑥ 存档往返：pd["gongfa"].qi 含大品天仙诀
			var sd = _gm().call("collect_save_data")
			var pd = sd.get("player", {})
			var gf = pd.get("gongfa", {})
			var qi = gf.get("qi", {})
			print("[TEST] save gongfa qi: ", qi)
			_check(String(qi.get("id", "")) == "da_pin_tian_xian_jue", "存档 pd.gongfa.qi 含大品天仙诀")
			_check(bool(gf.get("xian_promoted", false)), "存档含 xian_promoted（金仙已飞升仙化）")
			# load 往返保持
			var d = g.call("save_to_dict")
			g.call("load_from_dict", d)
			q = g.call("get_slot_info", 1)
			_check(String(q.get("id")) == "da_pin_tian_xian_jue" and int(q.get("layer")) == 1, "save/load 往返保持")
			_check(abs(float(g.call("get_mana_mult")) - 1.18) < 0.001, "往返后乘区保持")
			return _finish()
	return false