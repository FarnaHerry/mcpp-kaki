extends Area2D
## 灵泉打坐点（洞天设施补全）：灵泉旁 X 入坐打坐——复用 Player Q 打坐管线
## （站定回灵+修为，倍率吃聚灵阵 get_dongtian_meditate_mult），再按 X/移动收功。
## StorageChest/JlzEye 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
var _player_inside := false
var _player: Node = null
var _refresh_t := 0.0
var _release_pending := false # 模拟 cultivate 键：按一帧后释放（同帧 press+release 不可靠）

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(40, 50)
	shape.shape = rect
	add_child(shape)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	set_process(true)

func _on_body_entered(body):
	if body.name != "Player":
		return
	# 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if body.global_position.distance_to(global_position) > 48.0:
		return
	_player_inside = true
	_player = body
	_refresh_t = 0.0

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	_player = null
	if body.get_parent() != get_parent():
		return
	# 打坐中提示归 Meditate 状态自身管理，离开不收
	if bool(body.call("is_meditating")):
		return
	_prompt("", false)

func _process(d):
	if _release_pending:
		Input.action_release("cultivate")
		_release_pending = false
	if not _player_inside or _player == null:
		return
	# 打坐中：提示归 Meditate 状态自身（打坐中…），本点只负责 X 收功
	if bool(_player.call("is_meditating")):
		if Input.is_action_just_pressed("interact"):
			_tap_cultivate() # 再按 X = 收功（与 Q 同管线切换）
		return
	_refresh_t -= d
	if _refresh_t <= 0.0:
		_refresh_t = 0.3
		var mult := float(_player.call("get_dongtian_meditate_mult"))
		_prompt("[X] 灵泉打坐 · 聚灵阵×%s" % String.num(mult, 2), true)
	if Input.is_action_just_pressed("interact"):
		_tap_cultivate()

func _tap_cultivate():
	# 模拟 Q（cultivate）：入坐/收功切换走 Player 既有打坐状态机。
	# 提示激活期 attack_just_pressed 被交互压制，X 不出刀不打断。
	Input.action_press("cultivate")
	_release_pending = true

func _prompt(text: String, show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, show)
