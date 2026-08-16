# 荒古冰墓秘境测试：
# ①玄冰高原入口 Portal + 地标 ②↑ 进秘境（Portal 房间模式）
# ③敌情：冰尸×3(realm8) / 寒螭×2(飞行 realm8) / 精英冰尸(tier1 xun_jie) / 万年冰魄 Boss(realm9)
# ④IceZone 墓道打滑 + ColdZone 冰穹厅极寒（减速+冰伤 dot）
# ⑤灌死 Boss → 必掉玄冰髓 xuan_bing_sui ⑥↑ 出秘境回玄冰高原入口旁
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _held: Array = []

func _initialize():
	var scene = load("res://scenes/continents/beijulu.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] beijulu scene loaded")

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

# 按住一帧再释放（同帧 press+release 对 action 轮询不可靠）
func _hold(action: String):
	Input.action_press(action)
	_held.append(action)

func _release_all():
	for a in _held:
		Input.action_release(a)
	_held.clear()

func _player():
	return root.find_child("Player", true, false)

func _room():
	return root.find_child("HuangGuBingMu", true, false)

func _has_label(text: String) -> bool:
	for l in root.find_children("*", "Label", true, false):
		if text in String(l.text):
			return true
	return false

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1
	if _step > 40:
		print("[TEST] hard cap")
		if _fail == 0:
			print("[TEST] ALL PASS")
		else:
			print("[TEST] ", _fail, " FAILURES")
		return true

	match _step:
		1:
			_next = _t + 1.0 # 等北俱芦洲装配
		2:
			# ① 入口 Portal（target_scene 指向荒古冰墓）+ 地标
			var found = false
			for p in current_scene.find_children("*", "Portal", true, false):
				if String(p.get("target_scene")).ends_with("huang_gu_bing_mu.tscn"):
					found = true
			_check(found, "玄冰高原有荒古冰墓入口 Portal")
			_check(_has_label("荒古冰墓"), "荒古冰墓地标存在")
			# 渡劫境界（对应北俱芦洲）+ 满血，走到入口 x=2280
			var pl = _player()
			pl.call("get_cultivation").call("set_realm", 9)
			pl.call("set_current_health", pl.call("get_max_health"))
			pl.global_position = Vector2(2280, 220)
			_next = _t + 0.5
		3:
			_hold("up")
		4:
			_release_all()
			_next = _t + 1.2
		5:
			# ②③ 已进秘境：敌情断言
			var room = _room()
			_check(room != null, "荒古冰墓房间已挂载")
			if room == null:
				return true
			var pl = _player()
			_check(pl.get_parent() == room, "玩家进入荒古冰墓（父节点=房间）")
			for i in range(3):
				var bs = room.get_node_or_null("BingShi%d" % i)
				_check(bs != null, "冰尸 BingShi%d 在场" % i)
				if bs:
					_check(String(bs.get("enemy_id")) == "bing_shi", "BingShi%d id=bing_shi" % i)
					_check(int(bs.get("realm")) == 8, "BingShi%d realm=8" % i)
			for i in range(2):
				var hc = room.get_node_or_null("HanChi%d" % i)
				_check(hc != null, "寒螭 HanChi%d 在场" % i)
				if hc:
					_check(String(hc.get("enemy_id")) == "han_chi", "HanChi%d id=han_chi" % i)
					_check(bool(hc.get("is_flying")), "HanChi%d 飞行" % i)
					_check(int(hc.get("realm")) == 8, "HanChi%d realm=8" % i)
			var jy = room.get_node_or_null("JingYingBingShi")
			_check(jy != null, "精英冰尸在场")
			if jy:
				_check(int(jy.get("elite_tier")) == 1, "精英冰尸 elite_tier=1")
				_check(String(jy.get("affix_id")) == "xun_jie", "精英冰尸词缀=迅捷(xun_jie)")
			var boss = room.get_node_or_null("Boss_WanNianBingPo")
			_check(boss != null, "万年冰魄 Boss 在场")
			if boss:
				_check(String(boss.get("enemy_id")) == "wan_nian_bing_po", "Boss id=wan_nian_bing_po")
				_check(bool(boss.get("is_boss")), "万年冰魄 is_boss")
				_check(int(boss.get("realm")) == 9, "万年冰魄 realm=9")
				_check(float(boss.get("max_health")) >= 2000.0, "万年冰魄 Boss ×5 血量 (%.0f)" % float(boss.get("max_health")))
			# ④ 机制：墓道 IceZone + 冰穹厅 ColdZone 存在
			_check(room.get_node_or_null("IceZone_MuDao") != null, "墓道 IceZone 存在")
			_check(room.get_node_or_null("ColdZone_BingQiong") != null, "冰穹厅 ColdZone 存在")
			# 出生在墓道冰面（x48 在 IceZone 内）→ 打滑
			_check(bool(pl.call("is_slippery")), "墓道冰面打滑标记")
			# 进冰穹厅极寒区
			pl.call("set_current_health", pl.call("get_max_health"))
			pl.position = Vector2(225, 210)
			_next = _t + 1.2
		6:
			var pl = _player()
			_check(bool(pl.call("is_chilled")), "冰穹厅极寒减速标记")
			_check(float(pl.call("get_current_health")) < float(pl.call("get_max_health")), "极寒冰伤扣血 (%.0f/%.0f)" % [pl.call("get_current_health"), pl.call("get_max_health")])
			# 出极寒区到墓心 → 恢复
			pl.call("set_current_health", pl.call("get_max_health"))
			pl.position = Vector2(330, 205)
			_next = _t + 0.6
		7:
			var pl = _player()
			_check(not bool(pl.call("is_chilled")), "离开极寒区恢复")
			_check(not bool(pl.call("is_slippery")), "离开冰面恢复")
			# 先远离墓心再灌死 Boss（纳戒磁吸 realm9 范围 ~150px，贴近杀会被直接吸走拾取）
			pl.position = Vector2(120, 210)
			var boss = _room().get_node_or_null("Boss_WanNianBingPo")
			if boss:
				boss.call("take_damage", 999999.0, null)
			_next = _t + 0.8
		8:
			var room = _room()
			_check(room.get_node_or_null("Boss_WanNianBingPo") == null, "万年冰魄已击杀")
			var found_drop = false
			for pk in room.find_children("*", "ItemPickup", true, false):
				if String(pk.get("item_id")) == "xuan_bing_sui":
					found_drop = true
			_check(found_drop, "Boss 必掉玄冰髓 xuan_bing_sui")
			# 走过去拾取
			_player().position = Vector2(350, 208)
			_next = _t + 0.8
		9:
			var inv = _player().call("get_inventory")
			_check(int(inv.call("get_item_count", "xuan_bing_sui")) >= 1, "拾得玄冰髓")
			# 清场（寒螭全图索敌追击+受击击退会把玩家撞出出口判定区，干扰出秘境断言）
			for e in _room().find_children("*", "Enemy", true, false):
				e.call("take_damage", 999999.0, null)
			# ⑥ 出秘境（出口在房间底中央 x200）
			_player().position = Vector2(200, 210)
			_next = _t + 0.6
		10:
			_hold("up")
		11:
			_release_all()
			_next = _t + 1.2
		12:
			var pl = _player()
			_check(pl.get_parent() == current_scene, "出秘境回北俱芦洲")
			_check(abs(pl.global_position.x - 2280.0) < 60.0, "回到玄冰高原入口旁 (x=%.0f)" % pl.global_position.x)
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
