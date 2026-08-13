# 凌霄宝殿主线 + 飞升结局 测试：
# ①持久化 flags API ②巨灵神真死 → boss_dead flag（GameManager 挂钩 boss_fight_ended）
# ③条件门无 flag 拒入 ④有 flag ↑ 进门（Portal 房间模式）⑤太白金星叙事 once/after
# ⑥玉帝 precheck 拒绝（未金仙大圆满）⑦混元仪式全流程 → attain_hunyuan + ending_seen + 全恢复
# ⑧flags 随档（collect_save_data）⑨出门回天界
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _held: Array = []

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

func _gm():
	return root.find_child("GameManager", true, false)

func _bus():
	return root.find_child("SignalBus", true, false)

func _cm():
	return root.find_child("ContinentManager", true, false)

func _find(s: String):
	return root.find_child(s, true, false)

func _finish() -> bool:
	if _fail == 0:
		print("[TEST] ALL PASS")
	else:
		print("[TEST] ", _fail, " FAILURES")
	return true

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_release_all()
	_step += 1
	if _step > 40:
		print("[TEST] hard cap")
		return _finish()

	match _step:
		1:
			# ① flags API + 真仙上天界
			var gm = _gm()
			_check(not bool(gm.call("has_flag", "boss_dead:巨灵神")), "初始无 boss_dead flag")
			gm.call("set_flag", "test_flag", 42)
			_check(int(gm.call("get_flag", "test_flag", 0)) == 42, "set/get_flag 读写")
			_player().call("get_cultivation").call("set_realm", 10)
			_check(bool(_cm().call("travel_to_direct", "tianjie")), "travel 天界")
			_next = _t + 1.5
		2:
			var sc = String(current_scene.scene_file_path)
			_check(sc.ends_with("tianjie.tscn"), "到达天界: " + sc)
			_check(_find("LingXiaoGate") != null, "凌霄宝殿条件门存在")
			_next = _t + 0.3
		3:
			# ③ 无 flag 拒入：玩家到门口按 ↑，应留在天界
			_player().global_position = Vector2(3476, 210)
			_next = _t + 0.5
		4:
			_hold("up")
			_next = _t + 0.3
		5:
			var par = _player().get_parent()
			_check(par == current_scene, "无 flag 拒入（仍在天界）: " + par.name)
			# ② 真杀巨灵神 → 死亡链 emit boss_fight_ended("巨灵神") → flag
			var boss = _find("Boss_JuLingShen")
			_check(boss != null, "巨灵神在场")
			if boss:
				boss.call("take_damage", 99999.0, null)
			_next = _t + 0.6
		6:
			_check(bool(_gm().call("has_flag", "boss_dead:巨灵神")), "巨灵神死亡 → boss_dead flag 落档")
			_next = _t + 0.3
		7:
			# ④ 有 flag 进门
			_hold("up")
			_next = _t + 0.4
		8:
			var par = _player().get_parent()
			_check(par.name == "LingXiaoDian", "↑ 进凌霄宝殿: " + par.name)
			_check(_find("YuDi") != null, "玉帝 NarrativeNode 存在")
			_check(_find("TaiBaiJinXing") != null, "太白金星 NarrativeNode 存在")
			_next = _t + 0.3
		9:
			# ⑤ 太白金星首轮叙事
			_player().global_position = Vector2(200, 220)
			_next = _t + 0.5
		10:
			_hold("interact")
			_next = _t + 0.3
		11:
			var tb = _find("TaiBaiJinXing")
			_check(bool(tb.call("is_overlay_open")), "太白叙事 overlay 开")
			_check(String(tb.call("get_current_line")).begins_with("下界修士"), "太白首行=引见叙事")
			_hold("interact") # 推进 2/4
			_next = _t + 0.3
		12:
			_hold("interact") # 3/4
			_next = _t + 0.3
		13:
			_hold("interact") # 4/4
			_next = _t + 0.3
		14:
			_hold("interact") # 收尾关闭
			_next = _t + 0.3
		15:
			var tb = _find("TaiBaiJinXing")
			_check(not bool(tb.call("is_overlay_open")), "太白叙事关闭")
			_check(bool(_gm().call("has_flag", "yudi_intro")), "once_flag yudi_intro 落档")
			# 再交互 → after_lines
			_hold("interact")
			_next = _t + 0.3
		16:
			var tb = _find("TaiBaiJinXing")
			_check(bool(tb.call("is_overlay_open")), "太白 after_lines 开")
			_check(String(tb.call("get_current_line")).begins_with("金仙大圆满"), "after_lines=指引")
			_hold("interact") # 关闭
			_next = _t + 0.3
		17:
			# ⑥ 玉帝 precheck 拒绝（真仙未金仙）
			_player().global_position = Vector2(310, 220)
			_next = _t + 0.5
		18:
			_hold("interact")
			_next = _t + 0.3
		19:
			var yd = _find("YuDi")
			_check(bool(yd.call("is_overlay_open")), "玉帝拒绝叙事开")
			_check(String(yd.call("get_current_line")).contains("金仙"), "拒绝原因=修为未至金仙")
			_hold("interact") # 关闭
			_next = _t + 0.3
		20:
			var yd = _find("YuDi")
			_check(not bool(yd.call("is_overlay_open")), "拒绝叙事关闭（未落 flag）")
			_check(not bool(_gm().call("has_flag", "ending_seen")), "拒绝不触发结局")
			# ⑦ 金仙大圆满 + 仪式
			var cs = _player().call("get_cultivation")
			cs.call("set_realm", 11)
			cs.call("set_xianyuan", 999999)
			_next = _t + 0.3
		21:
			var r = String(_gm().call("check_hunyuan_ready"))
			_check(r == "", "check_hunyuan_ready 通过（金仙大圆满+巨灵神已伏）: " + r)
			_hold("interact")
			_next = _t + 0.3
		22:
			var yd = _find("YuDi")
			_check(bool(yd.call("is_overlay_open")), "混元仪式叙事开")
			_check(String(yd.call("get_current_line")).begins_with("朕御极万载"), "仪式首行")
			_hold("interact")
			_next = _t + 0.25
		23:
			_hold("interact")
			_next = _t + 0.25
		24:
			_hold("interact")
			_next = _t + 0.25
		25:
			_hold("interact")
			_next = _t + 0.25
		26:
			_hold("interact")
			_next = _t + 0.25
		27:
			_hold("interact")
			_next = _t + 0.25
		28:
			_hold("interact") # 第 7 行后关闭 → gm_method 回调
			_next = _t + 0.3
		29:
			var yd = _find("YuDi")
			var cs = _player().call("get_cultivation")
			_check(not bool(yd.call("is_overlay_open")), "仪式叙事关闭")
			_check(bool(cs.call("is_hunyuan")), "attain_hunyuan 成就混元一气")
			_check(bool(_gm().call("has_flag", "ending_seen")), "ending_seen 落档")
			var hp = float(_player().call("get_current_health"))
			var hpmax = float(_player().call("get_max_health"))
			_check(hp >= hpmax - 0.5, "结局全恢复 (%.0f/%.0f)" % [hp, hpmax])
			_next = _t + 0.3
		30:
			# ⑧ flags 随档
			var flags = _gm().call("collect_save_data").get("flags", {})
			_check(flags.has("boss_dead:巨灵神"), "存档含 boss_dead:巨灵神")
			_check(flags.has("ending_seen"), "存档含 ending_seen")
			# 仪式后再觐见 → after_lines（不重复触发结局）
			_hold("interact")
			_next = _t + 0.3
		31:
			var yd = _find("YuDi")
			_check(bool(yd.call("is_overlay_open")), "结局后再觐见=后日谈")
			_check(String(yd.call("get_current_line")).begins_with("混元金仙大驾"), "玉帝 after_lines")
			_hold("interact")
			_next = _t + 0.3
		32:
			# ⑨ 出门回天界
			_player().global_position = Vector2(240, 210) # ExitPortal 在房间中下方
			_next = _t + 0.5
		33:
			_hold("up")
			_next = _t + 0.4
		34:
			var par = _player().get_parent()
			_check(par == current_scene, "↑ 出门回天界: " + par.name)
			return _finish()
	return false
