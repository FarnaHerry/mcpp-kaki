extends CanvasLayer
## 丹房炼丹面板（洞天设施补全）：复刻 GameMenu 炼丹页卡片式布局，
## 复用 AlchemySystem get_recipe_list/craft，就地炼丹不用回 GameMenu。
## ↑/↓←/→ 选丹方  X 炼制  ESC/O 关闭；打开暂停，关闭还原原暂停状态。
## 节点名固定 PillLabPanel：GameMenu/DongtianManager 依名防抢 ESC/O。

const GRID_COLS := 3

var _open := false
var _restore_pause := false
var _player: Node = null
var _recipes: Array = []
var _grid: Control = null
var _detail: Label = null
var _mats: Label = null
var _msg: Label = null

func _ready():
	name = "PillLabPanel"
	layer = 115 # 同 StoragePanel
	process_mode = Node.PROCESS_MODE_ALWAYS

	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0, 0.78)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dim)

	var title := _line("—— 丹房 · 炼丹 ——", 185, 36, 13, Color(1.0, 0.9, 0.5))
	title.size.x = 140

	_grid = ClassDB.instantiate("GridList")
	_grid.position = Vector2(40, 66)
	_grid.size = Vector2(400, 84) # 3 行窗口
	add_child(_grid)
	_grid.call("set_columns", GRID_COLS)
	_grid.call("set_cell_size", Vector2(133, 28))

	_detail = _line("", 40, 158, 9, Color(1.0, 0.95, 0.6))
	_mats = _line("", 40, 172, 8, Color(0.45, 0.45, 0.45))
	_msg = _line("", 60, 244, 10, Color(0.6, 1.0, 0.6))

	visible = false
	set_process(true)

func _line(text: String, x: float, y: float, fsize: int, color: Color) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", fsize)
	l.add_theme_color_override("font_color", color)
	l.position = Vector2(x, y)
	add_child(l)
	return l

func is_open() -> bool:
	return _open

func open_panel(player: Node):
	_player = player
	_open = true
	_restore_pause = get_tree().paused
	get_tree().paused = true
	_msg.text = "" # 重置结果话术 → 回到操作提示
	visible = true
	_grid.call("set_selected", 0)
	_refresh()

func close_panel():
	if not _open:
		return
	_open = false
	visible = false
	get_tree().paused = _restore_pause # 嵌套暂停安全：还原原暂停状态

func _alchemy():
	return _player.call("get_alchemy") if _player else null

func _process(_d):
	if not _open:
		return
	if Input.is_action_just_pressed("menu") or Input.is_action_just_pressed("dongtian"):
		close_panel()
		return
	if _recipes.is_empty():
		return
	var moved := false
	if Input.is_action_just_pressed("left"):
		_grid.call("move_selection", -1, 0)
		moved = true
	elif Input.is_action_just_pressed("right"):
		_grid.call("move_selection", 1, 0)
		moved = true
	elif Input.is_action_just_pressed("up"):
		_grid.call("move_selection", 0, -1)
		moved = true
	elif Input.is_action_just_pressed("down"):
		_grid.call("move_selection", 0, 1)
		moved = true
	if moved:
		_refresh_detail()
	if Input.is_action_just_pressed("interact"):
		_craft_selected()

func _craft_selected():
	var al = _alchemy()
	if al == null:
		return
	var sel := int(_grid.call("get_selected"))
	if sel < 0 or sel >= _recipes.size():
		return
	var id = _recipes[sel]["id"]
	al.call("craft", id)
	var msg := String(al.call("get_last_message"))
	_msg.text = msg
	_msg.add_theme_color_override("font_color",
		Color(0.6, 1.0, 0.6) if msg.contains("炼成") else Color(1.0, 0.5, 0.5))
	_refresh() # 材料消耗后重算卡片配色（保留选中）

func _refresh():
	var al = _alchemy()
	if al == null:
		_recipes = []
		_grid.call("set_items", [])
		return
	_recipes = al.call("get_recipe_list")
	var items: Array = []
	for r in _recipes:
		var cell := {"text": String(r["name"])}
		if bool(r["realm_locked"]):
			cell["dim"] = true # 境界未达：灰显
			cell["color"] = Color(0.5, 0.5, 0.5, 1.0)
		elif bool(r["can_craft"]):
			cell["color"] = Color(0.6, 1.0, 0.6) # 材料齐：绿
		else:
			cell["color"] = Color(1.0, 0.5, 0.5) # 材料不足：红
		items.append(cell)
	_grid.call("set_items", items)
	_refresh_detail()

func _refresh_detail():
	if _recipes.is_empty():
		_detail.text = "（丹炉未备）"
		_mats.text = ""
		return
	var sel := clampi(int(_grid.call("get_selected")), 0, _recipes.size() - 1)
	var r: Dictionary = _recipes[sel]
	_detail.text = "%s  %s" % [String(r["name"]), String(r["effect"])]
	if bool(r["realm_locked"]):
		_detail.text += "  （金丹起）"
	var mat_line := "材料 "
	var mats: Array = r["mats"]
	for j in mats.size():
		var m: Dictionary = mats[j]
		if j > 0:
			mat_line += " + "
		mat_line += "%s×%d(%d)" % [String(m["name"]), int(m["need"]), int(m["have"])]
	_mats.text = mat_line
	_mats.add_theme_color_override("font_color",
		Color(0.45, 0.45, 0.45) if bool(r["can_craft"]) else Color(1.0, 0.5, 0.5))
	if _msg.text.is_empty():
		_msg.text = "↑/↓←/→ 选丹方  X 炼制  ESC/O 关闭。炼制亦修行：每炉喂练气 +5。"
		_msg.add_theme_color_override("font_color", Color(0.45, 0.45, 0.45))
