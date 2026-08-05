extends Area2D
## 环境火伤区（火焰山）：玩家在区域内每 tick 扣血（三灾阴火 dot 模式）。
## 芭蕉扇使用后可熄灭（extinguish）：停伤害 + 隐藏视觉 + 关 monitoring。
@export var damage_frac := 0.06 # 每 tick 扣 max 血比例
@export var tick := 0.5         # 伤害间隔秒

var _dot := 0.0
var _extinguished := false

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	set_deferred("monitoring", true)
	add_to_group("fire_zones")
	set_physics_process(true)

func _physics_process(delta):
	if _extinguished:
		return
	var bodies = get_overlapping_bodies()
	if bodies.is_empty():
		_dot = 0.0
		return
	_dot += delta
	while _dot >= tick:
		_dot -= tick
		for b in bodies:
			if b.name == "Player":
				b.call("take_damage", float(b.call("get_max_health")) * damage_frac, self)
				break

func extinguish():
	_extinguished = true
	monitoring = false
	for c in get_children():
		if c is Polygon2D or c is ColorRect:
			c.visible = false

func is_extinguished() -> bool:
	return _extinguished
