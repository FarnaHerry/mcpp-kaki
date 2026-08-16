# 大雁塔地宫秘境测试（南赡部洲·长安）：
# ①炼虚旅行到南赡部洲 ②入口 Portal（target_scene/prompt）③↑ 进地宫
# ④敌人配置（塔妖×3/烛幽灵×2 realm6、精英塔妖 狂暴词缀、守塔金刚 Boss realm7 ×5 血）
# ⑤灌死 Boss → 命名表 da_yan_ta 必掉舍利子 ⑥↑ 出地宫回入口旁
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

func _find(s: String):
	return root.find_child(s, true, false)

func _find_dyt_portal():
	for p in root.find_children("*", "Portal", true, false):
		if String(p.get("target_scene")).ends_with("da_yan_ta.tscn"):
			return p
	return null

# 全场景扫 ItemPickup 节点，返回第一个 item_id 匹配的（掉落可能散落后未被拾取）
func _find_pickup(item_id: String):
	var stack := [current_scene]
	while not stack.is_empty():
		var n = stack.pop_back()
		for c in n.get_children():
			stack.push_back(c)
		var v = n.get("item_id")
		if (typeof(v) == TYPE_STRING_NAME or typeof(v) == TYPE_STRING) and String(v) == item_id:
			return n
	return null

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	paused = false

	match _step:
		0:
			# ①炼虚（南赡部洲门槛）→ 直达长安
			_player().call("get_cultivation").call("set_realm", 6)
			var cm = _find("ContinentManager")
			_check(cm != null, "ContinentManager 存在")
			_check(bool(cm.call("travel_to_direct", "nanzhanbu")), "travel_to_direct 南赡部洲")
			_next = _t + 1.5
			_step = 1
		1:
			# ②入口 Portal
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("nanzhanbu.tscn"), "到达南赡部洲: " + sc)
			var portal = _find_dyt_portal()
			_check(portal != null, "大雁塔地宫入口 Portal 存在")
			if portal:
				_check(String(portal.get("prompt_text")) == "[↑] 进入大雁塔地宫", "入口提示: " + String(portal.get("prompt_text")))
			_check(_find("ShopKeeper") != null, "既有坊市内容未受影响")
			_player().global_position = Vector2(1650, 210)
			_next = _t + 0.6
			_step = 2
		2:
			# ③↑ 进地宫（按住一帧再释放，action 轮询才可靠）
			Input.action_press("up")
			_next = _t + 0.2
			_step = 3
		3:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 4
		4:
			# ④地宫挂载 + 敌人配置
			var room = current_scene.get_node_or_null("DaYanTa")
			_check(room != null, "大雁塔地宫已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进宫")
			if room:
				for i in range(3):
					var ty = room.find_child("TaYao%d" % i, true, false)
					_check(ty != null and int(ty.call("get_realm")) == 6, "塔妖%d 在场 realm6" % i)
				for i in range(2):
					var zy = room.find_child("ZhuYouLing%d" % i, true, false)
					_check(zy != null and int(zy.call("get_realm")) == 6, "烛幽灵%d 在场 realm6" % i)
					_check(zy != null and bool(zy.get("is_flying")), "烛幽灵%d 飞行" % i)
				var elite = room.find_child("TaYaoElite", true, false)
				_check(elite != null, "精英塔妖守佛龛")
				if elite:
					_check(int(elite.get("elite_tier")) == 1, "精英化 tier=1")
					_check(String(elite.get("affix_id")) == "kuang_bao", "狂暴词缀 kuang_bao")
					_check(float(elite.call("get_max_health")) > 150.0, "精英血量强化（%.0f > 150）" % float(elite.call("get_max_health")))
				var boss = room.find_child("ShouTaJinGang", true, false)
				_check(boss != null, "守塔金刚 Boss 在场")
				if boss:
					_check(bool(boss.get("is_boss")), "守塔金刚 is_boss")
					_check(int(boss.call("get_realm")) == 7, "守塔金刚 realm7")
					_check(abs(float(boss.call("get_max_health")) - 900.0) < 0.5, "Boss ×5 血量 900（实际 %.0f）" % float(boss.call("get_max_health")))
			_next = _t + 0.3
			_step = 5
		5:
			# ⑤灌死 Boss → 命名表 da_yan_ta 必掉舍利子（玩家回血防被围殴致死）
			var boss = _find("ShouTaJinGang")
			_check(boss != null and float(boss.call("get_current_health")) > 0, "守塔金刚存活待击")
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			if boss:
				boss.call("take_damage", 999999.0, p)
			_next = _t + 0.6
			_step = 6
		6:
			var boss2 = _find("ShouTaJinGang")
			_check(boss2 == null or float(boss2.call("get_current_health")) <= 0, "守塔金刚已被击杀")
			# 命名表必掉舍利子：掉落物挂场景根（ItemPickup 设计跨父节点不拾取，宫内捡不到，
			# 先断言生成，出宫后玩家与掉落同父节点再贴身拾取验证入包）
			var pk = _find_pickup("she_li_zi")
			_check(pk != null, "舍利子秘藏掉落生成（命名表 da_yan_ta 必掉）")
			# ⑥出地宫
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 8
		8:
			Input.action_press("up")
			_next = _t + 0.2
			_step = 9
		9:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 10
		10:
			_check(current_scene.get_node_or_null("DaYanTa") == null, "地宫已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到南赡部洲")
			var pos = _player().global_position
			_check(abs(pos.x - 1650.0) < 40.0, "出宫回到大雁塔入口旁（x=%.0f）" % pos.x)
			# ⑦出宫后玩家与掉落同父节点：贴身拾取舍利子验证入包
			var pk3 = _find_pickup("she_li_zi")
			if pk3:
				_player().global_position = pk3.global_position
			_next = _t + 0.6
			_step = 11
		11:
			var inv4 = _player().call("get_inventory")
			_check(int(inv4.call("get_item_count", "she_li_zi")) >= 1, "拾得舍利子入包（地品饰品 防御+10）")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
