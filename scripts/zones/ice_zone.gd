extends Area2D
## 冰面打滑区（北俱芦洲·极北冰原）：玩家进入后 Idle 摩擦骤减 + Run 渐进加速，
## 惯性滑冰手感（Player._slippery → Idle/Run 状态分支；离开恢复）。
func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	set_deferred("monitoring", true)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)

func _on_body_entered(body):
	if body.name != "Player":
		return
	body.call("set_slippery", true)

func _on_body_exited(body):
	if body.name != "Player":
		return
	body.call("set_slippery", false)
