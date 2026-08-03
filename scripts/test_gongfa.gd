# 验证: ①功法装配/熟练喂养/层数提升 ②分系喂养(主100%/副20%) ③加成乘区 ④存档往返 ⑤切换保留熟练
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
			var g = p.call("get_gongfa")
			_check(g != null, "gongfa system exists")
			# 装配两门初始功法（不依赖境界 grant 时机）
			g.call("equip_gongfa", "mang_niu_jin")
			g.call("equip_gongfa", "tu_na_jue")
			var b = g.call("get_slot_info", 0)
			var q = g.call("get_slot_info", 1)
			print("[TEST] body: ", b, " qi: ", q)
			_check(String(b.get("id")) == "mang_niu_jin", "body slot = mang_niu_jin")
			_check(String(q.get("id")) == "tu_na_jue", "qi slot = tu_na_jue")
		2:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var g = p.call("get_gongfa")
			# 喂养炼体 250 → 层1需100, 层2需200 → 升到2层余50；练气副系 +50
			g.call("feed", 0, 250.0)
			var b = g.call("get_slot_info", 0)
			var q = g.call("get_slot_info", 1)
			print("[TEST] after feed: body layer=", b.get("layer"), " prof=", b.get("prof"),
				" qi prof=", q.get("prof"))
			_check(int(b.get("layer")) == 2, "body layer-up to 2")
			_check(abs(float(b.get("prof")) - 150.0) < 0.01, "body prof remainder 150")
			_check(abs(float(q.get("prof")) - 50.0) < 0.01, "qi side-feed 20% = 50")
			# 加成: 莽牛劲 2层 → hp 1+0.04*2=1.08
			var hp = float(g.call("get_hp_mult"))
			print("[TEST] hp_mult=", hp)
			_check(abs(hp - 1.08) < 0.001, "hp_mult 1.08 at layer 2")
		3:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var g = p.call("get_gongfa")
			# 切换: 铁布衫 → 莽牛劲熟练入 known；切回 → 恢复2层50
			g.call("equip_gongfa", "tie_bu_shan")
			var b = g.call("get_slot_info", 0)
			_check(String(b.get("id")) == "tie_bu_shan" and int(b.get("layer")) == 1, "switch to tie_bu_shan layer 1")
			g.call("equip_gongfa", "mang_niu_jin")
			b = g.call("get_slot_info", 0)
			print("[TEST] switch back: layer=", b.get("layer"), " prof=", b.get("prof"))
			_check(int(b.get("layer")) == 2 and abs(float(b.get("prof")) - 150.0) < 0.01, "switch preserves proficiency")
		4:
			_next = _t + 0.3
			var p = root.find_child("Player", true, false)
			var g = p.call("get_gongfa")
			# 存档往返
			var d = g.call("save_to_dict")
			g.call("load_from_dict", d)
			var b = g.call("get_slot_info", 0)
			var q = g.call("get_slot_info", 1)
			_check(int(b.get("layer")) == 2 and abs(float(b.get("prof")) - 150.0) < 0.01, "save/load body roundtrip")
			_check(String(q.get("id")) == "tu_na_jue", "save/load qi roundtrip")
		5:
			print("[TEST] DONE fail=", _fail)
			return true
	return false
