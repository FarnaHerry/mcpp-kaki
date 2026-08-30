extends Area2D
## 药童（洞天 v5 经营）：贴近 [X] 托付灵田——委托后药童自动收获成熟地块
## （收获物入洞天仓库，仓库满则留在地块；玩家不在洞天也照看），再按 X 收回委托。
## 委托状态自持于 DongtianManager 随档持久化。
## 台词气泡 TownNpc 模式（X 触发，2.5s 自消，循环播放）。
## StorageChest/JlzEye 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
## 用法：dongtian.gd 运行时创建（x=452 灵泉右侧远角——
## 出生点 (240,200) 的 48px 幽灵 enter 守卫半径内不得放交互物，见 dongtian.gd 注释）。
const LINES_HIRED := ["灵田交给我，药师放心。", "灵植成熟，我自会采收入库。"]
const LINES_IDLE := ["需要我照看灵田吗？", "洞天虽好，莫忘修行。"]

var _player_inside := false
var _player: Node = null
var _refresh_t := 0.0
var _bubble_t := 0.0
var _line_idx := 0
var _bubble: Label = null
var _bubble_bg: ColorRect = null

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(28, 46)
	shape.shape = rect
	shape.position = Vector2(0, -6)
	add_child(shape)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	_build_visuals()
	set_process(true)

func _build_visuals():
	# 药童视觉：青布短褂（梯形）+ 头 + 双髻
	var robe = Polygon2D.new()
	robe.name = "Robe"
	robe.color = Color(0.35, 0.55, 0.42, 1)
	robe.polygon = PackedVector2Array([-7, -8, 7, -8, 9, 12, -9, 12])
	add_child(robe)
	var head = Polygon2D.new()
	head.name = "Head"
	head.color = Color(0.92, 0.78, 0.62, 1)
	var hp := PackedVector2Array()
	for i in 8:
		var a := TAU * i / 8.0
		hp.append(Vector2(cos(a) * 5.0, sin(a) * 5.0 - 13.0))
	head.polygon = hp
	add_child(head)
	var hair = Polygon2D.new()
	hair.name = "Hair"
	hair.color = Color(0.16, 0.12, 0.1, 1)
	hair.polygon = PackedVector2Array([-5, -19, -2, -19, -4, -23, 2, -19, 5, -19, 4, -23])
	add_child(hair)
	var name_label = Label.new()
	name_label.name = "NpcName"
	name_label.text = "药童"
	name_label.add_theme_font_size_override("font_size", 7)
	name_label.add_theme_color_override("font_color", Color(0.95, 0.92, 0.8, 1))
	name_label.position = Vector2(-40, -38)
	name_label.size = Vector2(80, 10)
	name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(name_label)
	# 对话气泡（隐藏，X 时显示）
	_bubble_bg = ColorRect.new()
	_bubble_bg.name = "BubbleBg"
	_bubble_bg.position = Vector2(-70, -58)
	_bubble_bg.size = Vector2(140, 16)
	_bubble_bg.color = Color(0.08, 0.08, 0.1, 0.85)
	_bubble_bg.visible = false
	add_child(_bubble_bg)
	_bubble = Label.new()
	_bubble.name = "Bubble"
	_bubble.add_theme_font_size_override("font_size", 8)
	_bubble.add_theme_color_override("font_color", Color(1.0, 0.95, 0.8, 1))
	_bubble.position = Vector2(-70, -56)
	_bubble.size = Vector2(140, 14)
	_bubble.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_bubble.visible = false
	add_child(_bubble)

func _mgr():
	return get_tree().current_scene.find_child("DongtianManager", true, false)

func _hired() -> bool:
	var mgr = _mgr()
	return bool(mgr.call("is_yaotong_hired")) if mgr else false

func _on_body_entered(body):
	if body.name != "Player":
		return
	# 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if body.global_position.distance_to(global_position) > 48.0:
		return
	_player_inside = true
	_player = body
	_refresh_prompt()

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	_player = null
	if body.get_parent() != get_parent():
		return
	_prompt("", false)

func _process(d):
	# 气泡计时收起
	if _bubble_t > 0.0:
		_bubble_t -= d
		if _bubble_t <= 0.0:
			_bubble.visible = false
			_bubble_bg.visible = false
	if not _player_inside or _player == null:
		return
	_refresh_t -= d
	if _refresh_t <= 0.0:
		_refresh_t = 0.4
		_refresh_prompt()
	if Input.is_action_just_pressed("interact"):
		_interact()

func _interact():
	var mgr = _mgr()
	if mgr == null:
		return
	var was_hired := _hired()
	mgr.call("set_yaotong_hired", not was_hired)
	# 台词按新状态选组循环（TownNpc 模式）
	var lines := LINES_HIRED if not was_hired else LINES_IDLE
	_say(lines[_line_idx % lines.size()])
	_line_idx += 1
	_refresh_prompt()

func _refresh_prompt():
	if _hired():
		_prompt("[X] 收回委托（药童看管灵田中）", true)
	else:
		_prompt("[X] 托付灵田给药童", true)

func _say(text: String):
	_bubble.text = text
	_bubble.visible = true
	_bubble_bg.visible = true
	_bubble_t = 2.5

func _prompt(text: String, show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, show)
