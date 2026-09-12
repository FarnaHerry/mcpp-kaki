# 雷元素管线补全 harness（proj_element：敌人投射物元素结算）：
# ① enemies.json proj_element 加载：雷兽/天罚使="lei"，非雷系（夜叉/竹妖/未知id）=""
# ② 雷兽雷弹走 DMG_ELEMENTAL+ELEM_LEI（cat2/elem6），伤害基数不变（atk14×2.0=28）
# ③ 玩家有雷抗（菩提心法全元素+10%）时雷弹伤害降低（28→25.2），无抗时满额
# ④ 非雷系远程（巡海夜叉）弹仍走物理（cat0/elem0），元素抗性不影响其伤害
# ⑤ 天罚使三阶段弹全走雷元素：普通弹 atk44×1.5=66、雷链扇形弹 ×0.6=26.4，扇面 3/5/7
# 注：纯观察 harness 无按键输入；逐帧扫描投射物（弹 0.6s 内即中玩家，按步采样会漏）
extends SceneTree

const WC = preload("res://scripts/world_common.gd")

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

# 逐帧投射物扫描累积：source 名 → {n, bad, dmg}
var _proj_stats := {}
var _checked := {} # instance_id → true（每颗弹只记一次）
var _fan_max := { 1: 0, 2: 0, 3: 0 }
var _phase := 1

# 受击记录（当前阶段只有一只测试敌在攻击玩家，可按阶段归属）
var _lei_hits_pre: Array = []
var _lei_hits_post: Array = []
var _yecha_hits: Array = []
var _puti_learned := false
var _hit_mode := "" # "lei" / "yecha" / ""（tfs 阶段不断言受击额，只看弹体口径）

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

func _p():
	return root.find_child("Player", true, false)

func _top_up_hp():
	var p = _p()
	if p != null:
		p.call("set_current_health", 999999.0)

func _on_dmg(_pos, amount, is_player_victim):
	if not is_player_victim:
		return
	match _hit_mode:
		"lei":
			if _puti_learned:
				_lei_hits_post.append(float(amount))
			else:
				_lei_hits_pre.append(float(amount))
		"yecha":
			_yecha_hits.append(float(amount))

# 逐帧扫描：记录每颗测试敌投射物的结算口径（cat/elem/dmg），并跟踪天罚使扇形弹同帧峰
func _scan_projectiles():
	var projs = current_scene.find_children("*", "Projectile", true, false)
	var fan_now := 0
	for proj in projs:
		var src = proj.call("get_source")
		if src == null or not is_instance_valid(src):
			continue
		var sname := String(src.name)
		if not sname.begins_with("T_"):
			continue
		if "BossFan" in String(proj.name):
			fan_now += 1
		var id = proj.get_instance_id()
		if _checked.has(id):
			continue
		_checked[id] = true
		var key := sname + ("|fan" if "BossFan" in String(proj.name) else "")
		if not _proj_stats.has(key):
			_proj_stats[key] = { "n": 0, "bad": 0, "dmg": -1.0 }
		var st = _proj_stats[key]
		st["n"] = int(st["n"]) + 1
		var cat := int(proj.call("get_damage_category"))
		var elem := int(proj.call("get_element"))
		if float(st["dmg"]) < 0.0:
			st["dmg"] = float(proj.call("get_damage"))
		# 雷系须 cat2/elem6；非雷系须 cat0/elem0
		var lei_source := sname == "T_LeiShou" or sname == "T_TianFaShi"
		if lei_source:
			if cat != 2 or elem != 6:
				st["bad"] = int(st["bad"]) + 1
		else:
			if cat != 0 or elem != 0:
				st["bad"] = int(st["bad"]) + 1
	if fan_now > 0:
		_fan_max[_phase] = maxi(int(_fan_max[_phase]), fan_now)

# 清掉某测试敌在场上的弹（source 指针随 enemy 释放会悬垂，先清弹再放敌）
func _free_projs_of(sname: String):
	for proj in current_scene.find_children("*", "Projectile", true, false):
		var src = proj.call("get_source")
		if src != null and is_instance_valid(src) and String(src.name) == sname:
			proj.free()

func _stat(key: String) -> Dictionary:
	return _proj_stats.get(key, { "n": 0, "bad": 0, "dmg": -1.0 })

