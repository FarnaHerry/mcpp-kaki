extends Area2D
## 炼体圣地（北俱芦洲·南天门，design/world-map.md v5「炼体圣地」）：
## 贴近显 "[X] 炼体"，交互 → 炼体 buff（防+20% 600s）+ 少量修为。
## StorageChest/SceneGate 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
var _player_inside := false
var _player: Node = null

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	# 自带交互判定区（同 ShopKeeper：节点自持形状，地图只摆位置）
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
	_prompt(true)

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	_player = null
	if body.get_parent() != get_parent():
		return
	_prompt(false)

func _process(_d):
	if _player_inside and Input.is_action_just_pressed("interact"):
		_refine()

func _refine():
	if _player == null or not is_instance_valid(_player):
		return
	var buffs = _player.call("get_buffs")
	if buffs == null:
		return
	buffs.call("apply", "buff_lianti")
	_player.call("gain_spiritual_energy", 10.0)
	_result("玄冰淬体 · 炼体：防御+20%（600s）")

func _prompt(show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", "[X] 炼体" if show else "", show)

func _result(msg: String):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus == null:
		return
	bus.emit_signal("interaction_prompt", msg, true)
	var timer = get_tree().create_timer(2.5)
	timer.timeout.connect(func(): bus.emit_signal("interaction_prompt", "", false))
