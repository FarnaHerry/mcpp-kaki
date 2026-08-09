# 平衡遗留修复回归（session 011 遗留清单）：
#   ①三灾元素结算（元素抗性可减免/物理防御不可，雷LEI/火HUO/风FENG 同口径）
#   ②普攻走 get_effective_attack 全乘区（装备攻击加成生效）
#   ③recipes.json 真接入（AlchemySystem 与 DataLoader 同源，8 方含玄龙丹）
#   ④is_boss ×5 时序陷阱修复（add_child 前后置位都恰好 ×5 一次）
#   ⑤boss 掉落走 boss 表（enemy_killed → DropSystem 链路回归；千年灵芝 boss 表独占 100%）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

var _enemy = null      # 普攻沙包
var _hp_mark := 0.0
var _loss1 := -1.0
var _eff1 := 0.0
var _baseline := {}

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

func _equip(item_id: String) -> bool:
	var p = _player()
	var inv = p.call("get_inventory")
	inv.call("add_item", item_id, 1)
	for i in range(inv.call("get_capacity")):
		var sd = inv.call("get_slot", i)
		if not sd.is_empty() and String(sd["id"]) == item_id:
			return bool(p.call("equip_item", i))
	return false

func _near_combo_mult(loss: float, eff: float) -> bool:
	# 合成输入在一次按下里可能触多段连击（缓冲连段），有效倍率必落在 {1.0, 1.4, 2.0}
	for m in [1.0, 1.4, 2.0]:
		if abs(loss - eff * m) < 0.6:
			return true
	return false

func _make_dummy(pos: Vector2):
	# 专用沙包：无 AI、无碰撞体（防 depenetration 弹飞玩家）、HurtBox 正常挨打
	var e = ClassDB.instantiate("Enemy")
	e.set("max_health", 100000.0)
	e.set("move_speed", 0.0)
	e.set("detection_radius", 0.0)
	e.set("attack_cooldown", 9999.0)
	e.position = pos
	current_scene.add_child(e)
	e.set_collision_layer_value(4, false) # 只留 HurtBox(Area) 挨揍，身体不顶人
	return e

func _find_sandbag():
	return _enemy


