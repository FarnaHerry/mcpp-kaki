extends Area2D
## 傀儡（洞天 v5 经营）：灵石购买激活（CurrencySystem 四阶钱包，下品基准），
## 激活后 X 切换双模式（状态自持于 DongtianManager 随档持久化）：
##   驻守模式——所有生长中灵田地块生长速度 +50%（Manager 生长计时统一挂钩）；
##   随行模式——傀儡实体（CloneAvatar 弱化快照 HP×40%/攻×40%）跟随玩家出战，
##   仅洞天内/洲野外部署，进 Portal 房间自动收回，战死后 60s 冷却重新召出。
## 台词气泡 TownNpc 模式（X 触发，2.5s 自消，按模式两组文案循环）。
## StorageChest/YaoTong 交互模板（幽灵 enter 守卫 + 同空间离开才清提示）。
## 用法：dongtian.gd 运行时创建（灵田左端上空浮台 x=40——
## 出生点 (240,200) 的 48px 幽灵 enter 守卫半径内不得放交互物，见 dongtian.gd 注释）。
const LINES_GARRISON := ["傀儡驻守灵田，灵植生长加速五成。", "土木为躯，符箓为心。"]
const LINES_FOLLOW := ["傀儡随行，护主出战。", "此躯虽钝，尚可一战。"]

var _player_inside := false
var _player: Node = null
var _refresh_t := 0.0
var _bubble_t := 0.0
var _line_idx := 0
var _bubble: Label = null
var _bubble_bg: ColorRect = null
var _eye: Polygon2D = null # 傀儡眼：沉睡暗/激活亮（视觉状态缓存避免每帧改色）
var _awake := false

func _ready():
	set_collision_layer_value(1, false)
	set_collision_mask_value(3, true)
	var shape = CollisionShape2D.new()
	var rect = RectangleShape2D.new()
	rect.size = Vector2(28, 46)
	shape.shape = rect
	shape.position = Vector2(0, -6)
	add_child(shape)
	connect("body_entered", _on_body_entered)
	connect("body_exited", _on_body_exited)
	_build_visuals()
	set_process(true)

func _build_visuals():
	# 傀儡视觉：木质方躯（关节人形）+ 方头 + 符箓眼
	var body = Polygon2D.new()
	body.name = "Body"
	body.color = Color(0.5, 0.38, 0.24, 1)
	body.polygon = PackedVector2Array([-6, -12, 6, -12, 7, 10, -7, 10])
	add_child(body)
	var joints = Polygon2D.new()
	joints.name = "Joints"
	joints.color = Color(0.38, 0.28, 0.17, 1)
	joints.polygon = PackedVector2Array([-9, -10, -6, -10, -6, 2, -9, 2, 6, -10, 9, -10, 9, 2, 6, 2])
	add_child(joints)
	var head = Polygon2D.new()
	head.name = "Head"
	head.color = Color(0.58, 0.46, 0.3, 1)
	head.polygon = PackedVector2Array([-5, -20, 5, -20, 5, -12, -5, -12])
	add_child(head)
	_eye = Polygon2D.new()
	_eye.name = "Eye"
	_eye.color = Color(0.2, 0.16, 0.12, 1) # 沉睡：暗
	_eye.polygon = PackedVector2Array([-3, -17, 3, -17, 3, -15, -3, -15])
	add_child(_eye)
	var name_label = Label.new()
	name_label.name = "NpcName"
	name_label.text = "傀儡"
	name_label.add_theme_font_size_override("font_size", 7)
	name_label.add_theme_color_override("font_color", Color(0.95, 0.92, 0.8, 1))
	name_label.position = Vector2(-40, -38)
	name_label.size = Vector2(80, 10)
	name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	add_child(name_label)
	# 对话气泡（隐藏，X 时显示）
	_bubble_bg = ColorRect.new()
	_bubble_bg.name = "BubbleBg"
	_bubble_bg.position = Vector2(-70, -58)
	_bubble_bg.size = Vector2(140, 16)
	_bubble_bg.color = Color(0.08, 0.08, 0.1, 0.85)
	_bubble_bg.visible = false
	add_child(_bubble_bg)
	_bubble = Label.new()
	_bubble.name = "Bubble"
	_bubble.add_theme_font_size_override("font_size", 8)
	_bubble.add_theme_color_override("font_color", Color(1.0, 0.95, 0.8, 1))
	_bubble.position = Vector2(-70, -56)
	_bubble.size = Vector2(140, 14)
	_bubble.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_bubble.visible = false
	add_child(_bubble)

