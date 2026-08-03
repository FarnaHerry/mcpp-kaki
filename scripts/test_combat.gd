# 验证: ①连续攻击多次命中(第一下之后仍有伤害) ②damage_dealt 信号 ③ESC 设置面板
extends SceneTree

var _t := 0.0
var _next := 0.0
var _step := 0
var _hits := 0
var _player_hits := 0

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _press(action: String):
	Input.action_press(action)
	Input.action_release(action)

var _last_hb := ""
func _process(delta) -> bool:
	_t += delta
	# 每帧采样 HitBox 状态变化
	var p0 = root.find_child("Player", true, false)
	if p0 != null:
		var hb = p0.get_node_or_null("HitBox")
		if hb != null:
			var st = "mon=%s able=%s scale=%s" % [hb.monitoring, hb.monitorable, hb.scale.x]
			if st != _last_hb:
				_last_hb = st
				print("[TEST] t=%.2f HB " % _t, st)
	if _t < _next:
		return false

	match _step:
		0: # 传送到厚血敌人旁，接管血量，挂监听
			_next = _t + 0.5
			var p = root.find_child("Player", true, false)
			p.position = Vector2(480, 200)
			var e = _find_enemy()
			if e == null:
				print("[TEST] FAIL: enemy not found")
				return true
			e.call("take_damage", 0.0, null) # 触发一下确认方法可用
			# current_health 无属性, 直接用 bound getter 观测; 加血靠反复打
			e.set("current_health", 500.0)
			var bus = root.find_child("SignalBus", true, false)
			bus.connect("damage_dealt", _on_dmg)
			print("[TEST] setup done, enemy hp=", e.call("get_current_health"))
			_step = 1
		1, 2, 3, 4, 5, 6, 7, 8: # 连打 8 下
			_next = _t + 0.4
			var p = root.find_child("Player", true, false)
			var e = _find_enemy()
			print("[TEST] atk step ", _step, " ppos=", p.position, " epos=", e.position if e else "dead", " ehp=", e.call("get_current_health") if e else "-")
			_press("attack")
			_step += 1
		9:
			_next = _t + 0.5
			print("[TEST] damage events on enemy: ", _hits, " (expect >1, ideally 8)")
			# ESC 打开设置
			_press("menu")
			_step = 10
		10:
			_next = _t + 0.3
			print("[TEST] settings open, paused=", paused, " (expect true)")
			# 再按 ESC 关闭
			_press("menu")
			_step = 11
		11:
			print("[TEST] settings closed, paused=", paused, " (expect false)")
			print("[TEST] DONE: enemy_hits=", _hits, " player_hits=", _player_hits)
			return true
	return false

func _find_enemy():
	for c in current_scene.get_children():
		if c.get_class() == "Enemy" or c.is_class("CharacterBody2D") and c.name != "Player":
			if str(c.name).begins_with("Enemy") or c.get_class() == "Enemy":
				if c.position.x > 450 and c.position.x < 550:
					return c
	return null

func _on_dmg(pos, amount, is_player_victim):
	if is_player_victim:
		_player_hits += 1
	else:
		_hits += 1
		print("[TEST] hit enemy for ", amount)
