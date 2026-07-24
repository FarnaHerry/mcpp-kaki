# 验证: ①化神解锁法则之力(上限100,补满) ②缩地成寸授予+装T槽 ③施放耗法则之力+瞬移+冷却
#      ④槽型校验(神通不能装仙法槽) ⑤法则之力存档 ⑥技能槽存档往返
extends SceneTree

var _t := 0.0
var _next := 0.0
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

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			_check(float(cult.call("get_law_power_max")) == 0.0, "law power locked before SPIRIT_SEVERING")
			# 连续突破到化神（realm 5）
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 100000000000)
			while int(cult.call("get_realm_index")) < 5:
				cult.call("attempt_breakthrough")
			print("[TEST] realm idx=", cult.call("get_realm_index"))
			_check(int(cult.call("get_realm_index")) >= 5, "reached SPIRIT_SEVERING")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			print("[TEST] law power=", cult.call("get_law_power"), "/", cult.call("get_law_power_max"))
			_check(float(cult.call("get_law_power_max")) == 100.0, "law power max 100 at SPIRIT_SEVERING")
			_check(float(cult.call("get_law_power")) == 100.0, "law power refilled on unlock")
			var sk = p.call("get_skills")
			var s6 = sk.call("get_slot_info", 6)
			print("[TEST] slot T: ", s6.get("id"))
			_check(String(s6.get("id")) == "suo_di_cheng_cun", "suo_di_cheng_cun granted to slot T")
		3:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			var sk = p.call("get_skills")
			# 出生点直接施放（不传送，避免落点在墙边）
			var x0 = p.global_position.x
			print("[TEST] blink from ", p.global_position)
			var law0 = float(cult.call("get_law_power"))
			var ok = bool(sk.call("cast_slot", 6))
			var x1 = p.global_position.x
			var law1 = float(cult.call("get_law_power"))
			print("[TEST] blink cast=", ok, " x ", x0, "->", x1, " law ", law0, "->", law1)
			_check(ok, "shentong cast succeeds")
			_check(x1 - x0 > 60.0, "blink moved player forward")
			_check(abs(law0 - law1 - 30.0) < 0.01, "shentong consumed 30 law power")
			_check(not bool(sk.call("cast_slot", 6)), "shentong cooldown blocks recast")
		4:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var sk = p.call("get_skills")
			# 槽型校验：神通不能装仙法槽 Y，也不能装武技槽 A
			_check(not bool(sk.call("assign", 7, "suo_di_cheng_cun")), "shentong rejected from xianfa slot")
			_check(not bool(sk.call("assign", 0, "suo_di_cheng_cun")), "shentong rejected from martial slot")
		5:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			var sk = p.call("get_skills")
			# 法则之力存档接口
			cult.call("set_law_power", 55.0)
			_check(float(cult.call("get_law_power")) == 55.0, "set_law_power works")
			# 技能槽存档往返
			var d = sk.call("save_to_dict")
			sk.call("load_from_dict", d)
			var s6 = sk.call("get_slot_info", 6)
			_check(String(s6.get("id")) == "suo_di_cheng_cun", "save/load shentong slot roundtrip")
		6:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
