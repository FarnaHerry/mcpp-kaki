extends Area2D
## 丹房（洞天设施补全）：贴近显示「[X] 丹房炼丹」，
## X 打开 PillLabPanel（就地炼丹，复用 AlchemySystem，打开暂停）。
## StorageChest/JlzEye 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
const PillLabPanel := preload("res://scripts/spots/pill_lab_panel.gd")

var _player_inside := false
var _player: Node = null
var _panel: CanvasLayer = null
var _refresh_t := 0.0

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(30, 46)
	shape.shape = rect
	add_child(shape)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	set_process(true)
	# 丹房视觉：小丹炉（赭石炉身 + 火口）
	var body := Polygon2D.new()
	body.color = Color(0.6, 0.38, 0.22, 1)
	body.polygon = PackedVector2Array([-9, 0, -9, -14, -5, -20, 5, -20, 9, -14, 9, 0])
	add_child(body)
	var fire := Polygon2D.new()
	fire.color = Color(1.0, 0.62, 0.2, 0.9)
	fire.polygon = PackedVector2Array([-3, -8, 0, -13, 3, -8, 0, -5])
	add_child(fire)
	# 炼丹面板（CanvasLayer，随洞天场景卸载）
	_panel = PillLabPanel.new()
	add_child(_panel)

func _on_body_entered(body):
	if body.name != "Player":
		return
	# 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if body.global_position.distance_to(global_position) > 48.0:
		return
	_player_inside = true
	_player = body
	_prompt("[X] 丹房炼丹", true)

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	_player = null
	if body.get_parent() != get_parent():
		return
	if _panel.is_open():
		return
	_prompt("", false)

func _process(d):
	if not _player_inside or _player == null:
		return
	if _panel.is_open():
		return # 面板打开期间提示归面板，交互归面板
	# 周期重声明（防同帧 enter/exit 乱序误清提示；面板关闭后提示自动恢复）
	_refresh_t -= d
	if _refresh_t <= 0.0:
		_refresh_t = 0.3
		_prompt("[X] 丹房炼丹", true)
	if Input.is_action_just_pressed("interact"):
		_panel.open_panel(_player)
		_prompt("", false) # 面板自遮蔽，收起门口提示

func _prompt(text: String, show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, show)