func _mgr():
	return get_tree().current_scene.find_child("DongtianManager", true, false)

func _cur():
	return get_tree().current_scene.find_child("CurrencySystem", true, false)

func _active() -> bool:
	var mgr = _mgr()
	return bool(mgr.call("is_puppet_active")) if mgr else false

func _mode() -> int:
	var mgr = _mgr()
	return int(mgr.call("get_puppet_mode")) if mgr else 0

func _on_body_entered(body):
	if body.name != "Player":
		return
	# 幽灵 enter 守卫（同 StorageChest）：reparent 帧物理误报远处重叠
	if body.global_position.distance_to(global_position) > 48.0:
		return
	_player_inside = true
	_player = body
	_refresh_prompt()

func _on_body_exited(body):
	if body.name != "Player":
		return
	if not _player_inside:
		return
	_player_inside = false
	_player = null
	if body.get_parent() != get_parent():
		return
	_prompt("", false)

func _process(d):
	# 气泡计时收起
	if _bubble_t > 0.0:
		_bubble_t -= d
		if _bubble_t <= 0.0:
			_bubble.visible = false
			_bubble_bg.visible = false
	# 激活态视觉（符箓眼：暗→亮）
	var awake := _active()
	if awake != _awake:
		_awake = awake
		_eye.color = Color(0.85, 0.3, 0.2, 1) if awake else Color(0.2, 0.16, 0.12, 1)
	if not _player_inside or _player == null:
		return
	_refresh_t -= d
	if _refresh_t <= 0.0:
		_refresh_t = 0.4
		_refresh_prompt()
	if Input.is_action_just_pressed("interact"):
		_interact()

func _interact():
	var mgr = _mgr()
	if mgr == null:
		return
	if not _active():
		# 灵石购买激活
		if bool(mgr.call("activate_puppet")):
			_say(LINES_GARRISON[0])
			_line_idx = 1
		else:
			_say("灵石不足，傀儡难以唤醒。")
		_refresh_prompt()
		return
	# 已激活：X 切换驻守/随行
	mgr.call("set_puppet_mode", 1 - _mode())
	var lines := LINES_FOLLOW if _mode() == 1 else LINES_GARRISON
	_say(lines[_line_idx % lines.size()])
	_line_idx += 1
	_refresh_prompt()

func _refresh_prompt():
	var mgr = _mgr()
	if mgr == null:
		return
	if not _active():
		var cost := int(mgr.call("get_puppet_cost"))
		var cur = _cur()
		if cur and int(cur.call("get_total")) >= cost:
			_prompt("[X] 唤醒傀儡 · 下品×%d" % cost, true)
		else:
			_prompt("唤醒傀儡 需下品×%d（灵石不足）" % cost, true)
		return
	if _mode() == 1:
		# 随行模式：战死修复期提示冷却
		var cd := float(mgr.call("get_puppet_cooldown"))
		if cd > 0.0:
			_prompt("傀儡战毁修复中 · %d息" % int(ceil(cd)), true)
		else:
			_prompt("[X] 傀儡换岗 · 驻守灵田（当前：随行出战）", true)
	else:
		_prompt("[X] 傀儡换岗 · 随行出战（当前：驻守灵田）", true)

func _say(text: String):
	_bubble.text = text
	_bubble.visible = true
	_bubble_bg.visible = true
	_bubble_t = 2.5

func _prompt(text: String, show: bool):
	var bus = get_tree().current_scene.get_node_or_null("SignalBus")
	if bus:
		bus.emit_signal("interaction_prompt", text, show)
