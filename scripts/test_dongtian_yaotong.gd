# 洞天 v5 harness（药童托付照料 + 灵兽栏降伏入驻）：
# ① 药童节点/灵兽栏装配就位，贴近提示 [X] 托付灵田给药童
# ② X 委托 → 成熟地块自动收获入仓库（0.5s 轮询），台词气泡 2.5s
# ③ X 取消委托 → 成熟地块不再自动收获
# ④ 闯阵灵兽打至半血以下 → 贴近 [X] 降伏（不击杀无掉落）→ 迁入灵兽栏，
#    每只打坐倍率 +5%（get_dongtian_meditate_mult 挂钩）
# ⑤ 栏满 3 只后提示「灵兽栏已满」，不可再降伏（照旧击杀）
# ⑥ 存档往返：委托状态/灵兽栏/仓库持久化，重进洞天栏内视觉重建
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

func _invaders() -> Array:
	return get_nodes_in_group("dongtian_invaders")

func _dongtian_scene():
	return root.find_child("Dongtian", true, false)

func _yaotong():
	return root.find_child("YaoTong", true, false)

func _pen():
	return root.find_child("BeastPen", true, false)

# 仓库内某物品总数量（药童收获的落点）
func _storage_count(id: String) -> int:
	var total := 0
	for i in 48:
		var s: Dictionary = _dt().call("get_storage_slot", i)
		if String(s.get("id", "")) == id:
			total += int(s.get("quantity", 0))
	return total

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

# 打至半血以下（降伏前提）并贴近
func _weaken_and_approach(e) -> float:
	var hp := float(e.get("current_health"))
	e.call("take_damage", hp * 0.6, null)
	_player().global_position = e.global_position + Vector2(24, 0)
	return hp

