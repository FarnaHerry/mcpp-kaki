# 龙宫藏珍阁秘境测试（东海龙宫深处二层，入口在龙宫内镇守将身后 x=438）：
# ①物品注册（定海珠 天品饰品 水抗+25%）②金丹飞至龙宫口 ③↑ 入龙宫（既有内容不受影响）
# ④↑ 入藏珍阁：敌情断言（巡珍鲛卫×2/珠母精×2 远程水弹、精英·厚甲镇阁巨鼋、龙王三太子 Boss realm8 ×5 血）
# ⑤压制修为 realm7（回龙宫还原 6）⑥灌死 Boss → 命名表 cang_zhen_ge 必掉定海珠
# ⑦房内拾取定海珠 ⑧↑ 出阁回龙宫（压制还原 6）⑨出龙宫回主场景（压制还原 -1）
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

func _cult():
	return _player().call("get_cultivation")

# 全场景扫 ItemPickup 节点，返回第一个 item_id 匹配的
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
			# ①物品注册：定海珠（天品饰品 水抗+25%）
			var db = root.find_child("ItemDatabase", true, false)
			var info = db.call("get_item_info", "ding_hai_zhu")
			_check(not info.is_empty(), "物品注册: ding_hai_zhu")
			_check(String(info.get("name", "")) == "定海珠", "定海珠命名")
			_check(int(info.get("type", -1)) == 3, "定海珠=装备")
			_check(int(info.get("grade", -1)) == 3, "定海珠=天品")
			_check(String(info.get("description", "")).length() >= 4, "定海珠有说明")
			# ②金丹（飞行/龙宫门槛富余）→ 龙宫门口
			_cult().call("set_realm", 3)
			_player().global_position = Vector2(8600, 210)
			_next = _t + 0.6
			_step = 1
		1:
			# ③↑ 入龙宫（按住一帧再释放）
			Input.action_press("up")
			_next = _t + 0.2
			_step = 2
		2:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 3
		3:
			# 龙宫挂载 + 既有内容未受影响
			var room = current_scene.get_node_or_null("LongGong")
			_check(room != null, "东海龙宫已挂载")
			_check(room != null and room.find_child("XiaBing0", true, false) != null, "既有虾兵未受影响")
			_check(room != null and room.find_child("LongGongZhenShou", true, false) != null, "既有镇守将未受影响")
			_check(room != null and room.get_node_or_null("CangZhenGePortal") != null, "藏珍阁入口 Portal 存在")
			var cp = room.get_node_or_null("CangZhenGePortal")
			if cp:
				_check(String(cp.get("prompt_text")) == "[↑] 入藏珍阁", "入口提示: " + String(cp.get("prompt_text")))
				_check(cp.position.x >= 430.0, "入口在镇守将身后（x=%.0f）" % cp.position.x)
			# 走到藏珍阁门口
			_player().global_position = Vector2(438, 210)
			_next = _t + 0.6
			_step = 4
		4:
			# ④↑ 入藏珍阁
			Input.action_press("up")
			_next = _t + 0.2
			_step = 5
		5:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 6
		6:
			# ⑤藏珍阁挂载 + 敌情
			var room = current_scene.get_node_or_null("CangZhenGe")
			_check(room != null, "藏珍阁已挂载")
			if room:
				# 玩家重挂载：Portal 房间模式下玩家父节点应为阁（或阁的父链含阁）
				var par = _player().get_parent()
				var in_room := false
				while par:
					if par == room:
						in_room = true
						break
					par = par.get_parent()
				_check(in_room, "玩家已重挂载进藏珍阁")
				_check(int(_player().get("suppressed_realm")) == 7, "秘境压制修为 realm=7")
				for i in range(2):
					var jw = room.find_child("XunZhenJiaoWei%d" % i, true, false)
					_check(jw != null and int(jw.call("get_realm")) == 7, "巡珍鲛卫%d 在场 realm7" % i)
				for i in range(2):
					var zm = room.find_child("ZhuMuJing%d" % i, true, false)
					_check(zm != null and int(zm.call("get_realm")) == 7, "珠母精%d 在场 realm7" % i)
					_check(zm != null and bool(zm.get("is_ranged")), "珠母精%d 远程（泡珠弹）" % i)
					_check(zm != null and String(zm.get("proj_element")) == "shui", "珠母精%d 水元素弹" % i)
				var elite = room.find_child("ZhenGeJuYuanElite", true, false)
				_check(elite != null, "精英镇阁巨鼋守贝台")
				if elite:
					_check(int(elite.get("elite_tier")) == 1, "精英化 tier=1")
					_check(String(elite.get("affix_id")) == "hou_jia", "厚甲词缀 hou_jia")
				var boss = room.find_child("LongWangSanTaiZi", true, false)
				_check(boss != null, "龙王三太子 Boss 在场")
				if boss:
					_check(bool(boss.get("is_boss")), "龙王三太子 is_boss")
					_check(int(boss.call("get_realm")) == 8, "龙王三太子 realm8")
					_check(float(boss.call("get_max_health")) > 5000.0, "Boss ×5 血量（%.0f > 5000）" % float(boss.call("get_max_health")))
					_check(String(boss.call("get_drop_table")) == "cang_zhen_ge", "龙王三太子掉落表=cang_zhen_ge")
					_check(int(boss.get("boss_phase")) == 1, "Boss 初始一阶段")
			_next = _t + 0.3
			_step = 7
		7:
			# ⑥阶段机制 + 灌死 Boss → 命名表必掉定海珠
			var boss = root.find_child("LongWangSanTaiZi", true, false)
			_check(boss != null and float(boss.call("get_current_health")) > 0, "龙王三太子存活待击")
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			if boss:
				boss.call("take_damage", float(boss.call("get_max_health")) * 0.55, p)
			_next = _t + 0.4
			_step = 8
		8:
			var boss = root.find_child("LongWangSanTaiZi", true, false)
			_check(boss != null and int(boss.get("boss_phase")) == 2, "半血激怒转二阶段")
			var p = _player()
			p.call("set_current_health", p.call("get_max_health"))
			if boss:
				boss.call("take_damage", 999999.0, p)
			_next = _t + 0.6
			_step = 9
		9:
			var boss2 = root.find_child("LongWangSanTaiZi", true, false)
			_check(boss2 == null or float(boss2.call("get_current_health")) <= 0, "龙王三太子已被击杀")
			# ⑦房内拾取定海珠（命名表 cang_zhen_ge 必掉）
			var pk = _find_pickup("ding_hai_zhu")
			_check(pk != null, "定海珠秘藏掉落生成（命名表 cang_zhen_ge 必掉）")
			if pk:
				_player().position = pk.position
			_next = _t + 0.5
			_step = 10
		10:
			var inv0 = _player().call("get_inventory")
			_check(int(inv0.call("get_item_count", "ding_hai_zhu")) >= 1, "房内拾得定海珠入包（天品饰品）")
			# 清场防归途被残余敌兵围殴致死
			var p10 = _player()
			p10.call("set_current_health", p10.call("get_max_health"))
			for en in ["XunZhenJiaoWei0", "XunZhenJiaoWei1", "ZhuMuJing0", "ZhuMuJing1", "ZhenGeJuYuanElite"]:
				var e = root.find_child(en, true, false)
				if e and float(e.call("get_current_health")) > 0:
					e.call("take_damage", 999999.0, p10)
			# ⑧↑ 出阁回龙宫（ExitPortal 在 room_bounds 底中 x=240）
			_player().position = Vector2(240, 220)
			_next = _t + 0.6
			_step = 11
		11:
			Input.action_press("up")
			_next = _t + 0.2
			_step = 12
		12:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 13
		13:
			_check(current_scene.get_node_or_null("CangZhenGe") == null, "藏珍阁已卸载")
			_check(current_scene.get_node_or_null("LongGong") != null, "回到龙宫（非主场景）")
			_check(int(_player().get("suppressed_realm")) == 6, "压制修为还原为龙宫 realm=6")
			# ⑨出龙宫回主场景（回龙宫落点在镇守将旁，先补满血再走）
			var p13 = _player()
			p13.call("set_current_health", p13.call("get_max_health"))
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 14
		14:
			Input.action_press("up")
			_next = _t + 0.2
			_step = 15
		15:
			Input.action_release("up")
			_next = _t + 0.8
			_step = 16
		16:
			_check(current_scene.get_node_or_null("LongGong") == null, "龙宫已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到主场景")
			_check(int(_player().get("suppressed_realm")) == -1, "压制修为已还原")
			var pos = _player().global_position
			_check(abs(pos.x - 8600.0) < 40.0, "出龙宫回到门口位置（x=%.0f）" % pos.x)
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
