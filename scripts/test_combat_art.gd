# 战斗素材 harness：8 类投射物 + 主角攻击弧光的播放/结束/朝向。
extends SceneTree

var _t := 0.0
var _pass := 0
var _fail := 0
var _done := false


func _initialize():
	var scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(scene)
	current_scene = scene


func _check(cond: bool, msg: String) -> void:
	if cond:
		_pass += 1
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)


func _process(delta: float) -> bool:
	_t += delta
	if _t < 0.6 or _done:
		return false
	_done = true
	var manifest: Dictionary = load("res://assets/textures/projectiles/manifest.json").data
	var sheet = load("res://assets/textures/projectiles/projectile_sheet.png")
	var bad := ""
	for element in range(8):
		var p = ClassDB.instantiate("Projectile")
		p.set("element", element)
		p.position = Vector2(14000 + element * 30, -200)
		current_scene.add_child(p)
		p.set_physics_process(false)
		var ph = p.get_node_or_null("ProjVisual")
		var vis = ph.get_node_or_null("ProjectileSprite") if ph != null else null
		var info: Dictionary = manifest["elements"][str(element)]
		if vis == null or not ph.polygon.is_empty() or int(vis.call("get_element")) != element \
				or vis.texture.atlas != sheet or vis.texture.region != Rect2(0, int(info["row"]) * 16, 16, 16):
			bad = "element %d" % element
			break
		p.queue_free()
	_check(bad.is_empty(), "物理 + 7 元素弹体全部挂载正确图集" + ("" if bad.is_empty() else " → " + bad))
	var player = root.find_child("Player", true, false)
	var pv = player.get_node_or_null("PlayerVisual")
	var arc = pv.get_node_or_null("AttackArc")
	_check(arc != null and not arc.visible, "攻击弧光已挂载且待机隐藏")
	pv.call("_play", "attack")
	_check(arc != null and arc.visible and bool(arc.call("is_playing")), "attack 状态启动剑气")
	arc.call("set_facing", -1)
	_check(arc.flip_h and arc.position.x < 0, "剑气随主角朝左翻转")
	arc.call("_process", 1.0)
	_check(not arc.visible and not bool(arc.call("is_playing")), "非循环剑气末帧后自动隐藏")
	print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
	return true
