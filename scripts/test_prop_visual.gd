# 设施素材 harness：10 类 prop、占位清理、定制门 opt-out、云游阵激活态、面板保留。
extends SceneTree

var _t := 0.0
var _next := 0.6
var _step := 0
var _pass := 0
var _fail := 0
var _scene: Node = null
var _nodes: Array = []
var _tp: Node = null
var _portal: Node = null
var _portal_frame := 0


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


func _spawn(script_path: String, prop_id: String, clear := false) -> Node:
	var n = load(script_path).new()
	n.position = Vector2(13000 + _nodes.size() * 60, -200)
	_scene.add_child(n)
	_nodes.append(n)
	n.set_meta("want_prop", prop_id)
	n.set_meta("want_clear", clear)
	return n


func _process(delta: float) -> bool:
	_t += delta
	if _t < _next:
		return false
	match _step:
		0:
			_next = _t + 0.25
			_spawn_all()
			_step = 1
		1:
			_next = _t + 0.25
			_check_all()
			var gm = _scene.get_node_or_null("GameManager")
			gm.call("set_flag", "tp:art_test", true)
			_step = 2
		2:
			var vis = _tp.get_node_or_null("PropSprite")
			_check(vis != null and String(vis.call("get_visual_state")) == "active", "云游阵铭刻后自动切 active 动画")
			var pv = _portal.get_node_or_null("PropSprite")
			_check(pv != null and int(pv.call("get_frame_index")) != _portal_frame, "传送门动画实际推进")
			print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
			for n in _nodes:
				if is_instance_valid(n):
					n.queue_free()
			return true
	return false


func _spawn_all() -> void:
	_portal = ClassDB.instantiate("Portal")
	_portal.position = Vector2(13000, -200)
	_scene.add_child(_portal)
	_nodes.append(_portal)
	_portal.set_meta("want_prop", "portal")

	var custom = ClassDB.instantiate("Portal")
	custom.set_meta("prop_visual_disabled", true)
	_scene.add_child(custom)
	_check(custom.get_node_or_null("PropSprite") == null, "定制门 opt-out 不叠通用素材")
	custom.queue_free()

	var chest = ClassDB.instantiate("StorageChest")
	_scene.add_child(chest)
	_nodes.append(chest)
	chest.set_meta("want_prop", "storage_chest")
	chest.set_meta("want_clear", true)

	_tp = ClassDB.instantiate("TeleportArray")
	_scene.add_child(_tp)
	_tp.call("setup", "art_test", "测试阵", "dongsheng", "res://scenes/main.tscn")
	_nodes.append(_tp)
	_tp.set_meta("want_prop", "teleport_array")
	_tp.set_meta("want_clear", true)

	_spawn("res://scripts/spots/refine_spot.gd", "refine_spot")
	_spawn("res://scripts/spots/meditate_spot.gd", "meditate_spot")
	_spawn("res://scripts/spots/pill_lab.gd", "pill_lab", true)
	_spawn("res://scripts/spots/expand_monument.gd", "expand_monument", true)
	_spawn("res://scripts/spots/jlz_eye.gd", "jlz_eye")
	var gate = _spawn("res://scripts/gates/scene_gate.gd", "scene_gate", true)
	var old := Polygon2D.new()
	old.polygon = PackedVector2Array([Vector2(-6, -18), Vector2(6, -18), Vector2(6, 18), Vector2(-6, 18)])
	gate.add_child(old) # 模拟调用方在 _ready 后补占位，deferred 挂载应清掉
	_spawn("res://scripts/gates/cond_portal.gd", "cond_portal", true)


func _check_all() -> void:
	var sheet = load("res://assets/textures/props/prop_sheet.png")
	var bad := ""
	for n in _nodes:
		var vis = n.get_node_or_null("PropSprite")
		if vis == null or String(vis.call("get_prop_id")) != String(n.get_meta("want_prop")) \
				or not (vis.texture is AtlasTexture) or vis.texture.atlas != sheet:
			bad = "%s 缺失/图集错误" % n.get_meta("want_prop")
			break
		if bool(n.get_meta("want_clear", false)):
			for child in n.get_children():
				if child is Polygon2D and not child.polygon.is_empty():
					bad = "%s 占位未清" % n.get_meta("want_prop")
					break
	_check(bad.is_empty(), "10 类设施全部挂载原图集，指定占位全部清理" + ("" if bad.is_empty() else " → " + bad))
	var tp_vis = _tp.get_node_or_null("PropSprite")
	_check(tp_vis != null and String(tp_vis.call("get_visual_state")) == "inactive", "云游阵初始为 inactive")
	_check(_tp.get_node_or_null("TpName") != null, "云游阵重建保留 TpName 节点名")
	var pill = _nodes.filter(func(n): return n.get_meta("want_prop", "") == "pill_lab")[0]
	_check(pill.get_node_or_null("PillLabPanel") != null, "丹炉换素材未删除 PillLabPanel")
	var main_portals = _scene.find_children("*", "Portal", true, false)
	var covered := 0
	for p in main_portals:
		if p.get_node_or_null("PropSprite") != null:
			covered += 1
	_check(covered >= 2, "主场景既有 Portal 均接入素材（%d）" % covered)
	_portal_frame = int(_portal.get_node("PropSprite").call("get_frame_index"))
