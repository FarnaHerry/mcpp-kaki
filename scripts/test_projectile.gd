# 验证投射物: 玩家在弓手射程内站桩, 统计场上 Projectile 数与玩家受伤
extends SceneTree

var _t := 0.0
var _player_hits := 0
var _started := false
var _last_report := -1

func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene
	print("[TEST] main scene loaded")

func _on_dmg(pos, amount, is_player_victim):
	if is_player_victim:
		_player_hits += 1
		print("[TEST] player hit for ", amount)

func _process(delta) -> bool:
	_t += delta
	if not _started and _t > 0.5:
		_started = true
		var p = root.find_child("Player", true, false)
		p.position = Vector2(600, 200) # 弓手(750) 左侧150: 超过 too_close(80), 在 attack_range 内
		var bus = root.find_child("SignalBus", true, false)
		bus.connect("damage_dealt", _on_dmg)
		print("[TEST] player placed")
	if _started:
		var sec := int(_t)
		if sec != _last_report and sec % 2 == 0:
			_last_report = sec
			var projs = current_scene.find_children("*", "Projectile", true, false)
			print("[TEST] t=%d projectiles=%d" % [sec, projs.size()])
	if _t > 10.0:
		print("[TEST] player projectile hits: ", _player_hits)
		return true
	return false
