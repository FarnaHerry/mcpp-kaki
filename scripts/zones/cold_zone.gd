extends Area2D
## 极寒区（玄冰窟）：玩家进入减速 30%（Player._chilled）+ 每 tick 冰伤 dot。
## 冰伤走 DMG_ELEMENTAL + ELEM_SHUI —— 冰心丹（水抗）可减免，与炼丹主题闭环。
@export var damage_frac := 0.04 # 每 tick 扣 max 血比例
@export var tick := 0.5         # 伤害间隔秒

var _dot := 0.0

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	set_deferred("monitoring", true)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	set_physics_process(true)

func _on_body_entered(body):
	if body.name != "Player":
		return
	body.call("set_chilled", true)

func _on_body_exited(body):
	if body.name != "Player":
		return
	body.call("set_chilled", false)

func _physics_process(delta):
	var bodies = get_overlapping_bodies()
	if bodies.is_empty():
		_dot = 0.0
		return
	_dot += delta
	while _dot >= tick:
		_dot -= tick
		for b in bodies:
			if b.name == "Player":
				# DMG_ELEMENTAL(2) + ELEM_SHUI(3)：冰伤，吃水抗
				b.call("take_damage_typed", float(b.call("get_max_health")) * damage_frac, 2, 3, self)
				break
