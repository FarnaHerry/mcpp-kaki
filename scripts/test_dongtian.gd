# 洞天 v1 harness:
# ①凡人按 O 拒入（需炼虚期提示）②炼虚解锁后按 O 进入（玩家入洞天场景+相机锁定+横幅）
# ③洞内按 O 退出回到进入位置 ④战斗中按 O 拒入 ⑤洞天内存档→读档落在返回点且已退出洞天
extends SceneTree

var _t := 0.0
var _next := 1.0
var _step := 0
var _fail := 0
var _entry_pos := Vector2.ZERO
var _enemy = null

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
	Input.action_release(action)

func _player():
	return root.find_child("Player", true, false)

func _cult():
	return _player().call("get_cultivation")

func _gm():
	return root.find_child("GameManager", true, false)

func _dt():
	return root.find_child("DongtianManager", true, false)

func _hud_has(sub: String) -> bool:
	var hud = root.find_child("GameHUD", true, false)
	if hud == null:
		return false
	for l in hud.find_children("*", "Label", true, false):
		if sub in l.text and l.visible:
			return true
	return false

func _breakthrough_to(realm: int):
	_cult().call("set_free_breakthrough", true)
	_cult().call("accumulate_energy", 100000000000)
	while int(_cult().call("get_realm_index")) < realm:
		_cult().call("attempt_breakthrough")
	_cult().call("set_free_breakthrough", false)

func _clear_enemies():
	for e in get_nodes_in_group("enemies"):
		e.queue_free()

func _process(delta) -> bool:
	_t += delta
	if _t < _next:
		return false
	_next = _t + 0.3

	match _step:
		0:
			_clear_enemies() # 防出生地敌人仇恨干扰（同 test_travel）
			# 凡人按 O → 拒入
			_press("dongtian")
			_step = 1
		1:
			_check(_dt().call("is_inside") == false, "凡人未进入洞天")
			_check(_hud_has("需炼虚期"), "拒入提示：需炼虚期")
			_breakthrough_to(6) # 炼虚
			_check(int(_cult().call("get_realm_index")) == 6, "突破到炼虚")
			_step = 2
		2:
			_entry_pos = _player().global_position
			_press("dongtian")
			_step = 3
		3:
			_check(_dt().call("is_inside") == true, "炼虚按 O 进入洞天")
			_check(_player().get_parent().name == "Dongtian", "玩家已挂载进洞天场景")
			_check(_hud_has("—— 洞天"), "进入横幅：洞天")
			var cam = root.find_child("CameraRoom2D", true, false)
			_check(cam != null and cam.get_mode() == 1, "相机锁定 ROOM_LOCKED")
			_step = 4
		4:
			_press("dongtian")
			_step = 5
		5:
			_check(_dt().call("is_inside") == false, "洞内按 O 退出洞天")
			_check(_player().get_parent() == current_scene, "玩家回到主场景根")
			_check(_player().global_position.distance_to(_entry_pos) < 2.0, "退出回到进入位置")
			_step = 6
		6:
			# 战斗中拒入：生成敌人并把玩家传送到它脸上触发追击
			var WC = preload("res://scripts/world_common.gd")
			_enemy = WC.spawn_enemy(current_scene, _player().global_position + Vector2(60, 0), Color(0.8, 0.3, 0.3), 60.0, 200.0, "DongtianTestEnemy")
			if _enemy:
				_player().global_position = _enemy.global_position
			_next = _t + 0.8
			_step = 7
		7:
			_press("dongtian")
			_step = 8
		8:
			if _enemy:
				_check(_dt().call("is_inside") == false, "战斗中未进入洞天")
				_check(_hud_has("战斗中无法进入洞天"), "拒入提示：战斗中")
			else:
				print("[SKIP] 场景无敌人，战斗拒入未验证")
			_player().global_position = _entry_pos
			_clear_enemies()
			_next = _t + 0.5
			_step = 9
		9:
			# 洞天内存档 → 读档应退出洞天并落返回点
			_press("dongtian")
			_step = 10
		10:
			_check(_dt().call("is_inside") == true, "再次进入洞天（存档测试）")
			_gm().call("save_game", "auto")
			_gm().call("load_game", "auto")
			_step = 11
		11:
			_check(_dt().call("is_inside") == false, "读档后已退出洞天")
			_check(_player().get_parent() == current_scene, "读档后玩家在主场景根")
			_check(_player().global_position.distance_to(_entry_pos) < 2.0, "读档落在返回点")
			if _fail == 0:
				print("[TEST] ALL PASS")
			else:
				print("[TEST] ", _fail, " FAILURES")
			return true
	return false
