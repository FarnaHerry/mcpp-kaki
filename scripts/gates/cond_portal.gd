extends Area2D
## 条件房间门：GameManager flag 满足 → ↑ 触发内部 Portal 进房间（Portal 房间模式）；
## 不满足 → 拒绝提示（2.5s 自消）。用于 Boss 守关的房间入口（凌霄宝殿需 boss_dead:巨灵神）。
## 装配（tianjie.gd）：set() 属性 → add_child → setup(player, camera)。
@export var flag := ""
@export var prompt := "[↑] 进入"
@export var refuse_text := ""
@export var target_scene := ""
@export var room_bounds := Rect2(0, 0, 480, 270)

var _player_inside := false
var _portal: Node = null
var _refuse_t := 0.0

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	var cs = CollisionShape2D.new()
	var cr = RectangleShape2D.new()
	cr.size = Vector2(32, 80)
	cs.shape = cr
	add_child(cs)
	# 门扉视觉（金框）
	var vis = Polygon2D.new()
	vis.color = Color(0.9, 0.8, 0.35, 0.6)
	vis.polygon = PackedVector2Array([Vector2(-10, -30), Vector2(10, -30), Vector2(10, 30), Vector2(-10, 30)])
	add_child(vis)
	set_process(true)

## 创建内部 Portal（自带检测关掉——本门统一门控）
func setup(player: Node, camera: Node) -> void:
	var portal = ClassDB.instantiate("Portal")
	portal.set("target_scene", target_scene)
	portal.set("prompt_text", prompt)
	portal.set("room_bounds", room_bounds)
	portal.call("set_player", player)
	portal.call("set_camera", camera)
	var ds = CollisionShape2D.new()
	var dr = RectangleShape2D.new()
	dr.size = Vector2(32, 80)
	ds.shape = dr
	portal.add_child(ds)
	add_child(portal)
	portal.monitoring = false # Portal._ready 置 true，压回——trigger() 由本门手动调
	_portal = portal

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

func _process(d):
	if _refuse_t > 0.0:
		_refuse_t -= d
		if _refuse_t <= 0.0:
			# 拒绝提示消隐后恢复常态提示（玩家仍在门内）
			if _player_inside:
				_prompt(true)
			else:
				_prompt(false)
	if _player_inside and Input.is_action_just_pressed("up"):
		var gm = get_tree().current_scene.get_node_or_null("GameManager")
		if gm and (flag == "" or bool(gm.call("get_flag", flag, false))):
			if _portal:
				_portal.call("trigger")
		else:
			_emit_prompt(refuse_text)
			_refuse_t = 2.5

func _prompt(show: bool):
	_emit_prompt(prompt if show else "")

func _emit_prompt(text: String):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, text != "")