func _snap_pickups() -> Dictionary:
	var d = {}
	for c in current_scene.get_children():
		if c.has_method("get_item_id"):
			d[String(c.call("get_item_id")) + "@" + str(c.global_position)] = String(c.call("get_item_id"))
	return d

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false

	match _step:
		0:
			# ---------- ③ recipes.json 接入 ----------
			_next = _t + 0.3
			print("[TEST] scene=", current_scene.name, " php=", _player().call("get_current_health"), " ppos=", _player().position)
			var p = _player()
			var al = p.call("get_alchemy")
			var list = al.call("get_recipe_list")
			_check(list.size() == 8, "alchemy: 8 配方（含玄龙丹）")
			var dl = root.find_child("DataLoader", true, false)
			var dl_ids = {}
			for d in dl.call("get_all_recipes"):
				dl_ids[String(d["id"])] = true
			var match_src = dl_ids.size() == list.size()
			for r in list:
				if not dl_ids.has(String(r["id"])):
					match_src = false
			print("[TEST] dl_ids=", dl_ids.keys(), " al_ids=", list.map(func(r): return String(r["id"])))
			_check(match_src, "alchemy: 配方与 DataLoader(recipes.json) 同源")
			for r in list:
				if String(r["id"]) == "xuan_long_dan":
					var mats = r["mats"]
					_check(mats.size() == 2 and String(mats[0]["id"]) == "long_gu" and int(mats[0]["need"]) == 1
						and String(mats[1]["id"]) == "xuan_bing_shen" and int(mats[1]["need"]) == 2,
						"alchemy: 玄龙丹材料=龙骨×1+玄冰参×2（JSON 第 8 方）")
			_step = 1
		1:
			# ---------- ④ is_boss ×5 时序 ----------
			_next = _t + 0.3
			# a) 先 add_child 后置位（bootstrap 现状路径）：_ready 错过 → setter 补偿
			var e1 = ClassDB.instantiate("Enemy")
			e1.set("max_health", 10.0)
			current_scene.add_child(e1)
			e1.set("is_boss", true)
			_check(abs(float(e1.call("get_max_health")) - 50.0) < 0.01, "is_boss 后置位 ×5 补偿生效（10→50）")
			# 显式 HP 覆盖优先（bootstrap 先 set is_boss 再 set max_health 的写法不破）
			e1.set("max_health", 150.0); e1.set("current_health", 150.0)
			_check(abs(float(e1.call("get_max_health")) - 150.0) < 0.01, "显式 HP 覆盖 ×5（150 保持）")
			e1.queue_free()
			# b) 先置位再 add_child（.tscn 直摆路径）：_ready ×5，不多不少
			var e2 = ClassDB.instantiate("Enemy")
			e2.set("is_boss", true)
			e2.set("max_health", 10.0)
			current_scene.add_child(e2)
			_check(abs(float(e2.call("get_max_health")) - 50.0) < 0.01, "is_boss 前置位 _ready ×5（10→50，不重复）")
			e2.queue_free()
			_step = 2
		2:
			# ---------- ① 三灾元素结算口径 ----------
			_next = _t + 0.3
			var p = _player()
			var maxh = float(p.call("get_max_health"))
			_check(_equip("protect_robe"), "装备护身法袍（+3 防）")
			var defb = float(p.call("get_equip_bonus_defense"))
			var cult = p.call("get_cultivation")
			var defmult = float(cult.call("get_defense_multiplier"))
			# 物理：防御平减
			p.call("set_current_health", maxh)
			p.call("take_damage", 50.0, null)
			var loss_phys = maxh - float(p.call("get_current_health"))
			_check(abs(loss_phys - maxf(50.0 - defb * defmult, 1.0)) < 0.6,
				"物理结算被防御减免（50→%.1f）" % loss_phys)
			# 元素（雷=ELEM_LEI 6）：防御不可减免
			p.call("set_current_health", maxh)
			p.call("take_damage_typed", 50.0, 2, 6, null)
			var loss_lei = maxh - float(p.call("get_current_health"))
			_check(abs(loss_lei - 50.0) < 0.6, "雷元素结算无视防御（50→%.1f）" % loss_lei)
			# 菩提心法全元素抗性 +10%：雷/火/风 三天灾同口径
			p.call("get_skills").call("learn", "pu_ti_xin_fa")
			for elem in [[6, "雷LEI"], [4, "火HUO"], [7, "风FENG"]]:
				p.call("set_current_health", maxh)
				p.call("take_damage_typed", 50.0, 2, elem[0], null)
				var loss = maxh - float(p.call("get_current_health"))
				_check(abs(loss - 45.0) < 0.6, "菩提心法减免%s天劫（50→%.1f）" % [elem[1], loss])
			p.call("set_current_health", maxh)
			_step = 3
		3:
			# ---------- ② 普攻吃装备加成 ----------
			_next = _t + 0.5
			var p = _player()
			# 清场：出生区追过来的杂兵杀掉（无 source 不加修为），防干扰计量
			for e in get_nodes_in_group("enemies"):
				if not bool(e.get("is_boss")) and e.position.x < 400 and not e.is_queued_for_deletion():
					e.call("take_damage", 999999.0, null)
			p.position = Vector2(480, 200)
			p.set("velocity", Vector2.ZERO)
			p.call("set_current_health", float(p.call("get_max_health")))
			Input.action_press("right") # 先定面向（下一帧释放），攻击只向右挥
			_step = 35
		35:
			_next = _t + 0.3
			Input.action_release("right")
			var p = _player()
			p.position = Vector2(480, 200)
			p.set("velocity", Vector2.ZERO)
			if _enemy == null:
				_enemy = _make_dummy(p.position + Vector2(24, 0))
			else:
				_enemy.position = p.position + Vector2(24, 0)
			_enemy.set("current_health", 100000.0)
			_eff1 = float(p.call("get_effective_attack"))
			_hp_mark = float(_enemy.call("get_current_health"))
			Input.action_press("attack")
			_step = 4
		4:
			_next = _t + 0.1
			Input.action_release("attack")
			_step = 5
		5:
			# 命中窗口过后量血（combo 第一步 mult=1.0）
			_next = _t + 1.6 # >1.2s 连击窗，下一步攻击仍是第一段
			_loss1 = _hp_mark - float(_enemy.call("get_current_health"))
			_check(_near_combo_mult(_loss1, _eff1), "普攻无装备=面板×连段倍率（%.1f, eff %.1f）" % [_loss1, _eff1])
			_check(_equip("iron_sword"), "装备铁剑（+5 攻）")
			var p = _player()
			p.position = Vector2(480, 200)
			p.set("velocity", Vector2.ZERO)
			p.call("set_current_health", float(p.call("get_max_health")))
			_enemy.position = p.position + Vector2(24, 0)
			_enemy.set("current_health", 100000.0)
			_hp_mark = float(_enemy.call("get_current_health"))
			Input.action_press("attack")
			_step = 6
		6:
			_next = _t + 0.1
			Input.action_release("attack")
			_step = 7
		7:
			_next = _t + 0.3
			var eff2 = float(_player().call("get_effective_attack"))
			var loss2 = _hp_mark - float(_enemy.call("get_current_health"))
			print("[TEST] 普攻含铁剑: loss=%.1f eff=%.1f (无装备 %.1f)" % [loss2, eff2, _loss1])
			_check(_near_combo_mult(loss2, eff2), "普攻吃装备攻击加成（%.1f = eff %.1f ×连段）" % [loss2, eff2])
			_check(loss2 > _eff1 * 2.0 + 0.5, "超过无装备终结技上限（%.1f > %.1f）" % [loss2, _eff1 * 2.0])
			_step = 8
		8:
			# ---------- ⑤ boss 掉落走 boss 表 ----------
			_next = _t + 0.6
			var boss = null
			for e in get_nodes_in_group("enemies"):
				if bool(e.get("is_boss")) and not e.is_queued_for_deletion():
					boss = e
					break
			_check(boss != null, "找到 boss（赤瞳魔狼）")
			_baseline = _snap_pickups()
			boss.call("take_damage", 999999.0, _player())
			_step = 9
		9:
			_next = _t + 0.3
			var now = _snap_pickups()
			var new_ids = []
			for k in now:
				if not _baseline.has(k):
					new_ids.append(now[k])
			print("[TEST] boss drops: ", new_ids)
			_check(new_ids.has("qian_nian_ling_zhi"), "boss 掉落走 boss 表（千年灵芝保底）")
			_check(new_ids.size() >= 3, "boss 掉落数量 ≥3（实际 %d）" % new_ids.size())
			_step = 10
		10:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
