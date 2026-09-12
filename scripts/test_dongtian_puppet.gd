# 洞天 v5 傀儡 harness（灵石购买激活 + 驻守/随行双模式）：
# ① 傀儡 NPC 装配就位（灵田左端上空浮台 x=40），贴近提示 [X] 唤醒傀儡
# ② X 购买激活（下品×1500，CurrencySystem 四阶钱包扣款）→ 默认驻守：灵田生长 +50%
#    （聚灵草 60s → 40s，41s 拨快即成熟证明加速生效）
# ③ X 切换随行模式 → CloneAvatar 弱化快照实体（HP×40%/攻×40%）跟随部署（不占身外化身上限）
# ④ 傀儡战死 → 60s 冷却播报，清冷却/收回后可重新召出；O 出洞天傀儡随行到外界
# ⑤ 进 Portal 房间（古剑冢）自动收回（无冷却），出房间重新部署
# ⑥ 存档往返：激活/模式/灵石余额持久化，读档后按态重新部署
# 注：一帧只按一键；press 保持一帧再 release（同帧 press+release 对轮询不可靠）
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _pending_release := ""

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

func _press(action: String):
	Input.action_press(action)
	_pending_release = action

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _gm():
	return root.find_child("GameManager", true, false)

func _dt():
	return root.find_child("DongtianManager", true, false)

func _cur():
	return root.find_child("CurrencySystem", true, false)

func _dongtian_scene():
	return root.find_child("Dongtian", true, false)

func _kuilei():
	return root.find_child("Kuilei", true, false)

func _puppets() -> Array:
	return get_nodes_in_group("dongtian_puppet")

func _puppet():
	var g := _puppets()
	return g[0] if g.size() > 0 else null

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

