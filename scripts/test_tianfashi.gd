# 天罚使正式多阶段 Boss harness（roadmap「天罚使换正式 Boss」）：
# ① tian_fa_shi 定义装配（enemies.json → set_enemy_id：雷法远程 boss/中文名/颜色尺寸）
# ② 一相纯雷球弹幕（special_min_phase=2，无扇形弹）
# ③ 66% → 二相「雷链」（BossSpecial 5 发扇形弹，标题/信号）
# ④ 33% → 三相「雷域」（脚下落雷圈 DomainBolt + 移速/射速提升，与半血激怒叠加）
# ⑤ 斩杀 → tribulation_finished(true) → 渡劫成真仙
# 注：一帧只按一键；挂机观察窗每步补满玩家血防意外战死
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _phases_seen: Array = []
var _finished_ok := false
var _finished_got := false
var _fan_max_p1 := 0
var _fan_max_p2 := 0
var _record_fan := 0 # 0=不记录 / 1=一相窗 / 2=二相窗（逐帧采样——雷弹 0.3s 内即中玩家，按步采样会漏）
var _domain_seen := false

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

func _check(cond: bool, msg: String):
	if cond:
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)

func _p():
	return root.find_child("Player", true, false)

func _cs():
	var p = _p()
	return p.call("get_cultivation") if p != null else null

func _tc():
	return root.find_child("TribulationController", true, false)

func _boss():
	return root.find_child("TribulationBoss", true, false)

func _title_text() -> String:
	var tc = _tc()
	if tc == null:
		return ""
	var lab = tc.find_child("*Label*", true, false) # 运行时 memnew 未命名 → @Label@N
	return String(lab.get("text")) if lab != null else ""

# Boss 扇形弹（enemy.cpp BossSpecial 命名 BossFan，重名自动唯一化含 @BossFan@N）
func _count_fan() -> int:
	var b = _boss()
	if b == null or b.get_parent() == null:
		return 0
	var n := 0
	for c in b.get_parent().get_children():
		if "BossFan" in String(c.name):
			n += 1
	return n

func _top_up_hp():
	var p = _p()
	if p != null:
		p.call("set_current_health", 999999.0)

func _on_phase(p):
	_phases_seen.append(int(p))

func _on_finished(success):
	_finished_got = true
	_finished_ok = bool(success)

