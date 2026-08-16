# 步骤D harness: ①采集点存在 ②靠近提示[X] ③X采集入包+枯萎 ④采集后提示清除
#      ⑤Boss 掉落千年灵芝保底 ⑥采集喂练气
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _herb = null
var _qty0 := 0

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

func _count_item(inv, id) -> int:
	for i in range(inv.call("get_capacity")):
		var sd = inv.call("get_slot", i)
		if not sd.is_empty() and String(sd["id"]) == id:
			return int(sd["quantity"])
	return 0

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			_herb = root.find_child("Herb_zhi_xue_cao", true, false)
			_check(_herb != null, "herb node spawned (Herb_zhi_xue_cao)")
			_check(String(_herb.call("get_herb_id")) == "zhi_xue_cao", "herb id set")
			_check(not _herb.call("is_harvested"), "herb not harvested initially")
			# 玩家走到采集点
			var p = root.find_child("Player", true, false)
			p.global_position = _herb.global_position
		2:
			_next = _t + 0.4
			# 提示已发出（HUD 交互提示文本）
			var hud = root.find_child("GameHUD", true, false)
			var prompt = hud.find_child("PromptLabel", true, false)
			if prompt == null:
				# 找任意显示中的交互提示 Label
				for c in hud.get_children():
					if c is Label and c.visible and "[X]" in c.text:
						prompt = c
			_check(prompt != null and "[X]" in prompt.text, "interaction prompt shows [X]")
			# 按 X 采集
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			_qty0 = _count_item(inv, "zhi_xue_cao")
			Input.action_press("interact")
		3:
			_next = _t + 0.3
			Input.action_release("interact")
			var p = root.find_child("Player", true, false)
			var inv = p.call("get_inventory")
			_check(_herb.call("is_harvested"), "herb harvested after X")
			_check(_count_item(inv, "zhi_xue_cao") > _qty0, "herb added to inventory")
		4:
			_next = _t + 0.3
			# Boss 掉落千年灵芝保底：直接调 _do_spawn_drops（v2 签名：pos, drop_table, is_boss, is_ranged, is_flying, realm）
			var ds = root.find_child("DropSystem", true, false)
			ds.call("_do_spawn_drops", Vector2(-200, 200), "", true, false, false, 0)
		5:
			_next = _t + 0.3
			var found = false
			for c in current_scene.get_children():
				if c.has_method("get_item_id") and String(c.call("get_item_id")) == "qian_nian_ling_zhi":
					found = true
			_check(found, "boss drop includes qian_nian_ling_zhi (chance 1.0)")
		6:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
