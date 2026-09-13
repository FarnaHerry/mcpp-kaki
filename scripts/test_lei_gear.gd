# 雷系配套补全 harness（雷纹佩·雷抗饰品 + 雷弹染色 + 天界投放点）：
# ① items.json 雷纹佩定义加载（雷纹佩/地品/desc 非空）
# ② 未装备时雷兽雷弹满额 28（atk14×2.0）
# ③ 装备雷纹佩（雷抗20%）后雷弹 28→22.4
# ④ 雷兽雷弹弹体染色：ProjVisual 亮蓝紫（非默认橙红）
# ⑤ 天界飞檐秘藏投放点：雷纹佩 ItemPickup 存在且可拾取入包
# 注：纯观察 harness 无按键输入；逐帧扫描投射物（弹 0.6s 内即中玩家，按步采样会漏）
extends SceneTree

const WC = preload("res://scripts/world_common.gd")

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0

var _hits_pre: Array = []
var _hits_post: Array = []
var _equipped := false
var _proj_color: Color = Color(-1, -1, -1) # 首颗雷弹弹体色
var _count_before := 0

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
	if _equipped:
		_hits_post.append(float(amount))
	else:
		_hits_pre.append(float(amount))

# 逐帧扫描：记录首颗雷兽雷弹的弹体色（ProjVisual Polygon2D）
func _scan_proj_color():
	if _proj_color.r >= 0.0:
		return
	for proj in current_scene.find_children("*", "Projectile", true, false):
		var src = proj.call("get_source")
		if src == null or not is_instance_valid(src):
			continue
		if String(src.name) != "T_LeiShou":
			continue
		var vis = proj.find_child("ProjVisual", true, false)
		if vis:
			_proj_color = vis.color

func _process(delta) -> bool:
	_scan_proj_color()
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
			_p().position = Vector2(5150, 200) # 谷底长草地带，邻域无其他敌
			_top_up_hp()
			var bus = root.find_child("SignalBus", true, false)
			bus.connect("damage_dealt", _on_dmg)
			_step = 1
		1:
			# ---- ① 雷纹佩定义加载 ----
			var db = root.find_child("ItemDatabase", true, false)
			var info: Dictionary = db.call("get_item_info", "lei_wen_pei")
			_check(not info.is_empty(), "雷纹佩定义已加载")
			if not info.is_empty():
				_check(String(info.get("name", "")) == "雷纹佩", "雷纹佩中文名: %s" % String(info.get("name", "")))
				_check(String(info.get("description", "")).length() >= 4, "雷纹佩有说明: %s" % String(info.get("description", "")))
				_check(int(info.get("type", -1)) == 3, "雷纹佩为装备（type 3）")
				_check(int(info.get("grade", -1)) == 2, "雷纹佩为地品（grade 2）")
			# ---- 放雷兽（玩家右侧 150：preferred 180 窗口内，直接开射）----
			var ls = WC.spawn_enemy_by_id(current_scene, Vector2(5300, 200), "lei_shou", "T_LeiShou")
			_check(ls != null and String(ls.get("proj_element")) == "lei", "雷兽生成并承接 proj_element=lei")
			_step = 2
		2, 3, 4, 5, 6, 7, 8, 9, 10, 11:
			# 雷兽观察窗 ~3s（cd 1.2s → ≥2 发）
			_top_up_hp()
			_step += 1
		12:
			# ---- ② 未装备满额断言 ----
			_check(_hits_pre.size() >= 1, "雷兽雷弹命中玩家（%d 次）" % _hits_pre.size())
			if _hits_pre.size() >= 1:
				_check(abs(_hits_pre[0] - 28.0) < 0.05, "未装备满额 28（实际 %.2f）" % _hits_pre[0])
			# ---- 装备雷纹佩（背包找槽 → equip_item）----
			var inv = _p().call("get_inventory")
			_check(bool(inv.call("add_item", "lei_wen_pei", 1)), "雷纹佩入包")
			var slot := -1
			for i in range(int(inv.call("get_capacity"))):
				var s: Dictionary = inv.call("get_slot", i)
				if not s.is_empty() and String(s.get("id", "")) == "lei_wen_pei":
					slot = i
					break
			_check(slot >= 0, "背包中找到雷纹佩槽位（%d）" % slot)
			if slot >= 0:
				_check(bool(_p().call("equip_item", slot)), "装备雷纹佩（饰品槽）")
				_check(String(_p().call("get_equipment_in_slot", 2)) == "lei_wen_pei", "饰品槽 2 = 雷纹佩")
			_equipped = true
			_step = 13
		13, 14, 15, 16, 17, 18, 19, 20:
			# 装备后观察窗 ~2.4s
			_top_up_hp()
			_step += 1
		21:
			# ---- ③ 雷抗 20% 减免断言 ----
			_check(_hits_post.size() >= 1, "装备后雷弹再次命中（%d 次）" % _hits_post.size())
			if _hits_post.size() >= 1 and _hits_pre.size() >= 1:
				_check(_hits_post[0] < _hits_pre[0] - 2.0, "装备后伤害低于未装备（%.2f < %.2f）" % [_hits_post[0], _hits_pre[0]])
				_check(abs(_hits_post[0] - 22.4) < 0.1, "雷抗 20%% 减免口径 28→22.4（实际 %.2f）" % _hits_post[0])
			# ---- ④ 雷弹染色断言（亮蓝紫，非默认橙红）----
			_check(_proj_color.r >= 0.0, "已扫描到雷兽雷弹弹体")
			if _proj_color.r >= 0.0:
				_check(_proj_color.b > 0.9 and _proj_color.r > 0.4 and _proj_color.r < 0.9 and _proj_color.g < 0.7,
						"雷弹染色亮蓝紫（%.2f, %.2f, %.2f）" % [_proj_color.r, _proj_color.g, _proj_color.b])
			# 收雷兽
			for proj in current_scene.find_children("*", "Projectile", true, false):
				var src = proj.call("get_source")
				if src != null and is_instance_valid(src) and String(src.name) == "T_LeiShou":
					proj.free()
			var ls = current_scene.find_child("T_LeiShou", true, false)
			if ls:
				ls.queue_free()
			_step = 22
		22:
			# ---- ⑤ 天界投放点：真仙登天 ----
			_p().call("get_cultivation").call("set_realm", 10)
			var cm = root.find_child("ContinentManager", true, false)
			_check(bool(cm.call("travel_to_direct", "tianjie")), "travel 天界（真仙腾云直达）")
			_next = _t + 1.5
			_step = 23
		23:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("tianjie.tscn"), "到达天界: " + sc)
			# 找飞檐秘藏雷纹佩拾取点
			var target = null
			for pk in current_scene.find_children("*", "ItemPickup", true, false):
				if String(pk.get("item_id")) == "lei_wen_pei":
					target = pk
					break
			_check(target != null, "天界存在雷纹佩投放点")
			if target:
				_check(target.position.distance_to(Vector2(2378, 52)) < 4.0,
						"投放点在凌霄殿飞檐秘藏（%s）" % str(target.position))
				_count_before = int(_p().call("get_inventory").call("get_item_count", "lei_wen_pei"))
				_p().global_position = target.position # 接触即拾取
			_next = _t + 0.8
			_step = 24
		24:
			var cnt = int(_p().call("get_inventory").call("get_item_count", "lei_wen_pei"))
			_check(cnt == _count_before + 1, "雷纹佩拾取入包（%d → %d）" % [_count_before, cnt])
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
