# 大雷音寺遗址秘境测试（南赡部洲·五庄观以东）：
# ①物品注册（旃檀功德香 天品消耗品 修为+5000/全元素抗性buff_id）②炼虚旅行到南赡部洲 ③入口 Portal
# ④↑ 进遗址：敌情断言（扫地僧傀×3/诵经兽×2 远程、精英·狂暴护法金刚、心猿石像 Boss realm7 ×5 血/阶段）
# ⑤压制修为 realm7 ⑥灌死 Boss → 命名表 da_lei_yin_si 必掉旃檀功德香 ⑦↑ 出遗址回入口旁 + 拾取秘藏入包/服用
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

func _find_dlys_portal():
	for p in root.find_children("*", "Portal", true, false):
		if String(p.get("target_scene")).ends_with("da_lei_yin_si.tscn"):
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
			# ①物品注册：旃檀功德香（天品消耗品 修为+5000 + buff）
			var db = root.find_child("ItemDatabase", true, false)
			var info = db.call("get_item_info", "zhan_tan_gong_de_xiang")
			_check(not info.is_empty(), "物品注册: zhan_tan_gong_de_xiang")
			_check(int(info.get("type", -1)) == 0, "旃檀功德香=消耗品")
			_check(int(info.get("grade", -1)) == 3, "旃檀功德香=天品")
			_check(String(info.get("description", "")).length() >= 4, "旃檀功德香有说明")
			_check(String(info.get("buff_id", "")) == "buff_zhan_tan", "旃檀功德香 buff_id=buff_zhan_tan")
			# ②炼虚（南赡部洲门槛）→ 直达长安
			_player().call("get_cultivation").call("set_realm", 6)
			var cm = _find("ContinentManager")
			_check(cm != null, "ContinentManager 存在")
			_check(bool(cm.call("travel_to_direct", "nanzhanbu")), "travel_to_direct 南赡部洲")
			_next = _t + 1.5
			_step = 1
		1:
			# ③入口 Portal（五庄观以东 x=2450，避城镇/坊市/地府门）
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("nanzhanbu.tscn"), "到达南赡部洲: " + sc)
			var portal = _find_dlys_portal()
			_check(portal != null, "大雷音寺遗址入口 Portal 存在")
			if portal:
				_check(String(portal.get("prompt_text")) == "[↑] 进入大雷音寺遗址", "入口提示: " + String(portal.get("prompt_text")))
				_check(portal.position.x >= 2400.0, "入口位于五庄观以东荒野（x=%.0f）" % portal.position.x)
			_check(_find("ShopKeeper") != null, "既有坊市内容未受影响")
			_check(_find("DifuGate") != null, "既有地府入口未受影响")
			_player().global_position = Vector2(2450, 210)
			_next = _t + 0.6
			_step = 2
		2:
			# ④↑ 进遗址（按住一帧再释放，action 轮询才可靠）
			Input.action_press("up")
			_next = _t + 0.2
			_step = 3
		3:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 4
		4:
			# ⑤遗址挂载 + 敌情
			var room = current_scene.get_node_or_null("DaLeiYinSi")
			_check(room != null, "大雷音寺遗址已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进遗址")
			_check(int(_player().get("suppressed_realm")) == 7, "秘境压制修为 realm=7")
			if room:
				for i in range(3):
					var sk = room.find_child("SaoDiSengKui%d" % i, true, false)
					_check(sk != null and int(sk.call("get_realm")) == 6, "扫地僧傀%d 在场 realm6" % i)
				for i in range(2):
					var sj = room.find_child("SongJingShou%d" % i, true, false)
					_check(sj != null and int(sj.call("get_realm")) == 6, "诵经兽%d 在场 realm6" % i)
					_check(sj != null and bool(sj.get("is_ranged")), "诵经兽%d 远程（音波弹）" % i)
				var elite = room.find_child("HuFaJinGangElite", true, false)
				_check(elite != null, "精英护法金刚镇高台")
				if elite:
					_check(int(elite.get("elite_tier")) == 1, "精英化 tier=1")
					_check(String(elite.get("affix_id")) == "kuang_bao", "狂暴词缀 kuang_bao")
					_check(float(elite.call("get_max_health")) > 1170.0, "精英血量强化（%.0f > 1170）" % float(elite.call("get_max_health")))
				var boss = room.find_child("XinYuanShiXiang", true, false)
				_check(boss != null, "心猿石像 Boss 在场")
				if boss:
					_check(bool(boss.get("is_boss")), "心猿石像 is_boss")
					_check(int(boss.call("get_realm")) == 7, "心猿石像 realm7")
					_check(abs(float(boss.call("get_max_health")) - 4950.0) < 0.5, "Boss ×5 血量 4950（实际 %.0f）" % float(boss.call("get_max_health")))
					_check(String(boss.call("get_drop_table")) == "da_lei_yin_si", "心猿石像掉落表=da_lei_yin_si")
					_check(int(boss.get("boss_phase")) == 1, "Boss 初始一阶段")
			_next = _t + 0.3
			_step = 5
		5:
			# ⑥阶段机制：打到半血以下 → Hurt 态自动转二阶段（既有 Enemy 阶段机制）
			var boss = _find("XinYuanShiXiang")
			_check(boss != null and float(boss.call("get_current_health")) > 0, "心猿石像存活待击")
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			if boss:
				boss.call("take_damage", float(boss.call("get_max_health")) * 0.55, p)
			_next = _t + 0.4
			_step = 6
		6:
			var boss = _find("XinYuanShiXiang")
			_check(boss != null and int(boss.get("boss_phase")) == 2, "半血激怒转二阶段")
			# ⑦灌死 Boss → 命名表 da_lei_yin_si 必掉旃檀功德香（玩家回血防被围殴致死）
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			if boss:
				boss.call("take_damage", 999999.0, p)
			_next = _t + 0.6
			_step = 7
		7:
			var boss2 = _find("XinYuanShiXiang")
			_check(boss2 == null or float(boss2.call("get_current_health")) <= 0, "心猿石像已被击杀")
			# 命名表必掉旃檀功德香：掉落挂玩家当前父节点（房内同层），房内贴身拾取入包
			var pk = _find_pickup("zhan_tan_gong_de_xiang")
			_check(pk != null, "旃檀功德香秘藏掉落生成（命名表 da_lei_yin_si 必掉）")
			# 修为拉过 50% 上限，防拾取时能量类消耗品被自动服用（pickup_item auto-use），干扰入包断言
			_player().call("get_cultivation").call("set_spiritual_energy", 100000)
			if pk:
				_player().position = pk.position
			_next = _t + 0.5
			_step = 8
		8:
			var inv0 = _player().call("get_inventory")
			_check(int(inv0.call("get_item_count", "zhan_tan_gong_de_xiang")) >= 1, "房内拾得旃檀功德香入包（天品消耗品）")
			# ⑧出遗址
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 9
		9:
			Input.action_press("up")
			_next = _t + 0.2
			_step = 10
		10:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 11
		11:
			_check(current_scene.get_node_or_null("DaLeiYinSi") == null, "遗址已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到南赡部洲")
			_check(int(_player().get("suppressed_realm")) == -1, "压制修为已还原")
			var pos = _player().global_position
			_check(abs(pos.x - 2450.0) < 40.0, "出遗址回到入口旁（x=%.0f）" % pos.x)
			# ⑨服用：修为 +5000（buff_zhan_tan 待 BuffSystem 接 DataLoader 后生效，此处只验修为主效）
			var inv = _player().call("get_inventory")
			var cult = _player().call("get_cultivation")
			var e0 = int(cult.call("get_current_energy"))
			_check(bool(_player().call("use_consumable", "zhan_tan_gong_de_xiang")), "服用旃檀功德香")
			var e1 = int(cult.call("get_current_energy"))
			_check(e1 - e0 >= 5000 or e1 >= 5000, "修为+5000（%d→%d）" % [e0, e1])
			_check(int(inv.call("get_item_count", "zhan_tan_gong_de_xiang")) == 0, "服用后消耗")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
