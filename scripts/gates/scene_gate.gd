extends Area2D
## 通用场景门：↑ 触发，调用 GameManager 的指定方法（如 enter_difu / huan_yang）。
## 地府用全场景切换（change_scene_to_file + 旅行桥），不能用普通 Portal（房间语义，会污染 _respawn_scene）。
@export var gm_method := ""
@export var prompt := "[↑]"
var _player_inside := false

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
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
	_prompt(true)

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	# 同空间离开才清提示
	if body.get_parent() != get_parent():
		return
	_prompt(false)

func _process(_d):
	if _player_inside and Input.is_action_just_pressed("up"):
		var gm = get_tree().current_scene.get_node_or_null("GameManager")
		if gm and gm.has_method(gm_method):
			gm.call(gm_method)

func _prompt(show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", prompt if show else "", show)
