# 法宝系统完善 v2 验证:
# ①次要法宝×3 物品习得（learn_artifact）②威力系数两段温养（1.0→1.2→1.5）
# ③飞升解锁 6 槽（unlock_secondary_slots + 存档往返）④渡劫「只带本命法宝」禁用-恢复
# ⑤法宝页可交互：X 设本命 / 觉醒后拒绝 / A~H 装槽
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _fail := 0
var _release_next := ""

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

# 按住一帧再释放（同帧 press+release 对 action 轮询不可靠）
func _hold(action: String):
	Input.action_press(action)
	_release_next = action

func _press_key(code: int):
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	Input.parse_input_event(ev)

func _scan(n: Node, s: String) -> bool:
	if n is Label and String(n.text).contains(s):
		return true
	for c in n.get_children():
		if _scan(c, s):
			return true
	return false

func _menu_has(s: String) -> bool:
	var menu = root.find_child("GameMenu", true, false)
	return menu != null and _scan(menu, s)

func _equip(item_id: String) -> bool:
	var p = _player()
	var inv = p.call("get_inventory")
	inv.call("add_item", item_id, 1)
	for i in range(inv.call("get_capacity")):
		var sd = inv.call("get_slot", i)
		if not sd.is_empty() and String(sd["id"]) == item_id:
			return bool(p.call("equip_item", i))
	return false

