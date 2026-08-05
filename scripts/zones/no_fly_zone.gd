extends Area2D
## 弱水区（流沙河）：玩家在内禁飞（can_fly false + 飞行中强制坠落）。
## 飞行失效，须跳跃/走石墩过河。

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	set_deferred("monitoring", true)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)

func _on_body_entered(body):
	if body.name == "Player":
		body.call("set_flight_blocked", true)

func _on_body_exited(body):
	if body.name == "Player":
		body.call("set_flight_blocked", false)