func _kill_rest_invaders():
	for e in _invaders():
		e.call("take_damage", 99999.0, null)

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
			# 修为降到半管（避免封顶自动请求突破干扰）
			var max_e := int(_cult().call("get_max_energy"))
			_cult().call("set_spiritual_energy", int(max_e * 0.5))
			_dt().call("debug_suppress_invasion")
			_press("dongtian")
			_step = 1
		1:
			# ---- ① 装配 ----
			_check(_dt().call("is_inside") == true, "进入洞天")
			_check(_yaotong() != null, "药童节点就位")
			_check(_pen() != null, "灵兽栏节点就位")
			_check(bool(_dt().call("is_yaotong_hired")) == false, "初始未委托")
			_check(int(_dt().call("get_beast_count")) == 0, "初始灵兽栏为空")
			_player().global_position = Vector2(452, 214)
			_next = _t + 0.5
			_step = 2
		2:
			_check(_hud_has("托付灵田给药童"), "贴近药童提示：[X] 托付灵田给药童")
			# 备料：聚灵草×3（生长 60s），播种地块 0 并拨快成熟
			_player().call("pickup_item", "ju_ling_cao", 3)
			_check(bool(_dt().call("plant", 0)) == true, "地块 0 播种聚灵草")
			_dt().call("debug_age_plot", 0, 70.0)
			var p0: Dictionary = _dt().call("get_plot", 0)
			_check(bool(p0.get("mature", false)) == true, "地块 0 已成熟（待收）")
			_press("interact") # X 委托
			_step = 3
		3:
			# ---- ② 委托 → 自动收获入仓库 ----
			_check(bool(_dt().call("is_yaotong_hired")) == true, "X 后委托生效")
			var bub = _yaotong().get("_bubble")
			_check(bub != null and bub.visible, "委托后台词气泡显示")
			# 走远（255 无设施提示覆盖），静候 0.5s 轮询采收播报
			_player().global_position = Vector2(255, 214)
			_next = _t + 0.9 # 等 0.5s 轮询节拍
			_step = 4
		4:
			var p0: Dictionary = _dt().call("get_plot", 0)
			_check(bool(p0.get("empty", false)) == true, "委托后成熟地块被药童自动收获")
			_check(_storage_count("ju_ling_cao") == 2, "收获物入仓库（聚灵草×2，实际 %d）" % _storage_count("ju_ling_cao"))
			_check(_hud_has("药童采收"), "HUD 播报「药童采收 … 已存入仓库」")
			_player().global_position = Vector2(452, 214) # 走回药童旁（x=452）
			_next = _t + 0.5
			_step = 5
		5:
			# ---- ③ 取消委托 → 不再自动收获 ----
			_press("interact") # X 取消委托
			_step = 6
		6:
			_check(bool(_dt().call("is_yaotong_hired")) == false, "再按 X 收回委托")
			_check(bool(_dt().call("plant", 1)) == true, "地块 1 播种聚灵草")
			_dt().call("debug_age_plot", 1, 70.0)
			_next = _t + 0.9
			_step = 7
		7:
			var p1: Dictionary = _dt().call("get_plot", 1)
			_check(bool(p1.get("empty", true)) == false, "取消委托后成熟地块不再自动收获")
			_check(_storage_count("ju_ling_cao") == 2, "仓库数量不变（仍 ×2）")
			# ---- ④ 降伏入驻 ----
			_dt().call("debug_force_invasion")
			_step = 8
		8:
			_check(bool(_dt().call("is_invasion_active")) == true, "强制闯阵触发")
			var inv := _invaders()
			_check(inv.size() >= 1, "入侵者出现（%d 只）" % inv.size())
			_weaken_and_approach(inv[0])
			_next = _t + 0.4
			_step = 9
		9:
			_check(_hud_has("降伏"), "半血以下贴近提示：[X] 降伏")
			_press("interact") # X 降伏
			_step = 10
		10:
			_check(int(_dt().call("get_beast_count")) == 1, "降伏成功：灵兽栏 +1")
			var b0: Dictionary = _dt().call("get_beast", 0)
			_check(["qie_ling_shu", "tan_ling_feng"].has(String(b0.get("id", ""))), "栏内灵兽为入侵种类")
			_check(String(b0.get("name", "")) in ["窃灵鼠", "贪灵蜂"], "栏内灵兽中文名（%s）" % String(b0.get("name", "")))
			_kill_rest_invaders() # 其余照旧击杀（死亡/掉落管线不变）
			_step = 11
		11:
			_check(bool(_dt().call("is_invasion_active")) == false, "闯阵结束（降伏+击杀清场）")
			var pen = _pen()
			_check(pen != null and pen.get_node_or_null("BeastVisual0") != null, "栏内灵兽视觉重建（BeastVisual0）")
			var mult := float(_player().call("get_dongtian_meditate_mult"))
			_check(abs(mult - 2.05) < 0.001, "1 只灵兽打坐倍率 +0.05（基准 2.05，实际 %.3f）" % mult)
			# 第二次闯阵：降伏第 2 只
			_dt().call("debug_force_invasion")
			_step = 12
		12:
			var inv := _invaders()
			if inv.is_empty():
				return false # 等生成
			_weaken_and_approach(inv[0])
			_next = _t + 0.4
			_step = 13
		13:
			_press("interact")
			_step = 14
		14:
			_check(int(_dt().call("get_beast_count")) == 2, "降伏第 2 只")
			_kill_rest_invaders()
			_step = 15
		15:
			# 第三次闯阵：降伏第 3 只（栏满）
			_dt().call("debug_force_invasion")
			_step = 16
		16:
			var inv := _invaders()
			if inv.is_empty():
				return false
			_weaken_and_approach(inv[0])
			_next = _t + 0.4
			_step = 17
		17:
			_press("interact")
			_step = 18
		18:
			_check(int(_dt().call("get_beast_count")) == 3, "降伏第 3 只（栏满）")
			var bonus := float(_dt().call("get_beast_bonus"))
			_check(abs(bonus - 0.15) < 0.001, "3 只灵兽加成 +0.15（实际 %.3f）" % bonus)
			_kill_rest_invaders()
			_step = 19
		19:
			# ---- ⑤ 栏满后不可再降伏 ----
			_dt().call("debug_force_invasion")
			_step = 20
		20:
			var inv := _invaders()
			if inv.is_empty():
				return false
			_weaken_and_approach(inv[0])
			_next = _t + 0.4
			_step = 21
		21:
			_check(_hud_has("灵兽栏已满"), "栏满提示「灵兽栏已满，无法降伏」")
			_press("interact") # 尝试降伏 → 应无效
			_step = 22
		22:
			_check(int(_dt().call("get_beast_count")) == 3, "栏满后 X 降伏无效（仍 3 只）")
			_kill_rest_invaders()
			_step = 23
		23:
			_check(bool(_dt().call("is_invasion_active")) == false, "栏满后照旧击杀清场")
			# ---- ⑥ 存档往返 ----
			_player().global_position = Vector2(452, 214) # 回到药童旁（x=452）
			_next = _t + 0.5
			_step = 24
		24:
			_press("interact") # 重新委托（测委托状态持久化）
			_step = 25
		25:
			_check(bool(_dt().call("is_yaotong_hired")) == true, "重新委托生效")
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 26
		26:
			_check(_dt().call("is_inside") == false, "读档后已退出洞天")
			_check(bool(_dt().call("is_yaotong_hired")) == true, "读档后委托状态持久")
			_check(int(_dt().call("get_beast_count")) == 3, "读档后灵兽栏 3 只持久")
			_check(_storage_count("ju_ling_cao") == 2, "读档后仓库聚灵草×2 持久")
			# 重进洞天：栏内视觉按存档重建
			_dt().call("debug_suppress_invasion")
			_press("dongtian")
			_step = 27
		27:
			_check(_dt().call("is_inside") == true, "读档后重进洞天")
			var pen = _pen()
			_check(pen != null
				and pen.get_node_or_null("BeastVisual0") != null
				and pen.get_node_or_null("BeastVisual1") != null
				and pen.get_node_or_null("BeastVisual2") != null, "重进后栏内 3 只灵兽视觉重建")
			var mult := float(_player().call("get_dongtian_meditate_mult"))
			_check(abs(mult - 2.15) < 0.001, "3 只灵兽打坐倍率 +0.15（基准 2.15，实际 %.3f）" % mult)
			_press("dongtian") # 退出
			_step = 28
		28:
			_check(_dt().call("is_inside") == false, "退出洞天")
			return _finish()
	return false