func _process(delta) -> bool:
	_t += delta
	if _release_next != "":
		Input.action_release(_release_next)
		_release_next = ""
	if _t < _next:
		return false
	_next = _t + 0.3
	_step += 1

	match _step:
		1:
			var p = _player()
			_check(p != null, "player exists")
			var ar = p.call("get_artifacts")
			_check(ar != null, "artifact system exists")
			_check(int(ar.call("get_slot_limit")) == 3, "slot limit 3 pre-ascension")
			_check(not bool(ar.call("equip", 4, "fei_jian")), "slot 4 locked pre-ascension")
		2:
			# ①次要法宝×3：物品使用习得（learn_artifact 统一入口）
			var p = _player()
			var inv = p.call("get_inventory")
			var ar = p.call("get_artifacts")
			for id in ["ba_gua_lu", "kun_xian_sheng", "ding_feng_zhu"]:
				_check(bool(inv.call("add_item", id, 1)), "got item " + id)
				_check(bool(p.call("use_consumable", id)), "use " + id)
				_check(bool(ar.call("is_owned", id)), "artifact owned: " + id)
			ar.call("acquire", "fei_jian") # 攻击型本命候选
		3:
			# ②威力系数：次要 1.0→1.2→1.5 两段温养；辅助被动（八卦炉攻+15%/定风珠风抗+30%）
			var ar = _player().call("get_artifacts")
			ar.call("equip", 1, "ba_gua_lu")
			ar.call("equip", 2, "ding_feng_zhu")
			_check(abs(float(ar.call("get_slot_coeff", 1)) - 1.0) < 0.001, "secondary coeff starts 1.0")
			_check(abs(float(ar.call("get_passive_atk_bonus")) - 0.15) < 0.001, "ba_gua_lu atk +15%")
			_check(abs(float(ar.call("get_passive_elem_resist", 7)) - 0.30) < 0.001, "ding_feng_zhu wind resist +30%")
			ar.call("nurture_equipped", 300.0)
			_check(abs(float(ar.call("get_slot_coeff", 1)) - 1.2) < 0.001, "nurture 300 -> coeff 1.2")
			_check(abs(float(ar.call("get_passive_atk_bonus")) - 0.18) < 0.001, "atk bonus scales 0.18")
			ar.call("nurture_equipped", 300.0)
			_check(abs(float(ar.call("get_slot_coeff", 1)) - 1.5) < 0.001, "nurture 600 -> coeff 1.5")
		4:
			# ⑤法宝页可交互：开菜单 → E×4 到法宝页
			_hold("menu")
		5:
			for i in range(4):
				_press_key(KEY_E)
		6:
			_check(_menu_has("—— 法宝 ——"), "artifact page open")
			_check(_menu_has("已拥有法宝"), "owned list section shown")
			_check(_menu_has("飞升解锁"), "locked slots dim-marked pre-ascension")
			# 选中项 0 = fei_jian（owned 顺序按定义表），X 设本命
			_hold("interact")
		7:
			var p = _player()
			_check(String(p.call("get_benming_artifact")) == "fei_jian", "X sets benming = fei_jian")
			_check(_menu_has("已设本命法宝"), "benming set message shown")
		8:
			# 觉醒锁定后再设被拒
			var p = _player()
			p.call("awaken_benming_artifact")
			_check(bool(p.call("is_benming_awakened")), "benming awakened (locked)")
			_hold("right") # 移到 ba_gua_lu
		9:
			_hold("interact") # 试图换本命
		10:
			var p = _player()
			_check(String(p.call("get_benming_artifact")) == "fei_jian", "benming unchanged after lock")
			_check(_menu_has("本命已锁定"), "locked refusal message shown")
		11:
			# A~H 装入对应槽：skill_d = 槽2（当前选中 ba_gua_lu）
			_hold("skill_d")
		12:
			var ar = _player().call("get_artifacts")
			var info2 = ar.call("get_slot_info", 2)
			_check(String(info2.get("id")) == "ba_gua_lu", "D key equips slot 2 = ba_gua_lu")
			_hold("menu") # ESC 关菜单
		13:
			# ③飞升解锁 6 槽
			var p = _player()
			var cult = p.call("get_cultivation")
			cult.call("set_realm", 10) # 真仙：realm_changed → unlock_secondary_slots
			var ar = p.call("get_artifacts")
			_check(int(ar.call("get_slot_limit")) == 6, "slot limit 6 after ascension")
			_check(bool(ar.call("is_secondary_unlocked")), "secondary slots unlocked flag")
			_check(bool(ar.call("equip", 4, "kun_xian_sheng")), "equip slot 4 post-ascension")
			var info4 = ar.call("get_slot_info", 4)
			_check(String(info4.get("id")) == "kun_xian_sheng", "slot 4 = kun_xian_sheng")
			ar.call("equip", 1, "ding_feng_zhu") # 槽1 装回定风珠（渡劫抑制测试用）
			ar.call("equip", 2, "ba_gua_lu")   # 槽2 装回八卦炉（境界赠照妖葫会覆盖槽2）
		14:
			# 存档往返：解锁标记 + 次要槽 + 温养保持
			var ar = _player().call("get_artifacts")
			var d = ar.call("save_to_dict")
			ar.call("load_from_dict", d)
			_check(bool(ar.call("is_secondary_unlocked")), "save/load keeps unlocked")
			var info4 = ar.call("get_slot_info", 4)
			_check(String(info4.get("id")) == "kun_xian_sheng", "save/load keeps slot 4")
			_check(float(ar.call("get_slot_coeff", 2)) >= 1.49, "save/load keeps nurture coeff")
			_check(String(_player().call("get_benming_artifact")) == "fei_jian", "benming survives load")
		15:
			# ④渡劫「只带本命法宝」：入 arena 禁用次要法宝 + 装备加成
			var p = _player()
			_check(_equip("ding_hai_shen_zhen"), "equip weapon (atk +25)")
			var atk0 = float(p.call("get_equip_bonus_attack"))
			var ar = p.call("get_artifacts")
			var wind0 = float(ar.call("get_passive_elem_resist", 7))
			print("[TEST] pre-trib: equip_atk=", atk0, " wind_res=", wind0)
			_check(atk0 > 0.0, "equip atk bonus active pre-tribulation")
			_check(wind0 > 0.0, "artifact wind resist active pre-tribulation")
			p.call("enter_tribulation")
			_check(bool(p.call("is_in_tribulation")), "in tribulation")
			_check(bool(ar.call("is_tribulation_mode")), "artifacts tribulation mode")
			_check(float(p.call("get_equip_bonus_attack")) == 0.0, "equip atk bonus zeroed in tribulation")
			_check(float(ar.call("get_passive_elem_resist", 7)) == 0.0, "secondary artifact passive suppressed")
			var info4 = ar.call("get_slot_info", 4)
			_check(info4.has("tribulation_off"), "slot 4 marked tribulation_off for UI")
		16:
			# 次要法宝祭出被拒；本命仍可祭出
			var p = _player()
			var cult = p.call("get_cultivation")
			var ar = p.call("get_artifacts")
			cult.call("set_mana", 200.0)
			_check(not bool(ar.call("activate_slot", 4)), "secondary activation refused in tribulation")
			_check(bool(ar.call("activate_slot", 0)), "benming (fei_jian) still activatable")
		17:
			# 渡劫毕恢复
			var p = _player()
			var ar = p.call("get_artifacts")
			p.call("exit_tribulation")
			_check(not bool(p.call("is_in_tribulation")), "exited tribulation")
			_check(float(p.call("get_equip_bonus_attack")) > 0.0, "equip atk bonus restored")
			_check(float(ar.call("get_passive_elem_resist", 7)) > 0.0, "artifact wind resist restored")
			var cult = p.call("get_cultivation")
			cult.call("set_mana", 200.0)
			_check(bool(ar.call("activate_slot", 4)), "secondary activation works after exit")
		18:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
