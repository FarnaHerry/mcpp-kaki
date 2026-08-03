# 验证: ①法宝获得/装配/栏位上限 ②本命槽与Player本命同步 ③祭出(耗灵+投射物+冷却+温养)
#      ④B键切页 ⑤境界授予(筑基飞剑) ⑥存档往返
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
			_check(p != null, "player exists")
			var ar = p.call("get_artifacts")
			_check(ar != null, "artifact system exists")
			_check(int(ar.call("get_slot_limit")) == 3, "slot limit 3 pre-ascension")
			# 获得并装配飞剑到次要槽1
			ar.call("acquire", "fei_jian")
			_check(bool(ar.call("equip", 1, "fei_jian")), "equip fei_jian to slot 1")
			_check(not bool(ar.call("equip", 4, "fei_jian")), "slot 4 locked pre-ascension")
			var info = ar.call("get_slot_info", 1)
			print("[TEST] slot1: ", info.get("id"), " coeff=", info.get("coeff"))
			_check(String(info.get("id")) == "fei_jian", "slot 1 = fei_jian")
			_check(abs(float(info.get("coeff")) - 1.0) < 0.001, "secondary coeff starts 1.0")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var ar = p.call("get_artifacts")
			# 本命槽同步 Player 本命
			ar.call("acquire", "zhao_yao_hu")
			ar.call("equip", 0, "zhao_yao_hu")
			_check(String(p.call("get_benming_artifact")) == "zhao_yao_hu", "slot 0 syncs benming")
			var info0 = ar.call("get_slot_info", 0)
			print("[TEST] benming coeff=", info0.get("coeff"))
			_check(float(info0.get("coeff")) >= 1.19, "benming coeff >= 1.2")
		3:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			var ar = p.call("get_artifacts")
			# 凡人期灵力上限为 0：先突破炼气开灵力池
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 1000000000)
			if int(cult.call("get_realm_index")) < 1:
				cult.call("attempt_breakthrough")
			cult.call("set_mana", 100.0)
			var mana0 = float(cult.call("get_mana"))
			var proj0 = root.find_children("*", "Projectile", true, false).size()
			var ok = bool(ar.call("activate_slot", 1)) # 飞剑祭出
			var mana1 = float(cult.call("get_mana"))
			var proj1 = root.find_children("*", "Projectile", true, false).size()
			print("[TEST] activate fei_jian=", ok, " mana ", mana0, "->", mana1, " proj ", proj0, "->", proj1)
			_check(ok, "artifact activation succeeds")
			_check(mana1 < mana0, "activation consumed mana")
			_check(proj1 > proj0, "fei_jian projectile spawned")
			_check(not bool(ar.call("activate_slot", 1)), "artifact cooldown blocks recast")
			var info = ar.call("get_slot_info", 1)
			_check(float(info.get("nurture")) >= 10.0, "activation nurtures artifact")
		4:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			# B 键切页
			_check(int(p.call("get_skill_page")) == 0, "page starts at combat")
			p.call("toggle_skill_page")
			_check(int(p.call("get_skill_page")) == 1, "B toggles to artifact page")
			p.call("toggle_skill_page")
			_check(int(p.call("get_skill_page")) == 0, "B toggles back")
		5:
			# 突破到筑基 → 飞剑法宝授予（幂等：已 acquire 过）
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			var cult = p.call("get_cultivation")
			cult.call("set_free_breakthrough", true)
			cult.call("accumulate_energy", 1000000000)
			while int(cult.call("get_realm_index")) < 2:
				cult.call("attempt_breakthrough")
			_check(int(cult.call("get_realm_index")) >= 2, "reached FOUNDATION")
			var ar = p.call("get_artifacts")
			_check(bool(ar.call("is_owned", "fei_jian")), "fei_jian owned at FOUNDATION")
		6:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var ar = p.call("get_artifacts")
			var d = ar.call("save_to_dict")
			ar.call("load_from_dict", d)
			var info = ar.call("get_slot_info", 1)
			_check(String(info.get("id")) == "fei_jian", "save/load slot roundtrip")
			_check(float(info.get("nurture")) >= 10.0, "save/load nurture roundtrip")
			_check(String(p.call("get_benming_artifact")) == "zhao_yao_hu", "benming survives artifacts load")
		7:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