func _process(delta) -> bool:
	# 逐帧扇形弹采样（先于时间门）
	if _record_fan == 1:
		_fan_max_p1 = maxi(_fan_max_p1, _count_fan())
	elif _record_fan == 2:
		_fan_max_p2 = maxi(_fan_max_p2, _count_fan())

	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	var cs = _cs()
	if cs == null:
		return false

	match _step:
		0:
			# 大乘圆满 + 免门槛 → 请求机缘突破（=渡劫事件）
			cs.call("set_realm", 8)
			cs.call("set_spiritual_energy", 999999999)
			cs.call("set_free_breakthrough", true)
			_step = 1
		1:
			var bus = root.find_child("SignalBus", true, false)
			bus.emit_signal("breakthrough_requested")
			_step = 2
		2, 3, 4, 5, 6:
			_press("interact") # 推进 intro overlay
			_step += 1
		7:
			_press("interact")
			_step = 8
		8:
			# ---- ① 定义装配断言 ----
			var tc = _tc()
			_check(tc != null and tc.call("is_boss_alive") == true, "渡劫启动：天罚使已降临")
			var b = _boss()
			_check(b != null, "天罚使节点存在（TribulationBoss）")
			if b != null:
				_check(String(b.get("enemy_id")) == "tian_fa_shi", "天罚使按 enemies.json tian_fa_shi 定义装配")
				_check(String(b.get("display_name")) == "天罚使", "定义名=天罚使")
				_check(b.get("is_boss") == true, "定义 flags 含 boss")
				_check(b.get("is_ranged") == true, "定义 flags 含 ranged（雷法远程）")
				_check(float(b.get("attack_range")) == 260.0, "定义 attack_range=260")
				_check(float(b.get("preferred_distance")) == 200.0, "定义 preferred=200（保持距离）")
				_check(b.get("no_drops") == true, "no_drops（掉落由渡劫流程控制）")
				_check(float(b.get("max_health")) == 2500.0, "总血量 2500（控制器显式接管，渡劫时长不膨胀）")
				_check(int(b.get("realm")) == int(cs.call("get_realm_index")), "realm 镜像玩家（威压/灵压不可慑服）")
				_check(int(b.get("special_min_phase")) == 2, "一相纯弹幕：扇形弹最低阶段=2")
				_check(int(tc.call("get_boss_phase")) == 1, "初始一相（get_boss_phase==1）")
				tc.connect("boss_phase_changed", Callable(self, "_on_phase"))
				tc.connect("tribulation_finished", Callable(self, "_on_finished"))
			_record_fan = 1
			_step = 9
		9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33:
			# ---- ② 一相观察窗（~7.5s）：纯雷球弹幕，无扇形弹（逐帧采样在 _process 顶部）----
			_top_up_hp()
			_step += 1
		34:
			_record_fan = 0
			_check(_fan_max_p1 == 0, "一相纯雷球弹幕：无扇形弹（BossFan max=%d）" % _fan_max_p1)
			# 打到 58% → 二相
			var b = _boss()
			if b != null:
				b.call("take_damage", 1050.0, null) # 2500-1050=1450 = 58%
			_step = 35
		35:
			# ---- ③ 二相「雷链」断言 ----
			var tc = _tc()
			var b = _boss()
			_check(tc != null and int(tc.call("get_boss_phase")) == 2, "66% 阈值 → 二相（get_boss_phase==2）")
			_check(b != null and int(b.get("boss_phase")) == 2, "敌方状态机 boss_phase 同步=2")
			_check(2 in _phases_seen, "boss_phase_changed(2) 信号已发")
			_check("雷链" in _title_text(), "阶段 HUD 标题含「雷链」（%s）" % _title_text())
			_record_fan = 2
			_step = 36
		36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99:
			# ---- 二相观察窗（~19s）：雷链扇形弹（逐帧采样在 _process 顶部）----
			_top_up_hp()
			_step += 1
		100:
			_record_fan = 0
			_check(_fan_max_p2 >= 4, "二相雷链：5 发扇形雷弹（BossFan 同帧峰=%d，容许 1 发即中）" % _fan_max_p2)
			# 打到 30% → 三相
			var b = _boss()
			if b != null:
				b.call("take_damage", 700.0, null) # 1450-700=750 = 30%
			_step = 101
		101:
			# ---- ④ 三相「雷域」断言 ----
			var tc = _tc()
			var b = _boss()
			_check(tc != null and int(tc.call("get_boss_phase")) == 3, "33% 阈值 → 三相（get_boss_phase==3）")
			_check(b != null and int(b.get("boss_phase")) == 3, "敌方状态机 boss_phase 同步=3")
			_check(3 in _phases_seen, "boss_phase_changed(3) 信号已发")
			_check("雷域" in _title_text(), "阶段 HUD 标题含「雷域」（%s）" % _title_text())
			if b != null:
				_check(float(b.get("move_speed")) > 80.0, "三相激怒提速（70→%.1f）" % float(b.get("move_speed")))
				_check(float(b.get("attack_cooldown")) < 1.3, "三相激怒射速提升（1.6→%.2f）" % float(b.get("attack_cooldown")))
			_step = 102
		102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121:
			# ---- 三相观察窗（~6s）：脚下雷域落雷圈 ----
			_top_up_hp()
			var tc2 = _tc()
			if tc2 != null and tc2.find_child("DomainBolt*", true, false) != null:
				_domain_seen = true
			_step += 1
		122:
			_check(_domain_seen, "三相雷域：脚下周期性落雷圈出现（DomainBolt）")
			_step = 123
		123:
			# ---- ⑤ 斩杀 → 渡劫成 ----
			var b = _boss()
			if b != null:
				b.call("take_damage", 999999.0, null)
			_step = 124
		124, 125, 126:
			_step += 1
		127:
			_check(_finished_got and _finished_ok, "斩天罚使 → tribulation_finished(true)")
			_check(int(cs.call("get_realm_index")) == 10, "渡劫成功 → 真仙 realm=10")
			_step = 128
		128:
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
