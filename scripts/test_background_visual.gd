# 背景层 harness：主场景 / 12 套资源 / 真实 Portal 进房远景。
extends SceneTree

var _t := 0.0
var _step := 0
var _next := 0.6
var _pass := 0
var _fail := 0
var _scene: Node = null


func _initialize():
	_scene = load("res://scenes/main.tscn").instantiate()
	root.add_child(_scene)
	current_scene = _scene


func _check(cond: bool, msg: String) -> void:
	if cond:
		_pass += 1
		print("[PASS] ", msg)
	else:
		_fail += 1
		print("[FAIL] ", msg)


func _process(delta: float) -> bool:
	_t += delta
	if _t < _next:
		return false
	match _step:
		0:
			_next = _t + 0.2
			var bg = _scene.get_node_or_null("WorldBackground")
			_check(bg != null and bg is CanvasLayer and bg.layer == -100 and bg.get_meta("style") == "dongsheng",
				"主场景挂载东胜远景层且位于世界后方")
			var manifest: Dictionary = load("res://assets/textures/backgrounds/manifest.json").data
			var bad := ""
			for style in manifest["styles"]:
				var host := Node2D.new()
				_scene.add_child(host)
				var layer = load("res://scripts/art/background_visual.gd").call("attach", host, style)
				var image = layer.get_node_or_null("BackgroundImage") if layer != null else null
				if image == null or not (image.texture is Texture2D) or image.texture.get_size() != Vector2(480, 270) \
						or image.mouse_filter != Control.MOUSE_FILTER_IGNORE:
					bad = style
					break
				host.queue_free()
			_check(bad.is_empty(), "12 套 480×270 背景全部可挂载且不拦截输入" + ("" if bad.is_empty() else " → " + bad))
			_step = 1
		1:
			var portal = null
			for p in _scene.find_children("*", "Portal", true, false):
				if not String(p.get("target_scene")).is_empty() and p.get_parent() == _scene:
					portal = p
					break
			_check(portal != null, "找到主场景真实 Portal")
			if portal == null:
				print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
				return true
			portal.call("trigger")
			_next = _t + 0.25
			_step = 2
		2:
			var player = root.find_child("Player", true, false)
			var room = player.get_parent()
			_check(room != _scene and room.get_node_or_null("WorldBackground") != null,
				"真实 Portal 进房后房间自动获得背景层")
			print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
			return true
	return false
