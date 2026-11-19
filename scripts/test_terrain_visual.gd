# 地形材质 harness：12 种 tile 的 Atlas/UV/repeat + 主场景与公共 helper 接线。
extends SceneTree

var _t := 0.0
var _done := false
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
	if _t < 0.6 or _done:
		return false
	_done = true
	var terrain = load("res://scripts/art/terrain_visual.gd")
	var manifest: Dictionary = load("res://assets/textures/terrain/manifest.json").data
	var bad := ""
	for style in manifest["styles"]:
		var poly := Polygon2D.new()
		poly.polygon = PackedVector2Array([Vector2(-8, 0), Vector2(8, 0), Vector2(8, 24), Vector2(-8, 24)])
		if not bool(terrain.call("apply", poly, style)):
			bad = "%s apply 失败" % style
			break
		var tex = poly.texture
		var entry: Dictionary = manifest["styles"][style]
		var want = load("res://assets/textures/terrain/" + String(entry["texture"]))
		if not (tex is Texture2D) or tex != want or tex.get_size() != Vector2(16, 16) \
				or poly.texture_repeat != CanvasItem.TEXTURE_REPEAT_ENABLED or poly.uv.size() != 4 \
				or poly.uv[0] != Vector2(0, 0) or poly.uv[2] != Vector2(16, 24):
			bad = "%s 独立 tile/UV/repeat 错误" % style
			break
		poly.free()
	_check(bad.is_empty(), "12 种地形 tile 全部正确平铺" + ("" if bad.is_empty() else " → " + bad))
	var ground = _scene.get_node_or_null("GroundVisual")
	_check(ground != null and ground.texture is Texture2D and ground.uv.size() == ground.polygon.size(),
		"主场景手工 GroundVisual 已接材质且几何不变")
	var dynamic_ok := true
	var count := 0
	for child in _scene.get_children():
		if child is StaticBody2D and (String(child.name).begins_with("Ground_") or String(child.name).begins_with("Platform_")):
			var poly = child.get_node_or_null("Polygon2D")
			if poly == null or not (poly.texture is Texture2D) or poly.uv.size() != poly.polygon.size():
				dynamic_ok = false
			count += 1
	_check(dynamic_ok and count > 0, "公共 ground/platform 全部接材质（%d）" % count)
	var fallback := Polygon2D.new()
	fallback.polygon = PackedVector2Array([Vector2.ZERO, Vector2.ONE, Vector2(1, 0)])
	_check(not bool(terrain.call("apply", fallback, "__missing__")) and fallback.texture == null,
		"未知风格安全回退")
	fallback.free()
	print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
	return true
