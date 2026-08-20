# 秘境压制修为：端到端测试
# ①设 realm=3 断言攻击/防御/生命/法力下降②恢复-1属性回归③进入古剑冢被压制④出房恢复⑤各压制值不串⑥max_health 也被压制
extends SceneTree

var _t := 0.0
var _next := 1.0
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

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _wait_player() -> bool:
	var p = _player()
	if p == null:
		_next = _t + 0.3
		return false
	return true

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3
	paused = false

	match _step:
		0:
			if not _wait_player():
				return false
			var p = _player()
			_breakthrough_to(0)
			var atk0 = p.call("get_effective_attack")
			var hp0 = p.call("get_max_health")
			var mana0 = _cult().call("get_max_mana")
			_breakthrough_to(3)
			var atk3_norm = p.call("get_effective_attack")
			var hp3_norm = p.call("get_max_health")
			var mana3_norm = _cult().call("get_max_mana")
			_check(atk3_norm > atk0, "金丹攻击 > 凡人（%.1f > %.1f）" % [atk3_norm, atk0])
			_check(hp3_norm > hp0, "金丹生命 > 凡人（%.1f > %.1f）" % [hp3_norm, hp0])
			_check(mana3_norm > mana0, "金丹法力 > 凡人（%.1f > %.1f）" % [mana3_norm, mana0])
			_step = 1

		1:
			var p = _player()
			_breakthrough_to(6) # 炼虚
			var atk6 = p.call("get_effective_attack")
			var hp6 = p.call("get_max_health")
			var mana6 = _cult().call("get_max_mana")
			# 压制到 realm 3
			p.set("suppressed_realm", 3)
			var atk3_sup = p.call("get_effective_attack")
			var hp3_sup = p.call("get_max_health")
			var mana3_sup = _cult().call("get_max_mana")
			_check(atk3_sup < atk6, "压制后攻击 < 炼虚（%.1f < %.1f）" % [atk3_sup, atk6])
			_check(hp3_sup < hp6, "压制后生命 < 炼虚（%.1f < %.1f）" % [hp3_sup, hp6])
			_check(mana3_sup < mana6, "压制后法力 < 炼虚（%.1f < %.1f）" % [mana3_sup, mana6])
			# 恢复 -1 → 回到炼虚水平
			p.set("suppressed_realm", -1)
			var atk6_rest = p.call("get_effective_attack")
			_check(abs(atk6_rest - atk6) < 0.1, "恢复后攻击回到炼虚（%.1f ≈ %.1f）" % [atk6_rest, atk6])
			_step = 2

		2:
			# 各压制值不串：压制 realm=3 vs realm=5 应有不同底数
			var p = _player()
			var atk6 = p.call("get_effective_attack")
			p.set("suppressed_realm", 3)
			var atk3 = p.call("get_effective_attack")
			p.set("suppressed_realm", 5) # 化神
			var atk5 = p.call("get_effective_attack")
			p.set("suppressed_realm", -1)
			_check(atk3 < atk5, "压制 realm3 攻击 < realm5（%.1f < %.1f）" % [atk3, atk5])
			_check(atk5 < atk6, "压制 realm5 攻击 < 不压制（%.1f < %.1f）" % [atk5, atk6])
			# 进入古剑冢（压制 realm=3）
			_breakthrough_to(10) # 真仙，确保高出门槛
			_step = 3

		3:
			var portal: Node = null
			for n in current_scene.get_children():
				var v = n.get("target_scene")
				if typeof(v) == TYPE_STRING or typeof(v) == TYPE_STRING_NAME:
					if String(v) == "res://scenes/rooms/gu_jian_zhong.tscn":
						portal = n
						break
			_check(portal != null, "古剑冢入口 Portal 存在")
			if portal:
				_player().global_position = Vector2(3650, 210)
			_next = _t + 0.6
			_step = 4

		4:
			Input.action_press("up")
			_next = _t + 0.3
			_step = 5

		5:
			Input.action_release("up")
			_next = _t + 0.6
			_step = 6

		6:
			var room = current_scene.get_node_or_null("GuJianZhong")
			_check(room != null, "古剑冢已挂载")
			var p = _player()
			_check(int(p.get("suppressed_realm")) == 3, "进入古剑冢后 suppressed_realm=3（实际=%d）" % int(p.get("suppressed_realm")))
			_player().position = Vector2(200, 220)
			_next = _t + 0.6
			_step = 7

		7:
			Input.action_press("up")
			_next = _t + 0.3
			_step = 8

		8:
			Input.action_release("up")
			_next = _t + 0.6
			_step = 9

		9:
			var p = _player()
			_check(int(p.get("suppressed_realm")) == -1, "出古剑冢后 suppressed_realm=-1（实际=%d）" % int(p.get("suppressed_realm")))
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false