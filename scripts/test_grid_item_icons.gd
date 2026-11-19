# GridList 物品图标 harness：49 图标全显示 + 背包实际格子 + 非物品回退。
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
	var grid = ClassDB.instantiate("GridList")
	grid.position = Vector2(600, 300)
	grid.size = Vector2(140, 140)
	_scene.add_child(grid)
	grid.call("set_columns", 7)
	grid.call("set_cell_size", Vector2(20, 20))
	var entries: Array = []
	for def in JSON.parse_string(FileAccess.get_file_as_string("res://data/items.json")):
		entries.append({"text": def["name"], "item_id": def["id"]})
	grid.call("set_items", entries)
	var bad := ""
	for i in range(49):
		var frame = grid.get_child(i)
		var icon = frame.find_child("Icon", true, false)
		if icon == null or not icon.visible or not (icon.texture is Texture2D) or icon.texture.get_size() != Vector2(16, 16):
			bad = "cell %d" % i
			break
	_check(bad.is_empty(), "GridList 49 个物品格全部显示独立 16×16 图标" + ("" if bad.is_empty() else " → " + bad))

	grid.call("set_items", [{"text": "纯文字技能"}])
	var text_frame = grid.get_child(0)
	var text_icon = text_frame.find_child("Icon", true, false)
	_check(text_icon != null and not text_icon.visible, "无 item_id 条目保持纯文字回退")
	grid.queue_free()

	var panel = _scene.find_child("InventoryPanel", true, false)
	panel.call("toggle")
	var inv_grid = panel.find_child("ItemGrid", true, false)
	var found := false
	for frame in inv_grid.get_children():
		var icon = frame.find_child("Icon", true, false)
		if icon != null and icon.visible and icon.texture is Texture2D:
			found = true
			break
	_check(found, "实际背包物品格已显示图标")
	panel.call("toggle")
	print("[TEST] DONE: pass=%d fail=%d" % [_pass, _fail])
	return true
