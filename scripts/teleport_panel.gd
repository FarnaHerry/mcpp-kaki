extends CanvasLayer
## 云游阵·驾云面板：列出 data/teleports.json 全部阵点。
## 已铭刻（走近激活，GameManager flag "tp:<id>"）才可选；跨洲阵点沿用境界门控
## （ContinentManager.can_travel，锁定灰显 gate 话术）。↑/↓ 选阵  X 驾云  ESC 关闭。
## 同洲落阵点坐标；跨洲走 travel_to_direct_to（存档桥+落阵点）。打开暂停，关闭还原。

var _open := false
var _restore_pause := false
var _player: Node = null
var _entries: Array = [] # {d:Dictionary, activated:bool, unlocked:bool, gate:String}
var _sel := 0
var _list: Label = null
var _msg: Label = null
var _msg_t := 0.0
var _opened_frame := -1 # 同帧守卫：阵碑 X 开面板的同一帧，面板 _process 不再消费 interact

func _ready():
	name = "TeleportPanel"
	layer = 115 # 同 StoragePanel/PillLabPanel
	process_mode = Node.PROCESS_MODE_ALWAYS

	var dim := ColorRect.new()
	dim.color = Color(0, 0, 0.05, 0.82)
	dim.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(dim)

	var title := _line("—— 云游阵 · 驾云 ——", 165, 36, 13, Color(1.0, 0.9, 0.5))
	title.size.x = 160
	_line("身外阵碑铭刻于心，一念之间驾云而至。", 110, 56, 8, Color(0.55, 0.65, 0.75))

	_list = _line("", 60, 76, 9, Color(0.85, 0.85, 0.85))
	_list.size = Vector2(360, 150)

	_msg = _line("", 60, 240, 9, Color(0.6, 1.0, 0.6))
	_line("↑/↓ 选阵  X 驾云  ESC 关闭", 150, 256, 8, Color(0.5, 0.5, 0.5))

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
	_opened_frame = Engine.get_process_frames()
	_restore_pause = get_tree().paused
	get_tree().paused = true
	_sel = 0
	_refresh()
	visible = true

func close_panel():
	_open = false
	get_tree().paused = _restore_pause # 嵌套暂停安全
	visible = false

func _refresh():
	_entries.clear()
	var scene = get_tree().current_scene
	var dl = scene.find_child("DataLoader", true, false)
	var gm = scene.find_child("GameManager", true, false)
	var cm = scene.find_child("ContinentManager", true, false)
	if dl == null or gm == null or cm == null:
		return
	var cur_scene := str(scene.scene_file_path)
	var gates := {} # continent_id -> gate 话术
	for c in cm.call("get_continent_list"):
		gates[str(c["id"])] = str(c["gate"])
	for d in dl.call("get_all_teleports"):
		var activated: bool = bool(gm.call("has_flag", "tp:" + str(d["id"])))
		var same_continent: bool = str(d["scene"]) == cur_scene
		var unlocked: bool = same_continent or bool(cm.call("can_travel", str(d["continent"])))
		_entries.append({
			"d": d, "activated": activated, "unlocked": unlocked,
			"gate": gates.get(str(d["continent"]), ""),
			"same": same_continent,
		})
	_sel = clampi(_sel, 0, max(0, _entries.size() - 1))
	var lines := ""
	for i in range(_entries.size()):
		var e = _entries[i]
		var row := ""
		if not e["activated"]:
			row += "  ？？？ · 未铭刻的阵点"
		else:
			row += ("▶ " if i == _sel else "  ") + str(e["d"]["name"])
			if e["same"]:
				row += "（本洲）"
			elif e["unlocked"]:
				row += "（驾云可至）"
			else:
				row += "（%s）" % e["gate"]
		lines += row + "\n"
	_list.text = lines

func _flash(msg: String, bad := false):
	_msg.text = msg
	_msg.add_theme_color_override("font_color", Color(1.0, 0.5, 0.5) if bad else Color(0.6, 1.0, 0.6))
	_msg_t = 2.0

func _process(delta):
	if _msg_t > 0.0:
		_msg_t -= delta
		if _msg_t <= 0.0:
			_msg.text = ""
	if not _open:
		return
	if int(Engine.get_process_frames()) == _opened_frame:
		return # 开面板那一帧的 interact 已被阵碑消费，不再当「驾云」
	# ESC=menu 动作（GameMenu/DongtianManager 对 TeleportPanel 有防抢守卫）
	if Input.is_action_just_pressed("menu"):
		close_panel()
	elif Input.is_action_just_pressed("down"):
		if _entries.size() > 0:
			_sel = (_sel + 1) % _entries.size()
			_refresh()
	elif Input.is_action_just_pressed("up"):
		if _entries.size() > 0:
			_sel = (_sel - 1 + _entries.size()) % _entries.size()
			_refresh()
	elif Input.is_action_just_pressed("interact"):
		_travel()

func _travel():
	if _entries.is_empty():
		return
	var e = _entries[_sel]
	if not e["activated"]:
		_flash("未铭刻的阵点，走近阵碑方可铭刻。", true)
		return
	if not e["unlocked"]:
		_flash(str(e["d"]["name"]) + " 境界不足：" + str(e["gate"]), true)
		return
	var d = e["d"]
	close_panel()
	var scene = get_tree().current_scene
	if e["same"]:
		var pl = scene.find_child("Player", true, false)
		if pl:
			pl.global_position = Vector2(float(d["x"]), float(d["y"]))
		var bus = scene.find_child("SignalBus", true, false)
		if bus:
			bus.emit_signal("interaction_prompt", "驾云而至 · " + str(d["name"]), true)
	else:
		var cm = scene.find_child("ContinentManager", true, false)
		if cm:
			cm.call("travel_to_direct_to", str(d["continent"]), float(d["x"]), float(d["y"]))