func _process(delta) -> bool:
	_scan_projectiles()
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			# 等 bootstrap；清场：螭龙 Boss（detection 450 会索敌到测试点）入土为安
			var chi = current_scene.find_child("Boss_ChiLong", true, false)
			if chi:
				chi.queue_free()
			var p = _p()
			p.position = Vector2(5150, 200) # 谷底长草地带（4400~9000, y238），邻域无其他敌
			_top_up_hp()
			var bus = root.find_child("SignalBus", true, false)
			bus.connect("damage_dealt", _on_dmg)
			_step = 1
		1:
			# ---- ① proj_element 定义加载（不入树，纯定义装配）----
			for pair in [["lei_shou", "lei"], ["tian_fa_shi", "lei"], ["xun_hai_ye_cha", ""], ["zhu_yao", ""], ["no_such_enemy", ""]]:
				var e = ClassDB.instantiate("Enemy")
				e.set("enemy_id", pair[0])
				_check(String(e.get("proj_element")) == pair[1], "proj_element 加载：%s = \"%s\"" % [pair[0], pair[1]])
				e.free()
			# ---- ② 放雷兽（玩家右侧 150：preferred 180 窗口内，直接开射）----
			var ls = WC.spawn_enemy_by_id(current_scene, Vector2(5300, 200), "lei_shou", "T_LeiShou")
			_check(ls != null and String(ls.get("proj_element")) == "lei", "雷兽生成并承接 proj_element=lei")
			_hit_mode = "lei"
			_step = 2
		2, 3, 4, 5, 6, 7, 8, 9, 10, 11:
			# 雷兽观察窗 ~3s（cd 1.2s → ≥2 发）
			_top_up_hp()
			_step += 1
		12:
			# ---- ② 无抗雷弹满额断言 ----
			_check(_lei_hits_pre.size() >= 1, "雷兽雷弹命中玩家（%d 次）" % _lei_hits_pre.size())
			if _lei_hits_pre.size() >= 1:
				_check(abs(_lei_hits_pre[0] - 28.0) < 0.05, "无雷抗时满额 28（实际 %.2f）——数值基数不变" % _lei_hits_pre[0])
			# ---- ③ 学菩提心法（全元素抗性 +10%）----
			var skills = _p().call("get_skills")
			_check(bool(skills.call("learn", "pu_ti_xin_fa")), "习得菩提心法（雷抗来源）")
			_puti_learned = true
			_step = 13
		13, 14, 15, 16, 17, 18, 19, 20:
			_top_up_hp()
			_step += 1
		21:
			# ---- ③ 有抗雷弹减免断言 ----
			_check(_lei_hits_post.size() >= 1, "有雷抗后雷弹再次命中（%d 次）" % _lei_hits_post.size())
			if _lei_hits_post.size() >= 1:
				_check(_lei_hits_post[0] < _lei_hits_pre[0] - 1.0, "有雷抗伤害低于无抗（%.2f < %.2f）" % [_lei_hits_post[0], _lei_hits_pre[0]])
				_check(abs(_lei_hits_post[0] - 25.2) < 0.1, "雷抗 10%% 减免口径 28→25.2（实际 %.2f）" % _lei_hits_post[0])
			# 收雷兽，放夜叉（非雷系远程对照组）
			_free_projs_of("T_LeiShou")
			var ls = current_scene.find_child("T_LeiShou", true, false)
			if ls:
				ls.queue_free()
			var yc = WC.spawn_enemy_by_id(current_scene, Vector2(5330, 200), "xun_hai_ye_cha", "T_YeCha")
			_check(yc != null and String(yc.get("proj_element")) == "", "夜叉生成且 proj_element 空（物理弹）")
			_hit_mode = "yecha"
			_step = 22
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32:
			# 夜叉观察窗 ~3.3s（cd 1.4s → ≥2 发；玩家已带 10% 全元素抗性）
			_top_up_hp()
			_step += 1
		33:
			# ---- ④ 物理弹不受元素抗性影响 ----
			_check(_yecha_hits.size() >= 2, "夜叉物理弹命中 ≥2 次（%d）" % _yecha_hits.size())
			if _yecha_hits.size() >= 2:
				_check(abs(_yecha_hits[0] - _yecha_hits[1]) < 0.05, "物理弹伤害不随元素抗性变化（%.2f vs %.2f）" % [_yecha_hits[0], _yecha_hits[1]])
			# 收夜叉，放天罚使
			_free_projs_of("T_YeCha")
			var yc = current_scene.find_child("T_YeCha", true, false)
			if yc:
				yc.queue_free()
			var tfs = WC.spawn_enemy_by_id(current_scene, Vector2(5450, 200), "tian_fa_shi", "T_TianFaShi")
			_check(tfs != null and String(tfs.get("proj_element")) == "lei", "天罚使生成并承接 proj_element=lei")
			_hit_mode = ""
			_phase = 1
			_step = 34
		34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53:
			# ---- ⑤ 一相窗 ~6s：普通雷弹 + 3 发扇形弹 ----
			_top_up_hp()
			_step += 1
		54:
			var tfs = current_scene.find_child("T_TianFaShi", true, false)
			if tfs:
				tfs.set("boss_phase", 2)
			_phase = 2
			_step = 55
		55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74:
			# ---- ⑤ 二相窗 ~6s：5 发扇形雷弹 ----
			_top_up_hp()
			_step += 1
		75:
			var tfs = current_scene.find_child("T_TianFaShi", true, false)
			if tfs:
				tfs.set("boss_phase", 3)
			_phase = 3
			_step = 76
		76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95:
			# ---- ⑤ 三相窗 ~6s：7 发扇形雷弹 ----
			_top_up_hp()
			_step += 1
		96:
			# ---- 汇总断言：投射物结算口径 + 数值基数 ----
			var ls_st = _stat("T_LeiShou")
			_check(int(ls_st["n"]) >= 1, "雷兽雷弹已扫描（%d 颗）" % int(ls_st["n"]))
			_check(int(ls_st["bad"]) == 0, "雷兽弹全部 DMG_ELEMENTAL+ELEM_LEI（bad=%d）" % int(ls_st["bad"]))
			_check(abs(float(ls_st["dmg"]) - 28.0) < 0.05, "雷兽弹基数 atk14×2.0=28（实际 %.2f）" % float(ls_st["dmg"]))
			var yc_st = _stat("T_YeCha")
			_check(int(yc_st["n"]) >= 1, "夜叉弹已扫描（%d 颗）" % int(yc_st["n"]))
			_check(int(yc_st["bad"]) == 0, "夜叉弹全部物理口径（bad=%d）" % int(yc_st["bad"]))
			_check(abs(float(yc_st["dmg"]) - 40.0) < 0.05, "夜叉弹基数 atk16×2.5=40（实际 %.2f）" % float(yc_st["dmg"]))
			var tfs_st = _stat("T_TianFaShi")
			_check(int(tfs_st["n"]) >= 1, "天罚使普通雷弹已扫描（%d 颗）" % int(tfs_st["n"]))
			_check(int(tfs_st["bad"]) == 0, "天罚使普通弹全走雷元素（bad=%d）" % int(tfs_st["bad"]))
			_check(abs(float(tfs_st["dmg"]) - 66.0) < 0.05, "天罚使普通弹基数 atk44×1.5=66（实际 %.2f）" % float(tfs_st["dmg"]))
			var fan_st = _stat("T_TianFaShi|fan")
			_check(int(fan_st["n"]) >= 1, "天罚使雷链扇形弹已扫描（%d 颗）" % int(fan_st["n"]))
			_check(int(fan_st["bad"]) == 0, "天罚使扇形弹全走雷元素（bad=%d）" % int(fan_st["bad"]))
			_check(abs(float(fan_st["dmg"]) - 26.4) < 0.05, "扇形弹基数 atk44×0.6=26.4（实际 %.2f）" % float(fan_st["dmg"]))
			_check(int(_fan_max[1]) >= 2, "一相扇面 3 发（同帧峰=%d，容许即中）" % int(_fan_max[1]))
			_check(int(_fan_max[2]) >= 4, "二相扇面 5 发（同帧峰=%d）" % int(_fan_max[2]))
			_check(int(_fan_max[3]) >= 6, "三相扇面 7 发（同帧峰=%d）" % int(_fan_max[3]))
			_step = 97
		97:
			# 清场收工
			_free_projs_of("T_TianFaShi")
			var tfs = current_scene.find_child("T_TianFaShi", true, false)
			if tfs:
				tfs.queue_free()
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
