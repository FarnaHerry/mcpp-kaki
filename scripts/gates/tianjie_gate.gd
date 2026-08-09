extends Area2D
## 南天门·天界之门：↑ 触发登天（交互模板同 SceneGate：幽灵 enter 守卫 + 同空间离开才清提示）。
## realm<10 拒绝话术「天威浩荡，真仙方可登天」；realm>=10 → ContinentManager.travel_to_direct("tianjie")
## （真仙腾云驾雾直达天界，不渡云海——云海是金丹门控的凡俗强渡；旅行桥携带全量状态）。
@export var prompt := "[↑] 登天界"
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
	# 幽灵 enter 守卫（同 SceneGate/StorageChest）
	if body.global_position.distance_to(global_position) > 48.0:
		return
	_player_inside = true
	_prompt(prompt)

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	if body.get_parent() != get_parent():
		return
	_prompt("")

func _process(_d):
	if not (_player_inside and Input.is_action_just_pressed("up")):
		return
	var scene = get_tree().current_scene
	var gm = scene.get_node_or_null("GameManager")
	var cm = scene.get_node_or_null("ContinentManager")
	var realm = -1
	if gm:
		var player = gm.call("get_player")
		if player and player.has_method("get_cultivation"):
			var cult = player.call("get_cultivation")
			if cult:
				realm = int(cult.call("get_realm_index"))
	if realm < 10:
		_flash("天威浩荡，真仙方可登天")
		return
	if cm and bool(cm.call("travel_to_direct", "tianjie")):
		_player_inside = false
		return
	# ContinentManager 尚未接入 continents.json 时的兜底（旅行未成行，话术说明）
	_flash("天界之路尚未贯通")

func _prompt(text: String):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, text != "")

# 拒绝/兜底话术 2.5s 自消隐（仍在门内则恢复常规提示）
func _flash(text: String):
	_prompt(text)
	await get_tree().create_timer(2.5).timeout
	if not is_inside_tree():
		return
	_prompt(prompt if _player_inside else "")