func _finish() -> bool:
	if _fail == 0:
		print("[TEST] ALL PASS")
	else:
		print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	# 上一帧的按键保持到本帧再释放
	if _pending_release != "":
		Input.action_release(_pending_release)
		_pending_release = ""
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			for e in get_nodes_in_group("enemies"):
				e.queue_free()
			_cult().call("set_free_breakthrough", true)
			_cult().call("accumulate_energy", 100000000000)
			while int(_cult().call("get_realm_index")) < 6:
				_cult().call("attempt_breakthrough")
			_cult().call("set_free_breakthrough", false)
			_step = 55
		55:
			# 修为降到半管（避免封顶自动请求突破干扰）；备灵石 下品×2000
			var max_e := int(_cult().call("get_max_energy"))
			_cult().call("set_spiritual_energy", int(max_e * 0.5))
			_cur().call("add", 0, 2000)
			_dt().call("debug_suppress_invasion")
			_press("dongtian")
			_step = 1
		1:
			# ---- ① 装配 ----
			_check(_dt().call("is_inside") == true, "进入洞天")
			_check(_kuilei() != null, "傀儡 NPC 节点就位")
			_check(root.find_child("PuppetLedge", true, false) != null, "傀儡浮台就位")
			_check(bool(_dt().call("is_puppet_active")) == false, "初始傀儡未激活")
			_check(int(_dt().call("get_puppet_cost")) == 1500, "激活价格 下品×1500")
			_check(abs(float(_dt().call("get_puppet_growth_mult")) - 1.0) < 0.001, "未激活生长倍率 1.0")
			_player().global_position = Vector2(40, 120) # 从浮台上方落上台面（直接放台面高度会穿过单向板坠地）
			_next = _t + 0.5
			_step = 2
		2:
			_check(_hud_has("唤醒傀儡"), "贴近傀儡提示：[X] 唤醒傀儡")
			# 备料：聚灵草（生长 60s，驻守加速后 40s）
			_player().call("pickup_item", "ju_ling_cao", 2)
			_press("interact") # X 购买激活
			_step = 3
		3:
			# ---- ② 激活 → 驻守生长加速 ----
			_check(bool(_dt().call("is_puppet_active")) == true, "X 后傀儡激活")
			_check(int(_cur().call("get_total")) == 500, "扣款 下品×1500（余 %d）" % int(_cur().call("get_total")))
			_check(int(_dt().call("get_puppet_cost")) == 0, "已激活后价格归 0")
			_check(int(_dt().call("get_puppet_mode")) == 0, "初醒默认驻守模式")
			_check(abs(float(_dt().call("get_puppet_growth_mult")) - 1.5) < 0.001, "驻守生长倍率 1.5")
			var bub = _kuilei().get("_bubble")
			_check(bub != null and bub.visible, "激活后台词气泡显示")
			_check(bool(_dt().call("plant", 0)) == true, "地块 0 播种聚灵草")
			var p0: Dictionary = _dt().call("get_plot", 0)
			_check(int(p0.get("remaining", 0)) >= 39 and int(p0.get("remaining", 0)) <= 41,
				"驻守加速：生长倒计时 40s（实际 %d）" % int(p0.get("remaining", 0)))
			_dt().call("debug_age_plot", 0, 41.0) # 41s < 60s 基准：仅加速生效才成熟
			p0 = _dt().call("get_plot", 0)
			_check(bool(p0.get("mature", false)) == true, "41s 拨快即成熟（+50% 生长加速生效）")
			_press("interact") # X 切换随行模式
			_step = 4
		4:
			# ---- ③ 随行模式部署 ----
			_check(int(_dt().call("get_puppet_mode")) == 1, "X 切换随行模式")
			_check(abs(float(_dt().call("get_puppet_growth_mult")) - 1.0) < 0.001, "随行后生长倍率还原 1.0")
			_step = 5
		5:
			_check(bool(_dt().call("is_puppet_deployed")) == true, "随行傀儡已部署")
			var p = _puppet()
			_check(p != null, "dongtian_puppet 组实体在场")
			if p:
				_check(p.get_parent() == _dongtian_scene(), "傀儡挂在洞天场景同层")
				_check(not p.is_in_group("shen_wai_clones"), "傀儡不占身外化身上限组")
				var hp_ratio := float(p.call("get_max_health")) / float(_player().call("get_max_health"))
				_check(abs(hp_ratio - 0.4) < 0.001, "傀儡 HP=玩家×40%（实际 %s）" % String.num(hp_ratio, 2))
				var atk_ratio := float(p.call("get_attack_damage")) / float(_player().call("get_effective_attack"))
				_check(abs(atk_ratio - 0.4) < 0.001, "傀儡攻=玩家×40%（实际 %s）" % String.num(atk_ratio, 2))
				# ---- ④ 战死冷却 ----
				p.call("take_damage", 99999.0, null)
			_step = 6
		6:
			_check(bool(_dt().call("is_puppet_deployed")) == false, "傀儡战死后离场")
			var cd := float(_dt().call("get_puppet_cooldown"))
			_check(cd > 55.0 and cd <= 60.0, "战死冷却 60 息（实际 %.1f）" % cd)
			_check(_hud_has("傀儡战毁"), "HUD 播报傀儡战毁/修复中")
			_check(_puppets().is_empty(), "冷却期内不重新召出")
			_dt().call("debug_clear_puppet_cooldown")
			_step = 7
		7:
			_check(bool(_dt().call("is_puppet_deployed")) == true, "清冷却后傀儡重新召出（洞天内）")
			_press("dongtian") # O 出洞天
			_step = 8
		8:
			_check(_dt().call("is_inside") == false, "退出洞天")
			_step = 9
		9:
			# 傀儡随行到外界（挂在主场景根）
			var p = _puppet()
			_check(p != null and p.get_parent() == current_scene, "傀儡随行到洲野外（挂主场景根）")
			# ---- ⑤ 进 Portal 房间自动收回 ----
			_player().global_position = Vector2(3650, 210) # 古剑冢门口
			_next = _t + 0.6
			_step = 10
		10:
			_press("up")
			_next = _t + 0.6
			_step = 11
		11:
			var room = current_scene.get_node_or_null("GuJianZhong")
			_check(room != null, "古剑冢已挂载")
			_check(_player().get_parent() == room, "玩家已重挂载进秘境")
			_check(_puppets().is_empty(), "进 Portal 房间傀儡自动收回")
			_check(bool(_dt().call("is_puppet_deployed")) == false, "收回后部署记账清除")
			_check(float(_dt().call("get_puppet_cooldown")) <= 0.0, "收回非战死：无冷却")
			_player().position = Vector2(200, 220) # 走向出口
			_next = _t + 0.6
			_step = 12
		12:
			_press("up") # 出秘境
			_next = _t + 0.6
			_step = 13
		13:
			_check(current_scene.get_node_or_null("GuJianZhong") == null, "古剑冢已卸载")
			_check(_player().get_parent() == current_scene, "玩家回到主场景")
			var p = _puppet()
			_check(p != null and p.get_parent() == current_scene, "出房间傀儡重新部署")
			# ---- ⑥ 存档往返 ----
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 14
		14:
			_check(bool(_dt().call("is_puppet_active")) == true, "读档后傀儡激活状态持久")
			_check(int(_dt().call("get_puppet_mode")) == 1, "读档后随行模式持久")
			_check(int(_cur().call("get_total")) == 500, "读档后灵石余额持久（下品×500）")
			_step = 15
		15:
			var p = _puppet()
			_check(bool(_dt().call("is_puppet_deployed")) == true and p != null
				and p.get_parent() == current_scene, "读档后傀儡按态重新部署于外界")
			_dt().call("debug_suppress_invasion")
			_press("dongtian") # 重进洞天
			_step = 16
		16:
			_check(_dt().call("is_inside") == true, "读档后重进洞天")
			var p = _puppet()
			_check(p != null and p.get_parent() == _dongtian_scene(), "傀儡随行迁入洞天")
			_player().global_position = Vector2(40, 120) # 从浮台上方落上台面（直接放台面高度会穿过单向板坠地） # 回到傀儡 NPC 旁
			_next = _t + 0.5
			_step = 17
		17:
			_check(_hud_has("傀儡换岗"), "随行模式提示：[X] 傀儡换岗 · 驻守灵田")
			_press("interact") # X 切回驻守
			_step = 18
		18:
			_check(int(_dt().call("get_puppet_mode")) == 0, "X 切回驻守模式")
			_check(abs(float(_dt().call("get_puppet_growth_mult")) - 1.5) < 0.001, "驻守生长倍率恢复 1.5")
			_check(_puppets().is_empty() and bool(_dt().call("is_puppet_deployed")) == false,
				"切驻守后随行实体收回（无冷却 %.1f）" % float(_dt().call("get_puppet_cooldown")))
			_press("dongtian") # 退出收尾
			_step = 19
		19:
			_check(_dt().call("is_inside") == false, "退出洞天")
			_check(_puppets().is_empty(), "驻守模式无随行实体残留")
			return _finish()
	return false
