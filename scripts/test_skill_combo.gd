# 连招派生（Combo）系统测试：
#   ④ 4 组连招数据加载正确（get_combo_list）
#   ③ 非组合顺序不强化（突进→突进 / 裸放突进，mult=1.0，基准伤害）
#   ② 窗口外施放不强化（破空→等 3.2s→突进，mult=1.0，伤害≈基准）
#   ① 窗口内连招强化（破空→0.5s→突进，mult=1.5，伤害≈基准×1.5 + 提示语发出）
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _stake = null          # 木桩 Enemy（HP 10 万，免防，不会被打死）
var _hp_before := 0.0
var _d_base := 0.0         # 突进斩裸放基准伤害
var _d_out := 0.0          # 窗口外伤害
var _atk_at_cast := 0.0    # 施放后实时有效攻击（喂功法会缓涨，伤害断言按它换算期望）
var _combo_hint_seen := false

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

func _skills():
	return _player().call("get_skills")

# 木桩贴到玩家面朝侧（玩家默认面朝右），每次施放前重摆（击退/追击会位移）
func _place_stake():
	var p = _player()
	_stake.global_position = p.global_position + Vector2(30, 0)
	_stake.set("velocity", Vector2(0, 0))

func _on_prompt(text: String, show: bool):
	if show and text.contains("剑势暴涨"):
		_combo_hint_seen = true

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_step += 1

	match _step:
		1:
			_next = _t + 0.5
			var p = _player()
			_check(p != null, "player exists")
			var sk = _skills()
			_check(sk != null, "skill system exists")

			# ④ 连招数据加载（data/skills.json combo_* 字段）
			var combos = sk.call("get_combo_list")
			_check(combos.size() == 4, "combo list size == 4 (got %d)" % combos.size())
			var by_pair := {}
			for c in combos:
				by_pair[String(c["skill_id"]) + "<-" + String(c["after_id"])] = c
			var expect = {
				"tu_jin_zhan<-po_kong_zhan": 1.5,
				"lei_zhou_shu<-huo_dan_shu": 1.4,
				"sheng_long_ji<-xuan_feng_zhan": 1.5,
				"yu_jian_shu<-bing_zhui_shu": 1.4,
			}
			for pair in expect:
				var ok = by_pair.has(pair) \
					and abs(float(by_pair[pair]["mult"]) - expect[pair]) < 0.001 \
					and abs(float(by_pair[pair]["window"]) - 3.0) < 0.001 \
					and String(by_pair[pair]["text"]) != ""
				_check(ok, "combo data %s x%.1f window=3.0 text!=empty" % [pair, expect[pair]])

			# 木桩 Enemy（HP 10 万、防 0），放在玩家旁
			_stake = ClassDB.instantiate("Enemy")
			_check(_stake != null, "stake enemy instantiated")
			current_scene.add_child(_stake)
			_stake.set("max_health", 100000.0)
			_stake.set("current_health", 100000.0)
			_place_stake()

			# 监听连招提示（interaction_prompt 复用通道）
			var bus = root.find_child("SignalBus", true, false)
			_check(bus != null, "signal bus exists")
			if bus:
				bus.connect("interaction_prompt", _on_prompt)

			# 起手技能已装：A(6)=破空斩 S(7)=突进斩
			_check(String(sk.call("get_slot_info", 6).get("id")) == "po_kong_zhan", "slot A = po_kong_zhan")
			_check(String(sk.call("get_slot_info", 7).get("id")) == "tu_jin_zhan", "slot S = tu_jin_zhan")
		2:
			# ③ 非组合顺序（裸放突进，last_cast 为空）→ 不强化；记录基准伤害
			_next = _t + 0.4
			var sk = _skills()
			_place_stake()
			_hp_before = float(_stake.get("current_health"))
			_check(bool(sk.call("cast_slot", 7)), "cast 突进斩 (bare)")
			_check(abs(float(sk.call("get_last_combo_mult")) - 1.0) < 0.001, "bare 突进斩 mult == 1.0")
		3:
			_next = _t + 0.4
			_d_base = _hp_before - float(_stake.get("current_health"))
			print("[TEST] base 突进斩 dmg = ", _d_base)
			_check(_d_base > 0.0, "bare 突进斩 damaged stake")
		4:
			# ② 窗口外：先施放破空斩（last_cast=po_kong），等 3.2s（>窗口 3.0）再放突进
			_next = _t + 3.2
			var sk = _skills()
			_place_stake()
			_check(bool(sk.call("cast_slot", 6)), "cast 破空斩 (window-out setup)")
		5:
			# 距破空斩施放 3.2s，连招窗口已过；突进斩冷却 1.8s 早好
			# （每次施放会喂功法炼体使攻击缓涨——伤害断言一律按施放后的实时攻击换算期望值）
			_next = _t + 0.4
			var sk = _skills()
			_place_stake()
			_hp_before = float(_stake.get("current_health"))
			_check(bool(sk.call("cast_slot", 7)), "cast 突进斩 after 3.2s (window expired)")
			_check(abs(float(sk.call("get_last_combo_mult")) - 1.0) < 0.001, "window-expired 突进斩 mult == 1.0")
			_atk_at_cast = float(_player().call("get_effective_attack"))
		6:
			_next = _t + 1.9
			_d_out = _hp_before - float(_stake.get("current_health"))
			var expect_out = _atk_at_cast * 3.5 # 突进斩 power=3.5，木桩防 0
			print("[TEST] window-out dmg = ", _d_out, " (expect ~", expect_out, ")")
			_check(abs(_d_out - expect_out) < 2.0, "window-expired dmg = atk x 3.5 (no boost)")
		7:
			# ① 连招窗口内：破空斩 → 约1.4s后突进斩（间隔 1.4s 让突进斩冷却(1.8s)转好且仍在窗口(3.0s)内）
			_next = _t + 1.4
			var sk = _skills()
			_place_stake()
			_check(bool(sk.call("cast_slot", 6)), "cast 破空斩 (combo starter)")
		8:
			_next = _t + 0.4
			var sk = _skills()
			_place_stake()
			_hp_before = float(_stake.get("current_health"))
			_check(bool(sk.call("cast_slot", 7)), "cast 突进斩 within 0.4s (combo)")
			_check(abs(float(sk.call("get_last_combo_mult")) - 1.5) < 0.001, "combo 突进斩 mult == 1.5")
			_atk_at_cast = float(_player().call("get_effective_attack"))
		9:
			_next = _t + 1.9 # 让突进斩冷却(1.8s)转好，供 step10 错序重放
			var d_boost = _hp_before - float(_stake.get("current_health"))
			var expect_boost = _atk_at_cast * 3.5 * 1.5 # power 3.5 x combo 1.5，木桩防 0
			print("[TEST] combo dmg = ", d_boost, " (expect ~", expect_boost, ")")
			_check(abs(d_boost - expect_boost) < 2.0, "combo dmg = atk x 3.5 x 1.5")
			_check(_combo_hint_seen, "combo hint prompt shown (破空接突进——剑势暴涨！)")
		10:
			# ③ 非组合顺序：last_cast=突进斩，窗口仍开着（约 2.3s<3.0s），但前置 id 不匹配 → 不强化
			_next = _t + 0.4
			var sk = _skills()
			_place_stake()
			_hp_before = float(_stake.get("current_health"))
			_check(bool(sk.call("cast_slot", 7)), "cast 突进斩 after 突进斩 (wrong order, cd ok)")
			_check(abs(float(sk.call("get_last_combo_mult")) - 1.0) < 0.001, "wrong-order 突进斩 mult == 1.0")
			_atk_at_cast = float(_player().call("get_effective_attack"))
		11:
			_next = _t + 0.3
			var d_wrong = _hp_before - float(_stake.get("current_health"))
			var expect_wrong = _atk_at_cast * 3.5
			print("[TEST] wrong-order dmg = ", d_wrong, " (expect ~", expect_wrong, ")")
			_check(abs(d_wrong - expect_wrong) < 2.0, "wrong-order dmg = atk x 3.5 (no boost)")
		12:
			if is_instance_valid(_stake):
				_stake.queue_free()
			print("[TEST] DONE fail=", _fail)
			if _fail == 0:
				print("[TEST] ALL PASS")
			return true
	return false
